#include "NetworkEditor.h"
#include "SourceGraphicsItem.h"
#include "PortGraphicsItem.h"
#include "ConnectionGraphicsItem.h"
#include "vtkPVNetworkEditorSettings.h"

#include <vtkSMProxy.h>
#include <vtkSMSourceProxy.h>
#include <vtkSMPropertyIterator.h>
#include <vtkSMInputProperty.h>
#include <vtkSMPropertyHelper.h>
#include <vtkSMSessionProxyManager.h>
#include <vtkPVXMLElement.h>
#include <vtkPVXMLParser.h>
#include <vtkSMProxyManager.h>
#include <vtkCollection.h>

#include <pqPipelineFilter.h>
#include <pqPipelineSource.h>
#include <pqActiveObjects.h>
#include <pqApplicationCore.h>
#include <pqServerManagerModel.h>
#include <pqRepresentation.h>

#include <QPainter>
#include <QGraphicsSceneContextMenuEvent>
#include <QMenu>
#include <QMimeData>
#include <QApplication>
#include <QClipboard>

#include <algorithm>
#include <set>

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

  connect(&pqActiveObjects::instance(), &pqActiveObjects::viewChanged, this, [this](pqView*) {
    this->update();
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

  connect(smModel, &pqServerManagerModel::connectionRemoved, this, [this](pqPipelineSource* source, pqPipelineSource* dest, int sourcePort) {
    std::cout << "removed connection "
              << source->getSMName().toStdString() << " (" << sourcePort << ") -> " << dest->getSMName().toStdString()  << std::endl;
    updateConnectionRepresentations(source, dest);
  });

  connect(smModel, &pqServerManagerModel::representationAdded, this, [this](pqRepresentation* rep) {
    std::cout << "added representation " << rep->getSMName().toStdString() << std::endl;
  });

  connect(smModel, &pqServerManagerModel::representationRemoved, this, [this](pqRepresentation* rep) {
    std::cout << "removed representation " << rep->getSMName().toStdString() << std::endl;
  });

  connect(smModel, &pqServerManagerModel::modifiedStateChanged, this, [this](pqServerManagerModelItem* item) {
    auto source = qobject_cast<pqPipelineSource*>(item);
    if (!source)
      return;
    auto it = sourceGraphicsItems_.find(source);
    if (it != sourceGraphicsItems_.end()) {
      it->second->update();
    }
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
  // remove connections that belong to source
  std::vector<pqPipelineSource*> dests, sources;
  for (const auto& kv : connectionGraphicsItems_) {
    if (std::get<0>(kv.first) == source) {
      dests.push_back(std::get<1>(kv.first));
    }
    if (std::get<1>(kv.first) == source) {
      sources.push_back(std::get<0>(kv.first));
    }
  }
  for (auto dest : dests) {
    std::vector<ConnectionGraphicsItem*> connections;
    for (const auto& kv : connectionGraphicsItems_[std::make_tuple(source, dest)]) {
      connections.push_back(kv.second);
    }
    for (auto connection : connections) {
      delete connection;
    }
    connectionGraphicsItems_.erase(std::make_tuple(source, dest));
  }
  for (auto src : sources) {
    std::vector<ConnectionGraphicsItem*> connections;
    for (const auto& kv : connectionGraphicsItems_[std::make_tuple(src, source)]) {
      connections.push_back(kv.second);
    }
    for (auto connection : connections) {
      delete connection;
    }
    connectionGraphicsItems_.erase(std::make_tuple(src, source));
  }


  auto it = sourceGraphicsItems_.find(source);
  if (it == sourceGraphicsItems_.end())
    return;
  this->removeItem(it->second);
  delete it->second;
  sourceGraphicsItems_.erase(it);
}

void NetworkEditor::updateConnectionRepresentations(pqPipelineSource* source, pqPipelineSource* dest) {
  std::tuple<pqPipelineSource*, pqPipelineSource*> key(source, dest);
  std::set<std::tuple<int, int>> sm_connections, connections;

  // collect currently known connections
  auto it = connectionGraphicsItems_.find(key);
  if (it == connectionGraphicsItems_.end()) {
    connectionGraphicsItems_[key] = std::map<std::tuple<int, int>, ConnectionGraphicsItem*>();
  } else {
    for (auto const& kv : connectionGraphicsItems_[key]) {
      connections.insert(kv.first);
    }
  }

  // collect connections from ParaView pipeline
  auto smModel = pqApplicationCore::instance()->getServerManagerModel();
  pqPipelineFilter* filter = qobject_cast<pqPipelineFilter*>(dest);
  assert(filter);
  for (int input_id = 0; input_id < filter->getNumberOfInputPorts(); ++input_id) {
    const char* input_name = filter->getInputPortName(input_id).toLocal8Bit().constData();
    auto prop = vtkSMInputProperty::SafeDownCast(filter->getProxy()->GetProperty(input_name));
    assert(prop);

    vtkSMPropertyHelper helper(prop);
    unsigned int num_proxies = helper.GetNumberOfElements();
    for (unsigned int i = 0; i < num_proxies; ++i) {
      auto proxy_source = smModel->findItem<pqPipelineSource*>(helper.GetAsProxy(i));
      if (!proxy_source || proxy_source != source)
        continue;
      if (!proxy_source->getAllConsumers().contains(dest))
        continue;
      int output_id = helper.GetOutputPort(i);
      sm_connections.insert(std::make_tuple(output_id, input_id));
    }
  }

  // debug output
  std::cout << "Known: ";
  for (const auto& conn : connections) {
    std::cout << std::get<0>(conn) << "->" << std::get<1>(conn) << "; ";
  }
  std::cout << std::endl;
  std::cout << "SM: ";
  for (const auto& conn : sm_connections) {
    std::cout << std::get<0>(conn) << "->" << std::get<1>(conn) << "; ";
  }
  std::cout << std::endl;

  std::set<std::tuple<int, int>> added, removed;
  std::set_difference(
      sm_connections.begin(), sm_connections.end(),
      connections.begin(), connections.end(),
      std::inserter(added, added.begin()));
  std::set_difference(
      connections.begin(), connections.end(),
      sm_connections.begin(), sm_connections.end(),
      std::inserter(removed, removed.begin()));

  for (const auto& conn : removed) {
    if (connectionGraphicsItems_[key].count(conn) < 1) {
      continue;
    }
    auto graphics_item = connectionGraphicsItems_[key][conn];
    connectionGraphicsItems_[key].erase(conn);
    delete graphics_item;
  }

  for (const auto& conn : added) {
    auto outport_graphics = this->sourceGraphicsItems_[source]->getOutputPortGraphicsItem(std::get<0>(conn));
    auto inport_graphics = this->sourceGraphicsItems_[dest]->getInputPortGraphicsItem(std::get<1>(conn));

    if (!inport_graphics || !outport_graphics)
      continue;

    auto connection = new ConnectionGraphicsItem(outport_graphics, inport_graphics);
    this->addItem(connection);
    connectionGraphicsItems_[key][conn] = connection;
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
  menu.addSeparator();
  auto copy = menu.addAction(tr("Copy"));
  connect(copy, &QAction::triggered, this, &NetworkEditor::copy);

  auto paste = menu.addAction(tr("Paste"));
  connect(paste, &QAction::triggered, [this, e]() {
    QPointF pos = e->scenePos();
    this->paste(pos.x(), pos.y());
  });

  if (!menu.isEmpty()) {
    menu.exec(QCursor::pos());
    e->accept();
  }
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

  if (vtkPVNetworkEditorSettings::GetInstance()->GetUpdateActiveObject()) {
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

void NetworkEditor::copy() {
  auto app = pqApplicationCore::instance();
  // vtkPVXMLElement* state = app->saveState();
  vtkPVXMLElement* state = vtkPVXMLElement::New();
  state->SetName("ParaView");

  vtkPVXMLElement* rootElement = vtkPVXMLElement::New();
  rootElement->SetName("ServerManagerState");
  std::ostringstream version_string;
  version_string << vtkSMProxyManager::GetVersionMajor() << "."
                 << vtkSMProxyManager::GetVersionMinor() << "."
                 << vtkSMProxyManager::GetVersionPatch();
  rootElement->AddAttribute("version", version_string.str().c_str());

  float x_min(std::numeric_limits<float>::max()), y_min(std::numeric_limits<float>::max());
  for (auto item : this->selectedItems()) {
    auto source_item = qgraphicsitem_cast<SourceGraphicsItem *>(item);
    if (!source_item)
      continue;
    x_min = std::min(float(item->x()), x_min);
    y_min = std::min(float(item->y()), y_min);
  }

  // add sources
  std::vector<std::tuple<std::string, vtkSMProxy*>> source_proxies;
  for (auto item : this->selectedItems()) {
    auto source_item = qgraphicsitem_cast<SourceGraphicsItem *>(item);
    if (!source_item)
      continue;

    auto source = source_item->getSource();
    vtkSMProxy* proxy = source->getProxy();
    source_proxies.emplace_back(std::make_tuple(source->getSMName().toStdString(), proxy));
    std::string node_x = proxy->GetAnnotation("Node.x");
    std::string node_y = proxy->GetAnnotation("Node.y");

    proxy->SetAnnotation("Node.x", std::to_string(item->x() - x_min).c_str());
    proxy->SetAnnotation("Node.y", std::to_string(item->y() - y_min).c_str());
    proxy->SaveXMLState(rootElement);

    proxy->SetAnnotation("Node.x", node_x.c_str());
    proxy->SetAnnotation("Node.y", node_y.c_str());
  }

  // add proxycollection for sources
  vtkPVXMLElement* collectionElement = vtkPVXMLElement::New();
  collectionElement->SetName("ProxyCollection");
  collectionElement->AddAttribute("name", "sources");
  for (const auto& kv : source_proxies) {
    vtkPVXMLElement* itemElement = vtkPVXMLElement::New();
    itemElement->SetName("Item");
    itemElement->AddAttribute("id", std::get<1>(kv)->GetGlobalID());
    itemElement->AddAttribute("name", std::get<0>(kv).c_str());
    if (std::get<1>(kv)->GetLogName() != nullptr)
    {
      itemElement->AddAttribute("logname", std::get<1>(kv)->GetLogName());
    }
    collectionElement->AddNestedElement(itemElement);
    itemElement->Delete();
  }
  rootElement->AddNestedElement(collectionElement);
  collectionElement->Delete();

  state->AddNestedElement(rootElement);
  rootElement->FastDelete();

  std::stringstream ss;
  state->PrintXML(ss, vtkIndent());
  std::string str = ss.str();
  // std::cout << str << std::endl;
  QByteArray data(str.c_str(), str.length());
  auto mimedata = std::make_unique<QMimeData>();
  mimedata->setData(QString("text/plain"), data);
  QApplication::clipboard()->setMimeData(mimedata.release());
}

void NetworkEditor::paste(float x, float y) {
  auto clipboard = QApplication::clipboard();
  auto mimeData = clipboard->mimeData();
  if (!mimeData->formats().contains("text/plain"))
    return;
  QByteArray data = mimeData->data(QString("text/plain"));
  std::string str(data.constData(), data.length());

  auto parser = vtkSmartPointer<vtkPVXMLParser>::New();
  if (!parser->Parse(data.constData()))
    return;

  auto annotations = vtkSmartPointer<vtkCollection>::New();
  parser->GetRootElement()->GetElementsByName("Annotation", annotations);

  for (int i = 0; i < annotations->GetNumberOfItems(); ++i) {
    auto annotation = vtkPVXMLElement::SafeDownCast(annotations->GetItemAsObject(i));
    if (!annotation)
      continue;
    if (std::string(annotation->GetAttribute("key")) == "Node.x") {
      float node_x = x + std::atof(annotation->GetAttribute("value")) + SourceGraphicsItem::size_.width() / 2.;
      annotation->SetAttribute("value", std::to_string(node_x).c_str());
    }
    if (std::string(annotation->GetAttribute("key")) == "Node.y") {
      float node_y = y + std::atof(annotation->GetAttribute("value")) + SourceGraphicsItem::size_.height() / 2.;
      annotation->SetAttribute("value", std::to_string(node_y).c_str()) ;
    }
  }

  auto app = pqApplicationCore::instance();
  auto server = app->getActiveServer();
  server->proxyManager()->LoadXMLState(parser->GetRootElement(), nullptr, false);
}
