#include "NetworkEditor.h"
#include "SourceGraphicsItem.h"
#include "PortGraphicsItem.h"
#include "ConnectionGraphicsItem.h"
#include "ConnectionDragHelper.h"
#include "vtkPVNetworkEditorSettings.h"
#include "utilpq.h"
#include "debug_message.h"
#include "vtkPasteStateLoader.h"

#ifdef ENABLE_GRAPHVIZ
# include "graph_layout.h"
#endif

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
#include <vtkVersion.h>
#include <vtkSMParaViewPipelineController.h>
#include <vtkSMViewProxy.h>
#include <vtkSMPropertyHelper.h>

#include <pqPipelineFilter.h>
#include <pqPipelineSource.h>
#include <pqActiveObjects.h>
#include <pqApplicationCore.h>
#include <pqServerManagerModel.h>
#include <pqRepresentation.h>
#include <pqDataRepresentation.h>
#include <pqScalarBarVisibilityReaction.h>
#include <pqDeleteReaction.h>
#include <pqPVApplicationCore.h>
#include <pqParaViewMenuBuilders.h>
#include <pqProxyGroupMenuManager.h>
#include <pqSourcesMenuReaction.h>
#include <pqCoreUtilities.h>
#include <pqUndoStack.h>
#include <pqRepresentation.h>
#include <pqDataRepresentation.h>

#include <QGraphicsView>
#include <QPainter>
#include <QGraphicsSceneContextMenuEvent>
#include <QMenu>
#include <QMimeData>
#include <QApplication>
#include <QClipboard>
#include <QMenuBar>
#include <QMainWindow>

#include <algorithm>
#include <set>

const int NetworkEditor::gridSpacing_ = 25;

