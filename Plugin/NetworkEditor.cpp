#include "NetworkEditor.h"
#include "SourceGraphicsItem.h"
#include "PortGraphicsItem.h"
#include "ConnectionGraphicsItem.h"
#include "OutputPortStatusGraphicsItem.h"
#include "ConnectionDragHelper.h"
#include "vtkPVNetworkEditorSettings.h"
#include "utilpq.h"
#include "vtkPasteStateLoader.h"
#include "vtkPasteProxyLocator.h"
#include "StickyNoteGraphicsItem.h"

#ifdef ENABLE_GRAPHVIZ
# include "graph_layout.h"
#endif

#include <vtkLogger.h>
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
#include <vtkSMProxyLocator.h>
#include <vtkSMProxySelectionModel.h>
#include <vtkSMProxyDefinitionManager.h>
#include <vtkPVProxyDefinitionIterator.h>

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
#include <pqQuickLaunchDialog.h>
#include <pqObjectBuilder.h>
#include <vtkPVConfig.h>

#include <QGraphicsView>
#include <QPainter>
#include <QGraphicsSceneContextMenuEvent>
#include <QMenu>
#include <QMimeData>
#include <QApplication>
#include <QClipboard>
#include <QMenuBar>
#include <QMainWindow>
#include <QScrollBar>

#include <algorithm>
#include <set>
#include <cassert>

