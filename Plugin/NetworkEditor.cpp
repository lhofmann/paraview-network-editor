#include "NetworkEditor.h"
#include "SourceGraphicsItem.h"
#include "PortGraphicsItem.h"
#include "ConnectionGraphicsItem.h"

#include <vtkSMProxy.h>
#include <vtkSMSourceProxy.h>
#include <vtkSMPropertyIterator.h>
#include <vtkSMInputProperty.h>
#include <vtkSMPropertyHelper.h>

#include <pqPipelineSource.h>
#include <pqActiveObjects.h>
#include <pqApplicationCore.h>
#include <pqServerManagerModel.h>

#include <QPainter>
#include <QGraphicsSceneContextMenuEvent>
#include <QMenu>

const int NetworkEditor::gridSpacing_ = 25;

NetworkEditor::NetworkEditor() {
  // The default BSP tends to crash...
  setItemIndexMethod(QGraphicsScene::NoIndex);
  setSceneRect(QRectF());

  connect(this, &QGraphicsScene::selectionChanged, this, &NetworkEditor::onSelectionChanged);

  // observe ParaView's pipeline
  connect(&pqActiveObjects::instance(), &pqActiveObjects::selectionChanged, this, [this](const pqProxySelection& selection) {
    updateSelection_ = true;
    for (auto const& item : sourceGraphicsItems_) {
      item.second->setSelected(selection.contains(item.first));
    }
    updateSelection_ = false;
  });

  auto smModel = pqApplicationCore::instance()->getServerManagerModel();
  connect(smModel, &pqServerManagerModel::sourceAdded, this, [this](pqPipelineSource* source) {
    std::cout << "added source " << source->getSMName().toStdString() << std::endl;
    addSourceRepresentation(source);
  });

  connect(smModel, &pqServerManagerModel::sourceRemoved, this, [this](pqPipelineSource* source) {
    std::cout << "removed source " << source->getSMName().toStdString() << std::endl;
    removeSourceRepresentation(source);
  });

  connect(smModel, &pqServerManagerModel::connectionAdded, this, [this](pqPipelineSource* source, pqPipelineSource* dest, int sourcePort) {
    std::cout << "added connection "
              << source->getSMName().toStdString() << " (" << sourcePort << ") -> " << dest->getSMName().toStdString()  << std::endl;
    updateConnectionRepresentations(source, dest);
  });
}

NetworkEditor::~NetworkEditor() = default;

void NetworkEditor::drawBackground(QPainter* painter, const QRectF& rect) {
  painter->save();
  painter->setWorldMatrixEnabled(true);
  painter->fillRect(rect, QColor(0x7d, 0x80, 0x83));
  qreal left = int(rect.left()) - (int(rect.left()) % gridSpacing_);
  qreal top = int(rect.top()) - (int(rect.top()) % gridSpacing_);
  QVarLengthArray<QLineF, 100> linesX;
  painter->setPen(QColor(153, 153, 153));

  for (qreal x = left; x < rect.right(); x += gridSpacing_)
    linesX.append(QLineF(x, rect.top(), x, rect.bottom()));

  QVarLengthArray<QLineF, 100> linesY;

  for (qreal y = top; y < rect.bottom(); y += gridSpacing_)
    linesY.append(QLineF(rect.left(), y, rect.right(), y));

  painter->drawLines(linesX.data(), linesX.size());
  painter->drawLines(linesY.data(), linesY.size());
  painter->restore();
}

void NetworkEditor::drawForeground(QPainter* painter, const QRectF& rect) {
  // For testing purposes only. Draw bounding rects around all graphics items
  if (false) {
    QList<QGraphicsItem *> items = QGraphicsScene::items(Qt::DescendingOrder);
    painter->setPen(Qt::magenta);
    for (QList<QGraphicsItem *>::iterator it = items.begin(); it != items.end(); ++it) {
      QRectF br = (*it)->sceneBoundingRect();
      painter->drawRect(br);
    }
    painter->setPen(Qt::red);
    painter->drawRect(QGraphicsScene::itemsBoundingRect());
  }
}

void NetworkEditor::addSourceRepresentation(pqPipelineSource* source) {
  auto sourceGraphicsItem = new SourceGraphicsItem(source);

  QPointF pos;
  auto proxy = source->getProxy();
  if (proxy->HasAnnotation("Node.x") && source->getProxy()->HasAnnotation("Node.y")) {
    pos.setX(std::stof(proxy->GetAnnotation("Node.x")));
    pos.setY(std::stof(proxy->GetAnnotation("Node.y")));
  } else {
    pos.setX(this->itemsBoundingRect().left() + SourceGraphicsItem::size_.width() / 2.);
    pos.setY(this->itemsBoundingRect().bottom() + SourceGraphicsItem::size_.height() / 2. + gridSpacing_);
  }
  sourceGraphicsItem->setPos(snapToGrid(pos));

  sourceGraphicsItems_[source] = sourceGraphicsItem;
  this->addItem(sourceGraphicsItem);
}