NetworkEditor::NetworkEditor()
: connectionDragHelper_{new ConnectionDragHelper(*this)}
{
  // The default BSP tends to crash...
  setItemIndexMethod(QGraphicsScene::NoIndex);
  setSceneRect(QRectF());

  // add current sources
  auto sources = utilpq::get_sources();
  for (pqPipelineSource* source: sources) {
    addSourceRepresentation(source);
  }
  for (pqPipelineSource* sourceA: sources) {
    for (pqPipelineSource* sourceB: sources) {
      updateConnectionRepresentations(sourceA, sourceB);
    }
  }

  installEventFilter(connectionDragHelper_);

  connect(this, &QGraphicsScene::selectionChanged, this, &NetworkEditor::onSelectionChanged);

  // observe ParaView's pipeline
  connect(&pqActiveObjects::instance(), &pqActiveObjects::selectionChanged, this, [this](const pqProxySelection& selection) {
    updateSelection_ = true;
    for (auto const& item : sourceGraphicsItems_) {
      bool selected = selection.contains(item.first);
      for (pqServerManagerModelItem* proxy : selection) {
        auto port = qobject_cast<pqOutputPort*>(proxy);
        if (!port)
          continue;
        selected = selected || (item.first == port->getSource());
      }
      item.second->setSelected(selected);
    }
    updateSelection_ = false;
  });

  connect(&pqActiveObjects::instance(), &pqActiveObjects::portChanged, this, [this](pqOutputPort*) {
    this->update();
  });
  connect(&pqActiveObjects::instance(), &pqActiveObjects::viewChanged, this, [this](pqView*) {
    this->update();
  });
  connect(&pqActiveObjects::instance(), static_cast<void (pqActiveObjects::*)(pqDataRepresentation*)>(&pqActiveObjects::representationChanged), this, [this](pqDataRepresentation*) {
    this->update();
  });

  auto smModel = pqApplicationCore::instance()->getServerManagerModel();
  connect(smModel, &pqServerManagerModel::sourceAdded, this, [this](pqPipelineSource* source) {
    DEBUG_MSG("added source " << source->getSMName().toStdString());
    addSourceRepresentation(source);
  });

  connect(smModel, &pqServerManagerModel::preSourceRemoved, this, [this](pqPipelineSource* source) {
  DEBUG_MSG("preSourceRemoved " << source->getSMName().toStdString());
    auto it = sourceGraphicsItems_.find(source);
    if (it != sourceGraphicsItems_.end()) {
      it->second->aboutToRemoveSource();
    }
  });

  connect(smModel, &pqServerManagerModel::sourceRemoved, this, [this](pqPipelineSource* source) {
    DEBUG_MSG("removed source " << source->getSMName().toStdString());
    removeSourceRepresentation(source);
  });

  connect(smModel, &pqServerManagerModel::connectionAdded, this, [this](pqPipelineSource* source, pqPipelineSource* dest, int sourcePort) {
    DEBUG_MSG("added connection "
              << source->getSMName().toStdString() << " (" << sourcePort << ") -> " << dest->getSMName().toStdString() );
    updateConnectionRepresentations(source, dest);
  });

  connect(smModel, &pqServerManagerModel::connectionRemoved, this, [this](pqPipelineSource* source, pqPipelineSource* dest, int sourcePort) {
    DEBUG_MSG("removed connection "
              << source->getSMName().toStdString() << " (" << sourcePort << ") -> " << dest->getSMName().toStdString() );
    updateConnectionRepresentations(source, dest);
  });

  connect(smModel, &pqServerManagerModel::representationAdded, this, [this](pqRepresentation* rep) {
    DEBUG_MSG("added representation " << rep->getSMName().toStdString());
    if (auto data_repr = dynamic_cast<pqDataRepresentation*>(rep)) {
      if (data_repr->getInput())
        DEBUG_MSG("input " << data_repr->getInput()->getSMName().toStdString());
      if (data_repr->getOutputPortFromInput())
        DEBUG_MSG("port " << data_repr->getOutputPortFromInput()->getPortNumber());
    }
  });

  connect(smModel, &pqServerManagerModel::representationRemoved, this, [this](pqRepresentation* rep) {
    DEBUG_MSG("removed representation " << rep->getSMName().toStdString());
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

  // undo/redo may change node positions
  pqUndoStack* undo_stack = pqApplicationCore::instance()->getUndoStack();
  connect(undo_stack, &pqUndoStack::undone, this, &NetworkEditor::updateSourcePositions);
  connect(undo_stack, &pqUndoStack::redone, this, &NetworkEditor::updateSourcePositions);

  QAction* showSBAction = new QAction(this);
  connect(showSBAction, &QAction::toggled, this, [this](bool) { this->update(); });
  connect(showSBAction, &QAction::changed, this, [this]() { this->update(); });
  new pqScalarBarVisibilityReaction(showSBAction);

  QAction *tempDeleteAction = new QAction(this);
  deleteReaction_ = new pqDeleteReaction(tempDeleteAction);
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
    if (addSourceAtMousePos_) {
      pos = lastMousePos_;
    } else {
      pos.setX(this->itemsBoundingRect().left());
      pos.setY(this->itemsBoundingRect().bottom() + gridSpacing_);
    }
    pos.setX(pos.x() + SourceGraphicsItem::size_.width() / 2.);
    pos.setY(pos.y() +  SourceGraphicsItem::size_.height() / 2.);
  }
  proxy->SetAnnotation("Node.x", std::to_string(pos.x()).c_str());
  proxy->SetAnnotation("Node.y", std::to_string(pos.y()).c_str());
  sourceGraphicsItem->setPos(snapToGrid(pos));

  sourceGraphicsItems_[source] = sourceGraphicsItem;
  this->addItem(sourceGraphicsItem);
  updateSceneSize();

  if (addSourceToSelection_) {
    sourceGraphicsItem->setSelected(true);
  }
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
  updateSceneSize();
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
  if (!filter)
    return;
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

  DEBUG_MSG("Known: ");
  for (const auto& conn : connections) {
    DEBUG_MSG(std::get<0>(conn) << "->" << std::get<1>(conn) << "; ");
  }
  DEBUG_MSG("SM: ");
  for (const auto& conn : sm_connections) {
    DEBUG_MSG(std::get<0>(conn) << "->" << std::get<1>(conn) << "; ");
  }

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
  lastMousePos_ = e->scenePos();

  QMenu menu;
  for (auto& item : items(e->scenePos())) {
    if (auto source = qgraphicsitem_cast<SourceGraphicsItem*>(item)) {
      source->setSelected(true);
      auto editName = menu.addAction(tr("Edit Name"));
      connect(editName, &QAction::triggered, [this, source]() {
        clearSelection();
        source->setSelected(true);
        source->editIdentifier();
      });
      break;
    }
  }

  auto select_all_action = menu.addAction(tr("Select All"));
  connect(select_all_action, &QAction::triggered, this, &NetworkEditor::selectAll);

  QMainWindow* main_window = qobject_cast<QMainWindow*>(pqCoreUtilities::mainWidget());
  QList<QAction*> actions = main_window->menuBar()->actions();
  if (actions.size() > 4) {
    menu.addMenu(actions[3]->menu());
    menu.addMenu(actions[4]->menu());
  }

  menu.addSeparator();

  auto show_action = menu.addAction(tr("Show"));
  connect(show_action, &QAction::triggered, this, &NetworkEditor::showSelected);

  auto hide_action = menu.addAction(tr("Hide"));
  connect(hide_action, &QAction::triggered, this, &NetworkEditor::hideSelected);

  auto show_scalar_bar_action = menu.addAction(tr("Show Color Legend"));
  connect(show_scalar_bar_action, &QAction::triggered, this, &NetworkEditor::showSelectedScalarBars);

  auto hide_scalar_bar_action = menu.addAction(tr("Hide Color Legend"));
  connect(hide_scalar_bar_action, &QAction::triggered, this, &NetworkEditor::hideSelectedScalarBars);

  auto delete_action = menu.addAction(tr("Delete"));
  connect(delete_action, &QAction::triggered, this, &NetworkEditor::deleteSelected);

  menu.addSeparator();
  auto copy = menu.addAction(tr("Copy"));
  connect(copy, &QAction::triggered, this, &NetworkEditor::copy);

  auto paste = menu.addAction(tr("Paste"));
  connect(paste, &QAction::triggered, [this, e]() {
    QPointF pos = e->scenePos();
    this->paste(pos.x(), pos.y());
  });

  if (!menu.isEmpty()) {
    addSourceAtMousePos_ = true;
    menu.exec(QCursor::pos());
    addSourceAtMousePos_ = false;
    e->accept();
  }
}

void NetworkEditor::onSelectionChanged() {
  if (updateSelection_)
    return;

  {
    pqProxySelection selection;
    pqPipelineSource *active_source = nullptr;
    int num_selected = 0;
    for (auto item : this->selectedItems()) {
      auto source_item = qgraphicsitem_cast<SourceGraphicsItem *>(item);
      if (!source_item)
        continue;
      if (auto source = source_item->getSource()) {
        selection.push_back(source);
        if (!active_source)
          active_source = source;
        ++num_selected;
      }
    }

    if (vtkPVNetworkEditorSettings::GetInstance()->GetUpdateActiveObject()) {
      pqActiveObjects::instance().setActiveSource(active_source);
      pqActiveObjects::instance().setSelection(selection, active_source);
    } else {
      pqActiveObjects::instance().setSelection(selection, nullptr);
    }
  }
  {
    auto selection = selectedItems();
    bool contains_source = false;
    for (auto item : selection) {
      if (qgraphicsitem_cast<SourceGraphicsItem*>(item)) {
        contains_source = true;
        break;
      }
    }
    if (contains_source) {
      for (auto item : selection) {
        if (!qgraphicsitem_cast<SourceGraphicsItem*>(item)) {
          item->setSelected(false);
        }
      }
    }
  }
}

QPointF NetworkEditor::snapToGrid(const QPointF& pos) {
  float ox = pos.x() > 0.0f ? 0.5f : -0.5f;
  float oy = pos.y() > 0.0f ? 0.5f : -0.5f;
  float nx = (int(pos.x() / gridSpacing_ + ox)) * gridSpacing_;
  float ny = (int(pos.y() / gridSpacing_ + oy)) * gridSpacing_;
  return {nx, ny};
}

void NetworkEditor::mousePressEvent(QGraphicsSceneMouseEvent* e) {
  lastMousePos_ = e->scenePos();
  activeSourceItem_ = getGraphicsItemAt<SourceGraphicsItem>(e->scenePos());
  QGraphicsScene::mousePressEvent(e);
}

void NetworkEditor::mouseReleaseEvent(QGraphicsSceneMouseEvent* e) {

  lastMousePos_ = e->scenePos();

  // snap selected sources to grid and store position in proxies
  BEGIN_UNDO_SET("Move Nodes");
  for (auto item : this->selectedItems()) {
    if (auto source = qgraphicsitem_cast<SourceGraphicsItem*>(item)) {
      source->setPos(snapToGrid(item->scenePos()));
      source->storePosition();
    }
  }
  END_UNDO_SET();
  if (activeSourceItem_) {
    updateSceneSize();
    activeSourceItem_ = nullptr;
  }
  QGraphicsScene::mouseReleaseEvent(e);
}

void NetworkEditor::mouseMoveEvent(QGraphicsSceneMouseEvent* e) {
  /* if ((e->buttons() & Qt::LeftButton) && activeSourceItem_) {
    updateSceneSize();
  } */
  QGraphicsScene::mouseMoveEvent(e);
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
  // this creates a minimal statefile from the selected items
  // see implementation of vtkSMSessionProxyManager::AddInternalState

  vtkNew<vtkPVXMLElement> state;
  state->SetName("ParaView");

  vtkNew<vtkPVXMLElement> rootElement;
  rootElement->SetName("ServerManagerState");
  std::ostringstream version_string;
  version_string << vtkSMProxyManager::GetVersionMajor() << "."
                 << vtkSMProxyManager::GetVersionMinor() << "."
                 << vtkSMProxyManager::GetVersionPatch();
  rootElement->AddAttribute("version", version_string.str().c_str());

  // add sources
  auto smModel = pqApplicationCore::instance()->getServerManagerModel();
  std::map<std::string, std::vector<std::tuple<std::string, vtkSMProxy*>>> collections;
  for (auto item : this->selectedItems()) {
    auto source_item = qgraphicsitem_cast<SourceGraphicsItem *>(item);
    if (!source_item)
      continue;

    if (auto source = source_item->getSource()) {
      vtkSMProxy *proxy = source->getProxy();
      collections["sources"].emplace_back(std::make_tuple(source->getSMName().toStdString(), proxy));
      proxy->SaveXMLState(rootElement);

      // each pqProxy may have several "helper proxies" that do not appear elsewhere in the pipeline
      std::string helper_group = vtkSMParaViewPipelineController::GetHelperProxyGroupName(proxy);
      auto helper_proxies = source->getHelperProxies();
      for (auto helper : helper_proxies) {
        if (auto helper_proxy = smModel->findItem<pqProxy*>(helper->GetGlobalID())) {
          collections[helper_group].emplace_back(std::make_tuple(helper_proxy->getSMName().toStdString(), helper));
        } else {
          // use id is name if pqProxy not found (seems to be the case most of the time?)
          collections[helper_group].emplace_back(std::make_tuple(std::to_string(helper->GetGlobalID()), helper));
        }
        helper->SaveXMLState(rootElement);
      }

      for (auto view : source->getViews()) {
        for (auto representation : source->getRepresentations(view)) {
          vtkSMProxy *proxy = representation->getProxy();
          collections["representations"].emplace_back(std::make_tuple(representation->getSMName().toStdString(), proxy));
          proxy->SaveXMLState(rootElement);

          std::string helper_group = vtkSMParaViewPipelineController::GetHelperProxyGroupName(proxy);
          auto helper_proxies = representation->getHelperProxies();
          for (auto helper : helper_proxies) {
            if (auto helper_proxy = smModel->findItem<pqProxy*>(helper->GetGlobalID())) {
              collections[helper_group].emplace_back(std::make_tuple(helper_proxy->getSMName().toStdString(), helper));
            } else {
              // use id is name if pqProxy not found (seems to be the case most of the time?)
              collections[helper_group].emplace_back(std::make_tuple(std::to_string(helper->GetGlobalID()), helper));
            }
            helper->SaveXMLState(rootElement);
          }
        }
      }
    }
  }

  // add proxy collections
  for (const auto& kv : collections) {
    vtkNew<vtkPVXMLElement> collectionElement;
    collectionElement->SetName("ProxyCollection");
    collectionElement->AddAttribute("name", kv.first.c_str());
    for (const auto &kv : kv.second) {
      vtkNew<vtkPVXMLElement> itemElement;
      itemElement->SetName("Item");
      itemElement->AddAttribute("id", std::get<1>(kv)->GetGlobalID());
      itemElement->AddAttribute("name", std::get<0>(kv).c_str());
#if   VTK_MAJOR_VERSION > 8 || VTK_MAJOR_VERSION == 8 && VTK_MINOR_VERSION >= 90
      if (std::get<1>(kv)->GetLogName() != nullptr) {
        itemElement->AddAttribute("logname", std::get<1>(kv)->GetLogName());
      }
#endif
      collectionElement->AddNestedElement(itemElement);
    }
    rootElement->AddNestedElement(collectionElement);
  }

  state->AddNestedElement(rootElement);

  std::stringstream ss;
  state->PrintXML(ss, vtkIndent());
  std::string str = ss.str();

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

  float x_min(std::numeric_limits<float>::max()), y_min(std::numeric_limits<float>::max());
  for (int i = 0; i < annotations->GetNumberOfItems(); ++i) {
    auto annotation = vtkPVXMLElement::SafeDownCast(annotations->GetItemAsObject(i));
    if (!annotation)
      continue;
    if (std::string(annotation->GetAttribute("key")) == "Node.x") {
      x_min = std::min(x_min, (float)std::atof(annotation->GetAttribute("value")));
    }
    if (std::string(annotation->GetAttribute("key")) == "Node.y") {
      y_min = std::min(y_min, (float)std::atof(annotation->GetAttribute("value")));
    }
  }

  for (int i = 0; i < annotations->GetNumberOfItems(); ++i) {
    auto annotation = vtkPVXMLElement::SafeDownCast(annotations->GetItemAsObject(i));
    if (!annotation)
      continue;
    if (std::string(annotation->GetAttribute("key")) == "Node.x") {
      float node_x = x - x_min + std::atof(annotation->GetAttribute("value")) + SourceGraphicsItem::size_.width() / 2.;
      annotation->SetAttribute("value", std::to_string(node_x).c_str());
    }
    if (std::string(annotation->GetAttribute("key")) == "Node.y") {
      float node_y = y - y_min + std::atof(annotation->GetAttribute("value")) + SourceGraphicsItem::size_.height() / 2.;
      annotation->SetAttribute("value", std::to_string(node_y).c_str()) ;
    }
  }

  std::unordered_set<std::string> source_names;
  auto sources = utilpq::get_sources();
  for (auto source : sources) {
    source_names.insert(source->getSMName().toStdString());
  }

  auto proxy_collections = vtkSmartPointer<vtkCollection>::New();
  parser->GetRootElement()->GetElementsByName("ProxyCollection", proxy_collections);
  for (int i = 0; i < proxy_collections->GetNumberOfItems(); ++i) {
    auto collection = vtkPVXMLElement::SafeDownCast(proxy_collections->GetItemAsObject(i));
    if (!collection)
      continue;
    if (collection->GetAttributeOrEmpty("name") != std::string("sources"))
      continue;
    for (int j = 0; j < collection->GetNumberOfNestedElements(); ++j) {
      vtkPVXMLElement* child = collection->GetNestedElement(j);
      const char* name = child->GetAttribute("name");
      if (!name)
        continue;
      std::string new_name(name);
      if (new_name.length() <= 0)
        continue;
      while (source_names.count(new_name) > 0) {
        int k = new_name.length();
        while (k > 0 && std::isdigit(new_name[k - 1])) {
          --k;
        }
        int number = std::atoi(new_name.substr(k, std::string::npos).c_str());
        new_name = new_name.substr(0, k) + std::to_string(number + 1);
      }
      source_names.insert(new_name);
      child->SetAttribute("name", new_name.c_str());
    }
  }

  clearSelection();
  addSourceToSelection_ = true;
  updateSelection_ = true;
  auto app = pqApplicationCore::instance();
  auto server = app->getActiveServer();
  vtkNew<vtkPasteStateLoader> loader;
  loader->SetSessionProxyManager(server->proxyManager());
  server->proxyManager()->LoadXMLState(parser->GetRootElement(), loader, false);
  DEBUG_MSG("done pasting");

  auto smModel = app->getServerManagerModel();
  auto pm = server->proxyManager();
  for (auto proxy : loader->representation_proxies) {
    auto repr = smModel->findItem<pqDataRepresentation*>(proxy->GetGlobalID());
    if (!repr)
      continue;
    DEBUG_MSG("Repr " << repr->getSMName().toStdString());
    if (auto input = repr->getInput())
      DEBUG_MSG("Input " << input->getSMName().toStdString());

    auto view = pqActiveObjects::instance().activeView();
    auto view_proxy = view->getViewProxy();

    vtkSMPropertyHelper(view_proxy, "Representations").Add(proxy);
    view_proxy->UpdateVTKObjects();
    repr->setVisible(vtkSMPropertyHelper(proxy, "Visibility").GetAsInt());
  }

  addSourceToSelection_ = false;
  updateSelection_ = false;
  this->onSelectionChanged();
}

void NetworkEditor::paste() {
  paste(lastMousePos_.x(), lastMousePos_.y());
}

void NetworkEditor::updateSceneSize() {
  QRectF sr = this->sceneRect();
  QRectF bounding = getSourcesBoundingRect().adjusted(-50, -50, 50, 50);
  QRectF extended = bounding.united(sr);
  // hack for allowing small scenes to be moved freely within the window
  if (!views().empty() && (extended.width() <= views().front()->width() + 50 && extended.height() <= views().front()->height() + 50))
    setSceneRect(extended);
  else
    setSceneRect(bounding);
}


bool NetworkEditor::empty() const {
  return sourceGraphicsItems_.empty();
}

QRectF NetworkEditor::getSourcesBoundingRect() const {
  QRectF rect;
  for (const auto& item : sourceGraphicsItems_) {
    if (item.second->isVisible()) {
      rect = rect.united(item.second->sceneBoundingRect());
    }
  }
  return rect;
}

void NetworkEditor::initiateConnection(OutputPortGraphicsItem* item) {
  const auto pos = item->mapToScene(item->rect().center());
  // const auto color = item->getPort()->getColorCode();
  const QColor color(44, 123, 182);
  connectionDragHelper_->start(item, pos, color);
}

void NetworkEditor::releaseConnection(InputPortGraphicsItem* item) {
  if (item->getConnections().empty())
    return;
  // remove the old connection and add a new connection curve to be connected.
  auto oldConnection = item->getConnections()[0];
  auto port = oldConnection->getOutportGraphicsItem();
  const auto pos = oldConnection->getEndPoint();
  // const auto color = oldConnection->getOutport()->getColorCode();
  const QColor color(44, 123, 182);
  removeConnection(oldConnection);
  connectionDragHelper_->start(port, pos, color);
}

InputPortGraphicsItem* NetworkEditor::getInputPortGraphicsItemAt(const QPointF pos) const {
  return getGraphicsItemAt<InputPortGraphicsItem>(pos);
}

void NetworkEditor::removeConnection(ConnectionGraphicsItem* connection) {
  auto inport = connection->getInportGraphicsItem()->getPort();
  auto outport = connection->getOutportGraphicsItem()->getPort();
  utilpq::remove_connection(outport.first, outport.second, inport.first, inport.second);
}

void NetworkEditor::deleteSelected() {
  auto items = this->selectedItems();
  this->clearSelection();
  QSet<pqPipelineSource*> delete_sources;
  QSet<ConnectionGraphicsItem*> delete_connections;
  for (QGraphicsItem* item : items) {
    if (auto source = qgraphicsitem_cast<SourceGraphicsItem*>(item)) {
      if (source->getSource()) {
        delete_sources.insert(source->getSource());
      }
    } else if (auto connection = qgraphicsitem_cast<ConnectionGraphicsItem*>(item)) {
      delete_connections.insert(connection);
    }
  }
  if (!delete_connections.empty()) {
    BEGIN_UNDO_SET("Delete Selected Connections");
    for (auto connection : delete_connections) {
      removeConnection(connection);
    }
    END_UNDO_SET();
  }
  if (!delete_sources.empty()) {
    deleteReaction_->deleteSources(delete_sources);
  }
}


void NetworkEditor::showSelected() {
  auto items = this->selectedItems();
  for (QGraphicsItem* item : items) {
    if (auto source = qgraphicsitem_cast<SourceGraphicsItem *>(item)) {
      if (source->getSource()) {
        utilpq::set_source_visiblity(source->getSource(), true);
      }
    }
  }
  pqActiveObjects::instance().activeView()->render();
}

void NetworkEditor::hideSelected() {
  auto items = this->selectedItems();
  for (QGraphicsItem* item : items) {
    if (auto source = qgraphicsitem_cast<SourceGraphicsItem *>(item)) {
      if (source->getSource()) {
        utilpq::set_source_visiblity(source->getSource(), false);
      }
    }
  }
  pqActiveObjects::instance().activeView()->render();
}

void NetworkEditor::showSelectedScalarBars() {
  auto items = this->selectedItems();
  for (QGraphicsItem* item : items) {
    if (auto source = qgraphicsitem_cast<SourceGraphicsItem *>(item)) {
      if (source->getSource()) {
        utilpq::set_source_scalar_bar_visiblity(source->getSource(), true);
      }
    }
  }
  pqActiveObjects::instance().activeView()->render();
  this->update();
}

void NetworkEditor::hideSelectedScalarBars() {
  auto items = this->selectedItems();
  for (QGraphicsItem* item : items) {
    if (auto source = qgraphicsitem_cast<SourceGraphicsItem *>(item)) {
      if (source->getSource()) {
        utilpq::set_source_scalar_bar_visiblity(source->getSource(), false);
      }
    }
  }
  pqActiveObjects::instance().activeView()->render();
  this->update();
}

void NetworkEditor::selectAll() {
  this->clearSelection();
  auto items = this->items();
  for (QGraphicsItem* item : items) {
    if (auto source = qgraphicsitem_cast<SourceGraphicsItem *>(item)) {
      source->setSelected(true);
    }
  }
}

void NetworkEditor::quickLaunch() {
  addSourceAtMousePos_ = true;
  pqPVApplicationCore::instance()->quickLaunch();
  addSourceAtMousePos_ = false;
}

void NetworkEditor::updateSourcePositions() {
  for (const auto& kv : sourceGraphicsItems_) {
    kv.second->loadPosition();
  }
}

void NetworkEditor::computeGraphLayout() {
#ifdef ENABLE_GRAPHVIZ
  std::vector<size_t> nodes;
  std::vector<std::pair<size_t, size_t>> edges;
  std::map<size_t, SourceGraphicsItem*> id_map;

  for (const auto& kv : sourceGraphicsItems_) {
    size_t id = kv.first->getProxy()->GetGlobalID();
    id_map[id] = kv.second;
    nodes.push_back(id);

    for (int i = 0; i < kv.first->getNumberOfOutputPorts(); ++i) {
      auto output = kv.second->getOutputPortGraphicsItem(i);
      if (!output)
        continue;
      for (ConnectionGraphicsItem* connection : output->getConnections()) {
        pqPipelineSource* source = connection->getInportGraphicsItem()->getSourceGraphicsItem()->getSource();
        if (source) {
          size_t dest_id = source->getProxy()->GetGlobalID();
          edges.emplace_back(std::make_pair(dest_id, id));
        }
      }
    }
  }

  std::map<size_t, std::pair<float, float>> layout = compute_graph_layout(nodes, edges);

  BEGIN_UNDO_SET("Graph Layout");
  for (const auto& kv : id_map) {
    if (layout.count(kv.first) < 1)
      continue;
    QPointF pos(layout[kv.first].first, layout[kv.first].second);
    kv.second->setPos(snapToGrid(pos));
    kv.second->storePosition();
  }
  END_UNDO_SET();
#endif
}