namespace ParaViewNetworkEditor {

const int NetworkEditor::gridSpacing_ = 25;

NetworkEditor::NetworkEditor()
    : connectionDragHelper_{new ConnectionDragHelper(*this)} {
  // The default BSP tends to crash...
  setItemIndexMethod(QGraphicsScene::NoIndex);
  setSceneRect(QRectF());

  // add current sources
  auto sources = utilpq::get_sources();
  for (pqPipelineSource *source: sources) {
    addSourceRepresentation(source);
  }
  for (pqPipelineSource *sourceA: sources) {
    for (pqPipelineSource *sourceB: sources) {
      updateConnectionRepresentations(sourceA, sourceB);
    }
  }

  installEventFilter(connectionDragHelper_);

  // only synchronize selection on mouse leave event for peformance
  connect(this, &QGraphicsScene::selectionChanged, this, &NetworkEditor::onSelectionChanged);

  // observe ParaView's pipeline
  connect(&pqActiveObjects::instance(),
          &pqActiveObjects::selectionChanged,
          this,
          [this](const pqProxySelection &selection) {
            vtkLogScopeF(8, "pqActiveObjects::selectionChanged");
            if (updateSelection_)
              return;
            updateSelection_ = true;
            pqProxySelection selection_all = selection;
            for (pqServerManagerModelItem *proxy : selection) {
              auto port = qobject_cast<pqOutputPort *>(proxy);
              if (!port)
                continue;
              selection_all.push_back(port->getSource());
            }
            for (auto const &item : sourceGraphicsItems_) {
              item.second->setSelected(selection_all.contains(item.first));
            }
            updateSelection_ = false;
          });

  connect(&pqActiveObjects::instance(), &pqActiveObjects::portChanged, this, [this](pqOutputPort *) {
    this->update();
  });
  connect(&pqActiveObjects::instance(), &pqActiveObjects::viewChanged, this, [this](pqView *) {
    this->update();
  });
  connect(&pqActiveObjects::instance(),
          static_cast<void (pqActiveObjects::*)(pqDataRepresentation *)>(&pqActiveObjects::representationChanged),
          this,
          [this](pqDataRepresentation *) {
            this->update();
          });

  auto smModel = pqApplicationCore::instance()->getServerManagerModel();
  connect(smModel, &pqServerManagerModel::sourceAdded, this, [this](pqPipelineSource *source) {
    vtkLog(5,   "added source " << source->getSMName().toStdString());
    addSourceRepresentation(source);
  });

  connect(smModel, &pqServerManagerModel::preSourceRemoved, this, [this](pqPipelineSource *source) {
    vtkLog(5,  "preSourceRemoved " << source->getSMName().toStdString());
    auto it = sourceGraphicsItems_.find(source);
    if (it != sourceGraphicsItems_.end()) {
      it->second->aboutToRemoveSource();
    }
  });

  connect(smModel, &pqServerManagerModel::sourceRemoved, this, [this](pqPipelineSource *source) {
    vtkLog(5,  "removed source " << source->getSMName().toStdString());
    removeSourceRepresentation(source);
    utilpq::collect_dummy_source();
  });

  connect(smModel,
          static_cast<void (pqServerManagerModel::*)(pqPipelineSource*, pqPipelineSource*, int)>(&pqServerManagerModel::connectionAdded),
          this,
          [this](pqPipelineSource *source, pqPipelineSource *dest, int sourcePort) {
            vtkLog(5,  "added connection "
                          << source->getSMName().toStdString() << " (" << sourcePort << ") -> "
                          << dest->getSMName().toStdString());
            updateConnectionRepresentations(source, dest);
          });

  connect(smModel,
          static_cast<void (pqServerManagerModel::*)(pqPipelineSource*, pqPipelineSource*, int)>(&pqServerManagerModel::connectionRemoved),
          this,
          [this](pqPipelineSource *source, pqPipelineSource *dest, int sourcePort) {
            vtkLog(5,  "removed connection "
                          << source->getSMName().toStdString() << " (" << sourcePort << ") -> "
                          << dest->getSMName().toStdString());
            updateConnectionRepresentations(source, dest);
          });

  connect(smModel, &pqServerManagerModel::representationAdded, this, [this](pqRepresentation *rep) {
    vtkLog(5,   "added representation " << rep->getSMName().toStdString());
    if (auto data_repr = dynamic_cast<pqDataRepresentation *>(rep)) {
      if (data_repr->getInput())
        vtkLog(5,   "input " << data_repr->getInput()->getSMName().toStdString());
      if (data_repr->getOutputPortFromInput())
        vtkLog(5,   "port " << data_repr->getOutputPortFromInput()->getPortNumber());
    }
  });

  connect(smModel, &pqServerManagerModel::representationRemoved, this, [this](pqRepresentation *rep) {
    vtkLog(5,   "removed representation " << rep->getSMName().toStdString());
  });

  connect(smModel, &pqServerManagerModel::modifiedStateChanged, this, [this](pqServerManagerModelItem *item) {
    auto source = qobject_cast<pqPipelineSource *>(item);
    if (!source)
      return;
    auto it = sourceGraphicsItems_.find(source);
    if (it != sourceGraphicsItems_.end()) {
      it->second->update();
    }
  });

  connect(
      pqApplicationCore::instance(), &pqApplicationCore::stateLoaded,
      this, [this](vtkPVXMLElement* root, vtkSMProxyLocator* locator) {
        auto settings = this->getGlobalOptions();
        if (!settings)
          return;
        std::vector<double> M = vtkSMPropertyHelper(settings, "Transform").GetDoubleArray();
        int sx = vtkSMPropertyHelper(settings, "Scroll").GetAsInt(0);
        int sy = vtkSMPropertyHelper(settings, "Scroll").GetAsInt(1);
        if (M.size() == 9) {
          QTransform transform(M[0], M[1], M[2], M[3], M[4], M[5], M[6], M[7], M[8]);
          this->views().front()->setTransform(transform);
          this->views().front()->verticalScrollBar()->setValue(sx);
          this->views().front()->horizontalScrollBar()->setValue(sy);
        }
      }
  );



  // undo/redo may change node positions
  pqUndoStack *undo_stack = pqApplicationCore::instance()->getUndoStack();
  connect(undo_stack, &pqUndoStack::undone, this, &NetworkEditor::updateSourcePositions);
  connect(undo_stack, &pqUndoStack::redone, this, &NetworkEditor::updateSourcePositions);

  QAction *showSBAction = new QAction(this);
  connect(showSBAction, &QAction::toggled, this, [this](bool) { this->update(); });
  connect(showSBAction, &QAction::changed, this, [this]() { this->update(); });
  new pqScalarBarVisibilityReaction(showSBAction);

  QAction *tempDeleteAction = new QAction(this);
  deleteReaction_ = new pqDeleteReaction(tempDeleteAction);
}

NetworkEditor::~NetworkEditor() = default;

vtkSMProxy* NetworkEditor::getGlobalOptions() {
  vtkSMProxyManager* proxyManager = vtkSMProxyManager::GetProxyManager();
  if (!proxyManager)
    return nullptr;
  vtkSMSessionProxyManager *sessionProxyManager = proxyManager->GetActiveSessionProxyManager();
  if (!sessionProxyManager)
    return nullptr;
  vtkSMProxy* settings = nullptr;
  // settings = sessionProxyManager->FindProxy("networkeditor_global", "networkeditor", "NetworkEditorViewSettings");
  
  // get all settings proxies, in case multiple were instantiated only keep the latest
  vtkNew<vtkCollection> coll;
  sessionProxyManager->GetProxies("networkeditor", "NetworkEditorViewSettings", coll);
  coll->InitTraversal();
  vtkObject* obj = coll->GetNextItemAsObject();
  while (obj != nullptr) {
    settings = vtkSMProxy::SafeDownCast(obj);
    obj = coll->GetNextItemAsObject();
    if (obj) {
      // remove old proxy
      sessionProxyManager->UnRegisterProxy(settings);
    }
  }
  if (!settings) {
    settings = sessionProxyManager->NewProxy("networkeditor", "NetworkEditorViewSettings");
    vtkNew<vtkSMParaViewPipelineController> controller;
    controller->PreInitializeProxy(settings);
    controller->PostInitializeProxy(settings);
    sessionProxyManager->RegisterProxy("networkeditor", "NetworkEditorViewSettings", settings);
    settings->FastDelete();
  }
  return settings;
}

void NetworkEditor::storeTransform(const QTransform& transform, int sx, int sy) {
  std::vector<double> M {
      transform.m11(), transform.m12(), transform.m13(),
      transform.m21(), transform.m22(), transform.m23(),
      transform.m31(), transform.m32(), transform.m33(),
  };
  std::vector<int> s {sx, sy};
  if (M_ == M && s_ == s)
    return;
  M_ = M;
  s_ = s;
  auto settings = this->getGlobalOptions();
  if (!settings)
    return;
  vtkSMPropertyHelper(settings, "Transform").Set(M.data(), 9);
  vtkSMPropertyHelper(settings, "Scroll").Set(s.data(), 2);
}

void NetworkEditor::setBackgroundTransparent(bool transparent) {
  backgroundTransparent_ = transparent;
}

void NetworkEditor::drawBackground(QPainter *painter, const QRectF &rect) {
  painter->save();
  painter->setWorldMatrixEnabled(true);
  if (backgroundTransparent_) {
    painter->fillRect(rect, Qt::transparent);
  } else {
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
  }
  painter->restore();
}

void NetworkEditor::drawForeground(QPainter *painter, const QRectF &rect) {
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

void NetworkEditor::addSourceRepresentation(pqPipelineSource *source) {
  if (std::string(source->getProxy()->GetXMLName()) == "NetworkEditorDummySource")
    return;

  SourceGraphicsItem* sourceGraphicsItem;
  if (std::string(source->getProxy()->GetXMLName()) == "NetworkEditorStickyNote") {
    sourceGraphicsItem = new StickyNoteGraphicsItem(source);
  } else {
    sourceGraphicsItem = new SourceGraphicsItem(source);
  }

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
    pos.setY(pos.y() + SourceGraphicsItem::size_.height() / 2.);
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

void NetworkEditor::removeSourceRepresentation(pqPipelineSource *source) {
  // remove connections that belong to source
  std::vector<pqPipelineSource *> dests, sources;
  for (const auto &kv : connectionGraphicsItems_) {
    if (std::get<0>(kv.first) == source) {
      dests.push_back(std::get<1>(kv.first));
    }
    if (std::get<1>(kv.first) == source) {
      sources.push_back(std::get<0>(kv.first));
    }
  }
  for (auto dest : dests) {
    std::vector<ConnectionGraphicsItem *> connections;
    for (const auto &kv : connectionGraphicsItems_[std::make_tuple(source, dest)]) {
      connections.push_back(kv.second);
    }
    for (auto connection : connections) {
      delete connection;
    }
    connectionGraphicsItems_.erase(std::make_tuple(source, dest));
  }
  for (auto src : sources) {
    std::vector<ConnectionGraphicsItem *> connections;
    for (const auto &kv : connectionGraphicsItems_[std::make_tuple(src, source)]) {
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

void NetworkEditor::updateConnectionRepresentations(pqPipelineSource *source, pqPipelineSource *dest) {
  if ((this->sourceGraphicsItems_.count(source) <= 0) || (this->sourceGraphicsItems_.count(dest) <= 0))
    return;

  std::tuple<pqPipelineSource *, pqPipelineSource *> key(source, dest);
  std::set<std::tuple<int, int>> sm_connections, connections;

  // collect currently known connections
  auto it = connectionGraphicsItems_.find(key);
  if (it == connectionGraphicsItems_.end()) {
    connectionGraphicsItems_[key] = std::map<std::tuple<int, int>, ConnectionGraphicsItem *>();
  } else {
    for (auto const &kv : connectionGraphicsItems_[key]) {
      connections.insert(kv.first);
    }
  }

  // collect connections from ParaView pipeline
  auto smModel = pqApplicationCore::instance()->getServerManagerModel();
  pqPipelineFilter *filter = qobject_cast<pqPipelineFilter *>(dest);
  if (!filter)
    return;
  assert(filter);
  for (int input_id = 0; input_id < filter->getNumberOfInputPorts(); ++input_id) {
    const char *input_name = filter->getInputPortName(input_id).toLocal8Bit().constData();
    auto prop = vtkSMInputProperty::SafeDownCast(filter->getProxy()->GetProperty(input_name));
    assert(prop);

    vtkSMPropertyHelper helper(prop);
    unsigned int num_proxies = helper.GetNumberOfElements();
    for (unsigned int i = 0; i < num_proxies; ++i) {
      auto proxy_source = smModel->findItem<pqPipelineSource *>(helper.GetAsProxy(i));
      if (!proxy_source || proxy_source != source)
        continue;
      if (!proxy_source->getAllConsumers().contains(dest))
        continue;
      int output_id = helper.GetOutputPort(i);
      sm_connections.insert(std::make_tuple(output_id, input_id));
    }
  }

  vtkLog(5,   "Known: ");
  for (const auto &conn : connections) {
    vtkLog(5,   "" << std::get<0>(conn) << "->" << std::get<1>(conn) << "; ");
  }
  vtkLog(5,   "SM: ");
  for (const auto &conn : sm_connections) {
    vtkLog(5,   "" << std::get<0>(conn) << "->" << std::get<1>(conn) << "; ");
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

  for (const auto &conn : removed) {
    if (connectionGraphicsItems_[key].count(conn) < 1) {
      continue;
    }
    auto graphics_item = connectionGraphicsItems_[key][conn];
    connectionGraphicsItems_[key].erase(conn);
    delete graphics_item;
  }

  for (const auto &conn : added) {
    auto outport_graphics = this->sourceGraphicsItems_[source]->getOutputPortGraphicsItem(std::get<0>(conn));
    auto inport_graphics = this->sourceGraphicsItems_[dest]->getInputPortGraphicsItem(std::get<1>(conn));

    if (!inport_graphics || !outport_graphics)
      continue;

    auto connection = new ConnectionGraphicsItem(outport_graphics, inport_graphics);
    this->addItem(connection);
    connectionGraphicsItems_[key][conn] = connection;
  }
}

void NetworkEditor::contextMenuEvent(QGraphicsSceneContextMenuEvent *e) {
  lastMousePos_ = e->scenePos();

  QMenu menu;
  for (auto &item : items(e->scenePos())) {
    if (auto source = qgraphicsitem_cast<SourceGraphicsItem *>(item)) {
      source->setSelected(true);
      if (!qgraphicsitem_cast<StickyNoteGraphicsItem*>(source)) {
        auto editName = menu.addAction(tr("Edit Name"));
        connect(editName, &QAction::triggered, [this, source]() {
          clearSelection();
          source->setSelected(true);
          source->editIdentifier();
        });
      }
      break;
    }
  }

  auto select_all_action = menu.addAction(tr("Select All"));
  connect(select_all_action, &QAction::triggered, this, &NetworkEditor::selectAll);

  QMainWindow *main_window = qobject_cast<QMainWindow *>(pqCoreUtilities::mainWidget());
  QList<QAction *> actions = main_window->menuBar()->actions();
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
    this->paste(pos.x(), pos.y(), false);
  });

  auto paste_with_connections = menu.addAction(tr("Paste with Connections"));
  connect(paste_with_connections, &QAction::triggered, [this, e]() {
    QPointF pos = e->scenePos();
    this->paste(pos.x(), pos.y(), true);
  });

  if (!menu.isEmpty()) {
    addSourceAtMousePos_ = true;
    menu.exec(QCursor::pos());
    addSourceAtMousePos_ = false;
    e->accept();
  }
}

void NetworkEditor::onSelectionChanged() {
  vtkLogScopeFunction(8);
  if (updateSelection_)
    return;

  if (mouseDown_ || !this->views().first()->rubberBandRect().isEmpty())
    return;

  {
    vtkLogScopeF(8, "Synchronize with ParaView selection");

    pqOutputPort* current_active_port = pqActiveObjects::instance().activePort();
    pqPipelineSource* current_active_port_source = nullptr;
    if (current_active_port) {
      current_active_port_source = current_active_port->getSource();
    }
    pqPipelineSource* current_active_source = pqActiveObjects::instance().activeSource();
    pqProxySelection selection;
    pqPipelineSource *active_source = nullptr;
    int num_selected = 0;
    bool has_active_port_source = false;
    for (auto item : this->selectedItems()) {
      auto source_item = qgraphicsitem_cast<SourceGraphicsItem *>(item);
      if (!source_item)
        continue;
      if (auto source = source_item->getSource()) {
        if (source == current_active_port_source) {
          selection.push_back(current_active_port);
          has_active_port_source = true;
        } else {
          selection.push_back(source);
        }
        if ((source == current_active_source) || !active_source)
          active_source = source;
        ++num_selected;
      }
    }
    if (!has_active_port_source) {
      current_active_port = nullptr;
    }

    updateSelection_ = true;
    {
      vtkLogScopeF(8, "Update pqActiveObjects selection");
      if (vtkPVNetworkEditorSettings::GetInstance()->GetUpdateActiveObject()) {
        vtkLogScopeF(8, "pqActiveObjects::setSelection");
        if (current_active_port) {
          pqActiveObjects::instance().setSelection(selection, current_active_port);
        } else {
          pqActiveObjects::instance().setSelection(selection, active_source);
        }
        if (selection.isEmpty()) {
          pqActiveObjects::instance().setActiveSource(nullptr);
        }
      } else {
        pqActiveObjects::instance().setSelection(selection, nullptr);
      }
    }
    updateSelection_ = false;
  }
  {
    vtkLogScopeF(8, "Remove non-source selections");
    auto selection = selectedItems();
    bool contains_source = false;
    for (auto item : selection) {
      if (qgraphicsitem_cast<SourceGraphicsItem *>(item)) {
        contains_source = true;
        break;
      }
    }
    if (contains_source) {
      for (auto item : selection) {
        if (!qgraphicsitem_cast<SourceGraphicsItem *>(item)) {
          item->setSelected(false);
        }
      }
    }
  }
}

QPointF NetworkEditor::snapToGrid(const QPointF &pos) {
  float ox = pos.x() > 0.0f ? 0.5f : -0.5f;
  float oy = pos.y() > 0.0f ? 0.5f : -0.5f;
  float nx = (int(pos.x() / gridSpacing_ + ox)) * gridSpacing_;
  float ny = (int(pos.y() / gridSpacing_ + oy)) * gridSpacing_;
  return {nx, ny};
}

void NetworkEditor::mousePressEvent(QGraphicsSceneMouseEvent *e) {
  mouseDown_ = true;
  lastMousePos_ = e->scenePos();
  activeSourceItem_ = getGraphicsItemAt<SourceGraphicsItem>(e->scenePos());
  QGraphicsScene::mousePressEvent(e);
}

void NetworkEditor::mouseReleaseEvent(QGraphicsSceneMouseEvent *e) {
  vtkLogScopeFunction(8);
  mouseDown_ = false;
  this->onSelectionChanged();
  lastMousePos_ = e->scenePos();

  // snap selected sources to grid and store position in proxies
  BEGIN_UNDO_SET("Move Nodes");
  for (auto item : this->selectedItems()) {
    if (auto source = qgraphicsitem_cast<SourceGraphicsItem *>(item)) {
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

void NetworkEditor::mouseMoveEvent(QGraphicsSceneMouseEvent *e) {
  lastMouseMovePos_ = e->scenePos();
  Qt::KeyboardModifiers modifiers = QGuiApplication::queryKeyboardModifiers();
  if ((modifiers & Qt::ShiftModifier) && (modifiers & Qt::MetaModifier)) {
    QGraphicsSceneHelpEvent help_event;
    help_event.setScenePos(e->scenePos());
    help_event.setScreenPos(e->screenPos());
    this->helpEvent(&help_event);
  }
  /* if ((e->buttons() & Qt::LeftButton) && activeSourceItem_) {
    updateSceneSize();
  } */
  QGraphicsScene::mouseMoveEvent(e);
}

void NetworkEditor::helpEvent(QGraphicsSceneHelpEvent *e) {
  QList<QGraphicsItem *> graphicsItems = items(e->scenePos(), Qt::IntersectsItemShape, Qt::DescendingOrder);
  for (auto item : graphicsItems) {
    if (auto editor_item = dynamic_cast<EditorGraphicsItem*>(item)) {
      editor_item->showToolTip(e);
      return;
    }
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

  auto active_view = pqActiveObjects::instance().activeView();
  vtkSMViewProxy* active_view_proxy = nullptr;
  if (active_view) {
    active_view_proxy = active_view->getViewProxy();
  }
  vtkIdType active_view_id = 0;
  if (active_view_proxy)
    active_view_id = active_view_proxy->GetGlobalID();

  // add sources
  auto smModel = pqApplicationCore::instance()->getServerManagerModel();
  std::map<std::string, std::vector<std::tuple<std::string, vtkSMProxy *>>> collections;
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
        if (auto helper_proxy = smModel->findItem<pqProxy *>(helper->GetGlobalID())) {
          collections[helper_group].emplace_back(std::make_tuple(helper_proxy->getSMName().toStdString(), helper));
        } else {
          // use id is name if pqProxy not found (seems to be the case most of the time?)
          collections[helper_group].emplace_back(std::make_tuple(std::to_string(helper->GetGlobalID()), helper));
        }
        helper->SaveXMLState(rootElement);
      }

      for (auto view : source->getViews()) {
        auto view_proxy = view->getViewProxy();
        vtkIdType view_id = 0;
        if (view_proxy)
          view_id = view_proxy->GetGlobalID();
        for (auto representation : source->getRepresentations(view)) {
          vtkSMProxy *proxy = representation->getProxy();
          proxy->SetAnnotation("ActiveView", (view_id == active_view_id) ? "1" : "0");
          proxy->SetAnnotation("View", view->getSMName().toStdString().c_str());
          collections["representations"].emplace_back(std::make_tuple(representation->getSMName().toStdString(),
                                                                      proxy));
          proxy->SaveXMLState(rootElement);

          std::string helper_group = vtkSMParaViewPipelineController::GetHelperProxyGroupName(proxy);
          auto helper_proxies = representation->getHelperProxies();
          for (auto helper : helper_proxies) {
            if (auto helper_proxy = smModel->findItem<pqProxy *>(helper->GetGlobalID())) {
              collections[helper_group].emplace_back(std::make_tuple(helper_proxy->getSMName().toStdString(), helper));
            } else {
              // use id is name if pqProxy not found (seems to be the case most of the time?)
              collections[helper_group].emplace_back(std::make_tuple(std::to_string(helper->GetGlobalID()), helper));
            }
            helper->SetAnnotation("View", view->getSMName().toStdString().c_str());
            helper->SaveXMLState(rootElement);
          }
        }
      }
    }
  }

  // add proxy collections
  for (const auto &kv : collections) {
    vtkNew<vtkPVXMLElement> collectionElement;
    collectionElement->SetName("ProxyCollection");
    collectionElement->AddAttribute("name", kv.first.c_str());
    for (const auto &kv : kv.second) {
      vtkNew<vtkPVXMLElement> itemElement;
      itemElement->SetName("Item");
      itemElement->AddAttribute("id", std::get<1>(kv)->GetGlobalID());
      itemElement->AddAttribute("name", std::get<0>(kv).c_str());
#if   VTK_MAJOR_VERSION > 8 || (VTK_MAJOR_VERSION == 8 && VTK_MINOR_VERSION >= 90)
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

void NetworkEditor::setPasteMode(int mode_index) {
  switch (mode_index) {
    case 1:
      pasteMode_ = PASTEMODE_ACTIVE_VIEW;
      break;
    case 2:
      pasteMode_ = PASTEMODE_ALL_VIEWS;
      break;
    case 0:
    default:
      pasteMode_ = PASTEMODE_NO_VIEWS;
  }
}

void NetworkEditor::paste(float x, float y, bool keep_connections) {
  auto clipboard = QApplication::clipboard();
  std::string text = clipboard->text().toStdString();
  auto parser = vtkSmartPointer<vtkPVXMLParser>::New();
  if (!parser->Parse(text.c_str())) {
    vtkLog(ERROR, "Encountered exception during parsing clipboard contents:\n" + text);
    return;
  }

  auto annotations = vtkSmartPointer<vtkCollection>::New();
  parser->GetRootElement()->GetElementsByName("Annotation", annotations);

  float x_min(std::numeric_limits<float>::max()), y_min(std::numeric_limits<float>::max());
  for (int i = 0; i < annotations->GetNumberOfItems(); ++i) {
    auto annotation = vtkPVXMLElement::SafeDownCast(annotations->GetItemAsObject(i));
    if (!annotation)
      continue;
    if (std::string(annotation->GetAttribute("key")) == "Node.x") {
      x_min = std::min(x_min, (float) std::atof(annotation->GetAttribute("value")));
    }
    if (std::string(annotation->GetAttribute("key")) == "Node.y") {
      y_min = std::min(y_min, (float) std::atof(annotation->GetAttribute("value")));
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
      annotation->SetAttribute("value", std::to_string(node_y).c_str());
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
    for (unsigned int j = 0; j < collection->GetNumberOfNestedElements(); ++j) {
      vtkPVXMLElement *child = collection->GetNestedElement(j);
      const char *name = child->GetAttribute("name");
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

  auto active_view = pqActiveObjects::instance().activeView();
  vtkSMViewProxy* active_view_proxy = nullptr;
  if (active_view) {
    active_view_proxy = active_view->getViewProxy();
  }
  auto views = utilpq::get_views();

  vtkNew<vtkPasteStateLoader> loader;
  loader->accept_active_view = (pasteMode_ == PASTEMODE_ACTIVE_VIEW);
  if (pasteMode_ == PASTEMODE_ALL_VIEWS) {
    for (auto view : views) {
      loader->accept_views.push_back(view->getSMName().toStdString());
    }
  }

  loader->SetSessionProxyManager(server->proxyManager());
  vtkNew<vtkPasteProxyLocator> locator;
  locator->SetFindExistingSources(keep_connections);
  loader->SetProxyLocator(locator);
  server->proxyManager()->LoadXMLState(parser->GetRootElement(), loader, false);
  vtkLog(5,   "done pasting");

  // TODO: lookup tables are not pasted properly (use vtkSMTransferFunctionManager)

  if (pasteMode_ != PASTEMODE_NO_VIEWS) {
    auto smModel = app->getServerManagerModel();
    for (auto view_rep : loader->representation_proxies) {
      std::string parent_view = std::get<0>(view_rep);
      auto proxy = std::get<1>(view_rep);
      auto repr = smModel->findItem<pqDataRepresentation *>(proxy->GetGlobalID());
      if (!repr)
        continue;

      vtkSMViewProxy* view_proxy = nullptr;
      if (pasteMode_ == PASTEMODE_ACTIVE_VIEW) {
        view_proxy = active_view_proxy;
      } else if (pasteMode_ == PASTEMODE_ALL_VIEWS) {
        auto it = std::find_if(views.begin(), views.end(), [parent_view](pqView* view) {
          return view->getSMName().toStdString() == parent_view;
        });
        if (it != views.end()) {
          view_proxy = (*it)->getViewProxy();
        }
      }
      if (view_proxy) {
        vtkSMPropertyHelper(view_proxy, "Representations").Add(proxy);
        view_proxy->UpdateVTKObjects();
        repr->setVisible(vtkSMPropertyHelper(proxy, "Visibility").GetAsInt());
      }
    }
  }

  addSourceToSelection_ = false;
  updateSelection_ = false;
  this->onSelectionChanged();
}

void NetworkEditor::paste(bool keep_connections) {
  paste(lastMousePos_.x(), lastMousePos_.y(), keep_connections);
}

void NetworkEditor::updateSceneSize() {
  QRectF sr = this->sceneRect();
  QRectF bounding = getSourcesBoundingRect().adjusted(-50, -50, 50, 50);
  QRectF extended = bounding.united(sr);
  // hack for allowing small scenes to be moved freely within the window
  if (!views().empty()
      && (extended.width() <= views().front()->width() + 50 && extended.height() <= views().front()->height() + 50))
    setSceneRect(extended);
  else
    setSceneRect(bounding);
}

bool NetworkEditor::empty() const {
  return sourceGraphicsItems_.empty();
}

QRectF NetworkEditor::getSourcesBoundingRect() const {
  QRectF rect;
  for (const auto &item : sourceGraphicsItems_) {
    if (item.second->isVisible()) {
      rect = rect.united(item.second->sceneBoundingRect());
    }
  }
  return rect;
}

void NetworkEditor::initiateConnection(OutputPortGraphicsItem *item) {
  const auto pos = item->mapToScene(item->rect().center());
  // const auto color = item->getPort()->getColorCode();
  const QColor color(44, 123, 182);
  connectionDragHelper_->start(item, pos, color);
}

void NetworkEditor::releaseConnection(InputPortGraphicsItem *item) {
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

InputPortGraphicsItem *NetworkEditor::getInputPortGraphicsItemAt(const QPointF pos) const {
  return getGraphicsItemAt<InputPortGraphicsItem>(pos);
}

void NetworkEditor::removeConnection(ConnectionGraphicsItem *connection) {
  auto inport = connection->getInportGraphicsItem()->getPort();
  auto outport = connection->getOutportGraphicsItem()->getPort();
  utilpq::remove_connection(outport.first, outport.second, inport.first, inport.second);
}

void NetworkEditor::deleteSelected() {
  auto items = this->selectedItems();
  this->clearSelection();
#if (PARAVIEW_VERSION_MAJOR > 5) || (PARAVIEW_VERSION_MAJOR == 5 && PARAVIEW_VERSION_MINOR >= 9)
  QSet<pqProxy *> delete_sources;
#else
  QSet<pqPipelineSource *> delete_sources;
#endif
  QSet<ConnectionGraphicsItem *> delete_connections;
  for (QGraphicsItem *item : items) {
    if (auto source = qgraphicsitem_cast<SourceGraphicsItem *>(item)) {
      if (source->getSource()) {
        delete_sources.insert(source->getSource());
      }
    } else if (auto connection = qgraphicsitem_cast<ConnectionGraphicsItem *>(item)) {
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
  for (QGraphicsItem *item : items) {
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
  for (QGraphicsItem *item : items) {
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
  for (QGraphicsItem *item : items) {
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
  for (QGraphicsItem *item : items) {
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
  for (QGraphicsItem *item : items) {
    if (auto source = qgraphicsitem_cast<SourceGraphicsItem *>(item)) {
      source->setSelected(true);
    }
  }
}

void NetworkEditor::quickLaunch() {
  // pqPVApplicationCore::instance()->quickLaunch();
  // we build our own dialog instead
  pqQuickLaunchDialog dialog(pqCoreUtilities::mainWidget());
  QList<QAction*> actions;

  // based on pqProxyGroupMenuManager::lookForNewDefinitions
  // and pqProxyGroupMenuManager::getAction

  vtkSMSessionProxyManager* pxm =
      vtkSMProxyManager::GetProxyManager()->GetActiveSessionProxyManager();
  if (!pxm) {
    return;
  }
  vtkSMProxyDefinitionManager* pdmgr = pxm->GetProxyDefinitionManager();
  if (!pdmgr) {
    return;
  }

  // Get the list of selected sources.
  QList<pqOutputPort *> selectedOutputPorts;
  pqServerManagerModel *smmodel = pqApplicationCore::instance()->getServerManagerModel();
  vtkSMProxySelectionModel *selModel = pqActiveObjects::instance().activeSourcesSelectionModel();
  // Determine the list of selected output ports.
  for (unsigned int cc = 0; cc < selModel->GetNumberOfSelectedProxies(); cc++) {
    pqServerManagerModelItem *item =
        smmodel->findItem<pqServerManagerModelItem *>(selModel->GetSelectedProxy(cc));
    pqOutputPort *opPort = qobject_cast<pqOutputPort *>(item);
    pqPipelineSource *source = qobject_cast<pqPipelineSource *>(item);
    if (opPort) {
      selectedOutputPorts.push_back(opPort);
    } else if (source) {
      selectedOutputPorts.push_back(source->getOutputPort(0));
    }
  }

  vtkSmartPointer<vtkPVProxyDefinitionIterator> iter;
  iter.TakeReference(pdmgr->NewIterator());
  iter->AddTraversalGroupName("sources");
  iter->AddTraversalGroupName("filters");
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem()) {
    QString xmlname = iter->GetProxyName();
    QString xmlgroup = iter->GetGroupName();
    QString icon;

    if (vtkPVXMLElement* hints = iter->GetProxyHints()) {
      if (hints->FindNestedElementByName("ReaderFactory") != NULL) {
        continue;
      }
      for (unsigned int cc = 0; cc < hints->GetNumberOfNestedElements(); cc++) {
        vtkPVXMLElement *showInMenu = hints->GetNestedElement(cc);
        if (showInMenu == NULL || showInMenu->GetName() == NULL ||
            strcmp(showInMenu->GetName(), "ShowInMenu") != 0) {
          continue;
        }
        icon = showInMenu->GetAttribute("icon");
      }
    }

    vtkSMProxy *prototype =
        pxm->GetPrototypeProxy(xmlgroup.toLocal8Bit().data(), xmlname.toLocal8Bit().data());
    if (!prototype) {
      continue;
    }
    QString label = prototype->GetXMLLabel() ? prototype->GetXMLLabel() : xmlname;
    QString name_prefix = "";
    if (label.contains("deprecated")) {
      name_prefix = "|";
    }
    if (xmlgroup == "filters") {
      bool can_connect = false;
      for (pqOutputPort* port : selectedOutputPorts) {
        can_connect = can_connect || utilpq::can_connect(
            port->getSource(), port->getPortNumber(),
            xmlgroup.toLocal8Bit().data(), xmlname.toLocal8Bit().data());
      }
      if (!can_connect) {
        name_prefix = "~";
      }
    }
    if (xmlgroup == "sources" && !selectedOutputPorts.empty()) {
      name_prefix = "~";
    }
    auto action = new QAction(name_prefix + label, this);
    action->setObjectName(name_prefix + xmlname + xmlgroup);  // important: pqQuickLaunchDialog uses this as key!
    if (icon.isEmpty() && prototype->IsA("vtkSMCompoundSourceProxy")) {
      icon = ":/pqWidgets/Icons/pqBundle32.png";
    }
    if (!icon.isEmpty()) {
      action->setIcon(QIcon(icon));
    }
    connect(action, &QAction::triggered, [xmlgroup, xmlname, selectedOutputPorts]() {
      vtkLog(5, "Creating " << xmlgroup.toStdString() << "; " << xmlname.toStdString());
      pqServer *server = pqActiveObjects::instance().activeServer();
      pqApplicationCore *core = pqApplicationCore::instance();
      pqObjectBuilder *builder = core->getObjectBuilder();
      vtkSMSessionProxyManager *pxm = server->proxyManager();
      vtkSMProxy *prototype =
          pxm->GetPrototypeProxy(xmlgroup.toLocal8Bit().data(), xmlname.toLocal8Bit().data());
      if (!prototype) {
        vtkLog(ERROR, "Unknown proxy type: " << xmlname.toStdString());
        return;
      }
      BEGIN_UNDO_SET(QString("Create '%1'").arg(xmlname));
      if (xmlgroup == "filters") {
        QList<const char *> inputPortNames = pqPipelineFilter::getInputPorts(prototype);

        auto assign_connections =
            [&](QList<pqOutputPort *> selectedOutputPorts, auto begin, auto end)
            -> QMap<QString, QList<pqOutputPort*>> {
          QMap<QString, QList<pqOutputPort *> > namedInputs;
          for (auto it = begin; it != end; ++it) {
            const char* input_name = *it;
            if (selectedOutputPorts.empty())
              break;
            vtkSMInputProperty *input = vtkSMInputProperty::SafeDownCast(prototype->GetProperty(input_name));
            if (!input)
              continue;
            QList<pqOutputPort *> ports;
            for (pqOutputPort* port : selectedOutputPorts) {
              input->RemoveAllUncheckedProxies();
              input->AddUncheckedInputConnection(port->getSourceProxy(), port->getPortNumber());
              if (input->IsInDomains() > 0) {
                ports.append(port);
                if (!input->GetMultipleInput()) {
                  break;
                }
              }
              input->RemoveAllUncheckedProxies();
            }
            if (!ports.empty()) {
              namedInputs[input_name] = ports;
              for (pqOutputPort* selected_port : ports) {
                selectedOutputPorts.removeAll(selected_port);
              }
            }
          }
          return namedInputs;
        };

        // quick way to find a good assignment between inputs and outpus
        // TODO: find optimal assignment
        auto assignment_forward = assign_connections(selectedOutputPorts, inputPortNames.cbegin(), inputPortNames.cend());
        auto assignment_backward = assign_connections(selectedOutputPorts, inputPortNames.crbegin(), inputPortNames.crend());
        auto count = [](const QMap<QString, QList<pqOutputPort*>>& map) -> int {
          return std::accumulate(
              map.begin(), map.end(), (int)0,
              [](int sum, const QList<pqOutputPort *>& list) -> int {
                return sum + list.size();
              });
        };
        auto named_inputs = count(assignment_forward) >= count(assignment_backward) ? assignment_forward : assignment_backward;

        // ensure first input is non-empty
        if (!inputPortNames.empty() && named_inputs[inputPortNames.first()].empty()) {
          if (pqPipelineSource* dummy_source = utilpq::get_dummy_source()) {
            named_inputs[inputPortNames.first()].push_back(dummy_source->getOutputPort(0));
          }
        }

        builder->createFilter(xmlgroup, xmlname, named_inputs, server);
      } else {
        builder->createSource(xmlgroup, xmlname, server);
      }
      END_UNDO_SET();
    });
    actions.append(action);
  }
  dialog.addActions(actions);

  addSourceAtMousePos_ = true;
  lastMousePos_ = lastMouseMovePos_;
  dialog.exec();
  addSourceAtMousePos_ = false;
  qDeleteAll(actions);
}

void NetworkEditor::updateSourcePositions() {
  for (const auto &kv : sourceGraphicsItems_) {
    kv.second->loadPosition();
  }
}

void NetworkEditor::computeGraphLayout() {
#ifdef ENABLE_GRAPHVIZ
  std::vector<size_t> nodes;
  std::vector<std::pair<size_t, size_t>> edges;
  std::map<size_t, SourceGraphicsItem *> id_map;

  for (const auto &kv : sourceGraphicsItems_) {
    size_t id = kv.first->getProxy()->GetGlobalID();
    id_map[id] = kv.second;
    nodes.push_back(id);

    for (int i = 0; i < kv.first->getNumberOfOutputPorts(); ++i) {
      auto output = kv.second->getOutputPortGraphicsItem(i);
      if (!output)
        continue;
      for (ConnectionGraphicsItem *connection : output->getConnections()) {
        pqPipelineSource *source = connection->getInportGraphicsItem()->getSourceGraphicsItem()->getSource();
        if (source) {
          size_t dest_id = source->getProxy()->GetGlobalID();
          edges.emplace_back(std::make_pair(dest_id, id));
        }
      }
    }
  }

  std::map<size_t, std::pair<float, float>> layout = compute_graph_layout(nodes, edges);

  BEGIN_UNDO_SET("Graph Layout");
  for (const auto &kv : id_map) {
    if (layout.count(kv.first) < 1)
      continue;
    QPointF pos(layout[kv.first].first, layout[kv.first].second);
    kv.second->setPos(snapToGrid(pos));
    kv.second->storePosition();
  }
  END_UNDO_SET();
#endif
}

}