void NetworkEditor::removeSourceRepresentation(pqPipelineSource* source) {
  auto it = sourceGraphicsItems_.find(source);
  if (it == sourceGraphicsItems_.end())
    return;
  this->removeItem(it->second);
  delete it->second;
  sourceGraphicsItems_.erase(it);
}

// adds connection representations (TODO: remove connection representations)
void NetworkEditor::updateConnectionRepresentations(pqPipelineSource* source, pqPipelineSource* dest) {
  auto smModel = pqApplicationCore::instance()->getServerManagerModel();
  vtkSMPropertyIterator* propIter = dest->getSourceProxy()->NewPropertyIterator();

  for (propIter->Begin(); !propIter->IsAtEnd(); propIter->Next()) {
    if (auto prop = vtkSMInputProperty::SafeDownCast(propIter->GetProperty())) {
      int input_id = prop->GetPortIndex();

      vtkSMPropertyHelper helper(prop);
      unsigned int num_proxies = helper.GetNumberOfElements();
      for (unsigned int i = 0; i < num_proxies; ++i) {
        auto proxy_source = smModel->findItem<pqPipelineSource*>(helper.GetAsProxy(i));
        if (!proxy_source || proxy_source != source)
          continue;
        int output_id = helper.GetOutputPort(i);

        auto key = std::make_tuple(source, output_id, dest, input_id);
        if (connectionGraphicsItems_.count(key) > 0)
          continue;

        auto inport_graphics = this->sourceGraphicsItems_[dest]->getInputPortGraphicsItem(input_id);
        auto outport_graphics = this->sourceGraphicsItems_[source]->getOutputPortGraphicsItem(output_id);

        if (!inport_graphics || !outport_graphics)
          continue;

        auto connection = new ConnectionGraphicsItem(outport_graphics, inport_graphics);
        this->addItem(connection);
        connectionGraphicsItems_[key] = connection;
      }
    }
  }
}

void NetworkEditor::contextMenuEvent(QGraphicsSceneContextMenuEvent* e) {
  QMenu menu;
  for (auto& item : items(e->scenePos())) {
    if (auto source = qgraphicsitem_cast<SourceGraphicsItem*>(item)) {
      auto editName = menu.addAction(tr("Edit Name"));
      connect(editName, &QAction::triggered, [this, source]() {
        clearSelection();
        source->setSelected(true);
        source->editIdentifier();
      });
      break;
    }
  }
  if (!menu.isEmpty()) {
    menu.exec(QCursor::pos());
    e->accept();
  }
}

void NetworkEditor::setAutoUpdateActiveObject(bool enabled) {
  autoUpdateActiveObject_ = enabled;
}

void NetworkEditor::onSelectionChanged() {
  if (updateSelection_)
    return;

  pqProxySelection selection;
  pqPipelineSource* active_source = nullptr;
  int num_selected = 0;
  for (auto item : this->selectedItems()) {
    auto source_item = qgraphicsitem_cast<SourceGraphicsItem*>(item);
    if (!source_item)
      continue;
    auto source = source_item->getSource();
    selection.push_back(source);
    if (!active_source)
      active_source = source;
    ++num_selected;
  }

  if (autoUpdateActiveObject_) {
    pqActiveObjects::instance().setActiveSource(active_source);
    pqActiveObjects::instance().setSelection(selection, active_source);
  } else {
    pqActiveObjects::instance().setSelection(selection, nullptr);
  }
}

QPointF NetworkEditor::snapToGrid(const QPointF& pos) {
  float ox = pos.x() > 0.0f ? 0.5f : -0.5f;
  float oy = pos.y() > 0.0f ? 0.5f : -0.5f;
  float nx = (int(pos.x() / gridSpacing_ + ox)) * gridSpacing_;
  float ny = (int(pos.y() / gridSpacing_ + oy)) * gridSpacing_;
  return {nx, ny};
}

void NetworkEditor::mouseReleaseEvent(QGraphicsSceneMouseEvent* e) {
  // snap selected sources to grid
  for (auto item : this->selectedItems()) {
    if (!qgraphicsitem_cast<SourceGraphicsItem*>(item))
      continue;

    item->setPos(snapToGrid(item->scenePos()));
  }
  QGraphicsScene::mouseReleaseEvent(e);
}

void NetworkEditor::helpEvent(QGraphicsSceneHelpEvent* e) {
  QList<QGraphicsItem*> graphicsItems = items(e->scenePos());
  for (auto item : graphicsItems) {
    switch (item->type()) {
      case SourceGraphicsItem::Type:
        qgraphicsitem_cast<SourceGraphicsItem*>(item)->showToolTip(e);
        return;
      case InputPortGraphicsItem::Type:
        qgraphicsitem_cast<InputPortGraphicsItem*>(item)->showToolTip(e);
        return;
      case OutputPortGraphicsItem::Type:
        qgraphicsitem_cast<OutputPortGraphicsItem*>(item)->showToolTip(e);
        return;
    };
  }
  QGraphicsScene::helpEvent(e);
}
