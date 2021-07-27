#include "NetworkEditorWidget.h"

#include "NetworkEditor.h"
#include "NetworkEditorView.h"
#include "utilqt.h"
#include "vtkPVNetworkEditorSettings.h"

#include <pqSettings.h>
#include <pqFileDialog.h>
#include <pqPVApplicationCore.h>
#include <pqApplicationCore.h>
#include <pqPipelineSource.h>
#include <pqOutputPort.h>
#include <pqServerManagerModel.h>
#include <pqCoreUtilities.h>
#include <pqRecentlyUsedResourcesList.h>
#include <vtkPVXMLElement.h>
#include <vtkSMSourceProxy.h>
#include <vtkLogger.h>

#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QFile>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QProcessEnvironment>
#include <QToolButton>
#include <QProxyStyle>
#include <QToolTip>
#include <QMenuBar>
#include <QKeyEvent>
#ifdef QT_HAS_SVG
# include <QSvgGenerator>
#endif

#include <iostream>

namespace ParaViewNetworkEditor {

class MyProxyStyle : public QProxyStyle {
 public:
  using QProxyStyle::QProxyStyle;

  int styleHint(StyleHint hint,
                const QStyleOption *option = nullptr,
                const QWidget *widget = nullptr,
                QStyleHintReturn *returnData = nullptr) const override {
    if (hint == QStyle::SH_ToolTip_WakeUpDelay) {
      return vtkPVNetworkEditorSettings::GetInstance()->GetTooltipWakeupDelay();
    }

    return QProxyStyle::styleHint(hint, option, widget, returnData);
  }
};

}

void NetworkEditorWidget::constructor()
{
  using namespace ParaViewNetworkEditor;

  isCentralWidget_ = false;
  networkEditor_ = std::make_unique<NetworkEditor>();
  networkEditorView_ = new NetworkEditorView(networkEditor_.get(), this);

  networkEditorView_->setStyle(new MyProxyStyle(networkEditorView_->style()));

  // setup layout
  networkEditorWidget_ = new QWidget(this);
  networkEditorWidget_->setAttribute(Qt::WA_AlwaysShowToolTips, true);
  auto vLayout = new QVBoxLayout(networkEditorWidget_);
  vLayout->setSpacing(1);
  vLayout->setContentsMargins(0, 0, 0, 0);

  // setup tool buttons
  auto titleBar = new QWidget(networkEditorWidget_);
  auto hLayout = new QHBoxLayout(titleBar);
  hLayout->setSpacing(1);
  hLayout->setContentsMargins(0, 0, 0, 0);
  titleBar->setLayout(hLayout);

  auto swap = new QAction("Swap", this);
  connect(swap, &QAction::triggered, this, &NetworkEditorWidget::swapWithCentralWidget);
  auto btnSwap = new QToolButton(titleBar);
  btnSwap->setDefaultAction(swap);
  hLayout->addWidget(btnSwap);

#ifdef ENABLE_GRAPHVIZ
  auto graphLayout = new QAction("Graph Layout", this);
  connect(graphLayout, &QAction::triggered, networkEditor_.get(), &NetworkEditor::computeGraphLayout);
  auto btnGraphLayout = new QToolButton(titleBar);
  btnGraphLayout->setDefaultAction(graphLayout);
  hLayout->addWidget(btnGraphLayout);
#endif

  auto paste_cb = new QComboBox();
  paste_cb->setToolTip("Behavior for pasting representations of copied sources.");
  paste_cb->addItem(QIcon(":/pqWidgets/Icons/pqPaste.svg"), "No view");
  paste_cb->addItem(QIcon(":/pqWidgets/Icons/pqPaste.svg"), "Active view");
  paste_cb->addItem(QIcon(":/pqWidgets/Icons/pqPaste.svg"), "All views");
  connect(paste_cb, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
          networkEditor_.get(), &NetworkEditor::setPasteMode);
  hLayout->addWidget(paste_cb);

  QIcon saveIcon(
      (vtkSMProxyManager::GetVersionMajor() == 5 && vtkSMProxyManager::GetVersionMinor() == 7)
      ? ":/pqWidgets/Icons/pqCaptureScreenshot24.png" : ":/pqWidgets/Icons/pqCaptureScreenshot.svg");
  QAction* saveAction = new QAction(saveIcon, "Save network as image.", this);
  connect(saveAction, &QAction::triggered, this, [this]() {
    this->savePipelineScreenshot("");
  });
  auto btnSave = new QToolButton(titleBar);

  btnSave->setDefaultAction(saveAction);
  hLayout->addWidget(btnSave);

  hLayout->addStretch();

  QString help_text = R"HTML(
  <html><body style="color: #9d9995; background-color: #323235; border-radius: 0.1875em; border: 0.0625em solid #1b1b1d; padding: 0.1875em;">
  <p>Help</p>
  <p>Shortcuts</p>
  <table>
    <tr>
      <th align="left"  valign="middle">Double Click</th>
      <td><b>Background:</b> Fit to view<br/>  <b>Source:</b> Toggle visibility</td>
    </tr>
    <tr>
      <th align="left">Ctrl+A</th>
      <td>Select all</td>
    </tr>
    <tr>
      <th align="left">Ctrl+C</th>
      <td>Copy sources</td>
    </tr>
    <tr>
      <th align="left">Ctrl+V</th>
      <td>Paste sources</td>
    </tr>
    <tr>
      <th align="left">Ctrl+Shift+V</th>
      <td>Paste sources with connections</td>
    </tr>
    <tr>
      <th align="left">Delete</th>
      <td>Delete selected</td>
    </tr>
  </table>
  </body></html>
  )HTML";

  auto help = new QAction("Help", this);
  auto btnHelp = new QToolButton(titleBar);
  connect(help, &QAction::triggered,  [btnHelp, help_text]() {
    QToolTip::showText(btnHelp->mapToGlobal(btnHelp->rect().center()), help_text, btnHelp);
  });
  btnHelp->setDefaultAction(help);
  btnHelp->setToolTip(help_text);
  hLayout->addWidget(btnHelp);

  auto search = new QAction("Search", this);
  search->setShortcutContext(Qt::ShortcutContext::WidgetShortcut);
  search->setShortcut(QKeySequence(Qt::ControlModifier + Qt::Key_Space));
  networkEditorView_->addAction(search);
  connect(search, &QAction::triggered, this, [this]() {
    networkEditor_->quickLaunch();
  });

  auto select_all_action = new QAction("Select All", this);
  select_all_action->setShortcutContext(Qt::ShortcutContext::WidgetShortcut);
  select_all_action->setShortcut(QKeySequence(Qt::ControlModifier + Qt::Key_A));
  connect(select_all_action, &QAction::triggered, networkEditor_.get(), &NetworkEditor::selectAll);
  networkEditorView_->addAction(select_all_action);

  auto delete_action = new QAction("Delete", this);
  delete_action->setShortcutContext(Qt::ShortcutContext::WidgetShortcut);
  delete_action->setShortcut(QKeySequence::Delete);
  connect(delete_action, &QAction::triggered, networkEditor_.get(), &NetworkEditor::deleteSelected);
  networkEditorView_->addAction(delete_action);

  auto copy_action = new QAction("Copy", this);
  copy_action->setShortcutContext(Qt::ShortcutContext::WidgetShortcut);
  copy_action->setShortcut(QKeySequence::Copy);
  connect(copy_action, &QAction::triggered, networkEditor_.get(), &NetworkEditor::copy);
  networkEditorView_->addAction(copy_action);

  auto paste_action = new QAction("Paste", this);
  paste_action->setShortcutContext(Qt::ShortcutContext::WidgetShortcut);
  paste_action->setShortcut(QKeySequence::Paste);
  connect(paste_action, &QAction::triggered, this, [this]() {
    this->networkEditor_.get()->paste(false);
  });
  networkEditorView_->addAction(paste_action);

  auto paste_with_connections_action = new QAction("Paste with Connections", this);
  paste_with_connections_action->setShortcutContext(Qt::ShortcutContext::WidgetShortcut);
  paste_with_connections_action->setShortcut(Qt::ShiftModifier + Qt::ControlModifier + Qt::Key_V);
  connect(paste_with_connections_action, &QAction::triggered, this, [this]() {
    this->networkEditor_.get()->paste(true);
  });
  networkEditorView_->addAction(paste_with_connections_action);


  vLayout->addWidget(titleBar);
  vLayout->addWidget(networkEditorView_);
  networkEditorWidget_->setLayout(vLayout);

  if (vtkPVNetworkEditorSettings::GetInstance()->GetSwapOnStartup()) {
    this->swapWithCentralWidget();
  } else {
    this->setWidget(networkEditorWidget_);
    this->setWindowTitle("Network Editor");
  }

  auto main_window = qobject_cast<QMainWindow*>(pqCoreUtilities::mainWidget());
  main_window->installEventFilter(new MainWindowEventFilter(networkEditorView_));

  connect(&pqApplicationCore::instance()->recentlyUsedResources(), &pqRecentlyUsedResourcesList::changed, this, [this]() {
    if (!vtkPVNetworkEditorSettings::GetInstance()->GetAutoSavePipelineScreenshot())
      return;
    if (pqApplicationCore::instance()->recentlyUsedResources().list().isEmpty())
      return;
    const auto& item = pqApplicationCore::instance()->recentlyUsedResources().list().front();
    if (item.data("PARAVIEW_STATE", "0") == "1") {
      QString path = item.path();
      QFileInfo fileInfo(path);
      path = fileInfo.dir().path() + QDir::separator() + fileInfo.baseName()
             + QString::fromStdString(vtkPVNetworkEditorSettings::GetInstance()->GetAutoSavePipelineSuffix());
      this->savePipelineScreenshot(path);
    }
  });
}

void NetworkEditorWidget::savePipelineScreenshot(QString path) const {
  if (path.isEmpty()) {
    const QString skey = QString("extensions/networkeditor/NetworkEditor");
    // Load the most recently used file extensions from QSettings, if available.
    pqSettings* settings = pqApplicationCore::instance()->settings();
    const QString lastUsedExt = settings->value(skey, ".png").toString();
    pqFileDialog file_dialog(nullptr, pqCoreUtilities::mainWidget(), "Save Network Screenshot", QString(), "PNG (*.png);;SVG (*.svg)");
    file_dialog.setRecentlyUsedExtension(lastUsedExt);
    file_dialog.setObjectName("NetworkEditorFileDialog");
    file_dialog.setFileMode(pqFileDialog::AnyFile);
    if (file_dialog.exec() != QDialog::Accepted) {
      return;
    }
    QString file = file_dialog.getSelectedFiles()[0];
    QFileInfo fileInfo(file);
    settings->setValue(skey, fileInfo.suffix().prepend("*."));
    path = file;
  }

  QFileInfo fileInfo(path);
  bool write_png = (fileInfo.suffix().toLower() != "svg");

  const auto br = this->networkEditor_->getSourcesBoundingRect().adjusted(-50, -50, 50, 50);
  QRectF source = br;
  QRectF target(QPointF(0, 0), br.size() * 2);

  bool transparency = vtkPVNetworkEditorSettings::GetInstance()->GetPipelineScreenshotTransparency();

  if (write_png) {
    QImage image(target.size().toSize(), QImage::Format_ARGB32);
    if (transparency) {
      image.fill(Qt::transparent);
    }
    QPainter painter(&image);
    this->networkEditor_->setBackgroundTransparent(transparency);
    this->networkEditorView_->scene()->render(&painter, target, source);
    this->networkEditor_->setBackgroundTransparent(false);
    image.save(path);
  } else {
#ifdef QT_HAS_SVG
    QSvgGenerator generator;
    generator.setFileName(path);
    generator.setSize(target.size().toSize());
    generator.setViewBox(target);
    generator.setDescription("");
    generator.setTitle("");
    QPainter painter;
    painter.begin(&generator);
    this->networkEditor_->setBackgroundTransparent(transparency);
    this->networkEditorView_->scene()->render(&painter, target, source);
    this->networkEditor_->setBackgroundTransparent(false);
    painter.end();
#else
    vtkLog(ERROR, "Qt SVG support not available.");
#endif
  }
}

void NetworkEditorWidget::swapWithCentralWidget() {
  auto main_window = qobject_cast<QMainWindow*>(pqCoreUtilities::mainWidget());

  auto dock_widgets = main_window->findChildren<QDockWidget*>();
  QList<int> widths, heights;
  for (auto widget : dock_widgets) {
    widths.push_back(widget->width());
    heights.push_back(widget->height());
  }

  QSize size = main_window->centralWidget()->size();
  if (!isCentralWidget_) {
    renderView_ = main_window->takeCentralWidget();
    main_window->setCentralWidget(networkEditorWidget_);
    this->setWidget(renderView_);
    this->setWindowTitle("Render View");
  } else {
    networkEditorWidget_ = main_window->takeCentralWidget();
    main_window->setCentralWidget(renderView_);
    this->setWidget(networkEditorWidget_);
    this->setWindowTitle("Network Editor");
  }

  QApplication::instance()->processEvents();

  main_window->centralWidget()->resize(size);
  main_window->resizeDocks(dock_widgets, heights, Qt::Vertical);
  main_window->resizeDocks(dock_widgets, widths, Qt::Horizontal);

  isCentralWidget_ = !isCentralWidget_;
}

namespace ParaViewNetworkEditor {

bool MainWindowEventFilter::eventFilter(QObject *obj, QEvent *event) {
  if (event->type() == QEvent::ShortcutOverride) {
    QKeyEvent* key_event = static_cast<QKeyEvent*>(event);
    QKeySequence key_sequence(key_event->key() | key_event->modifiers());
    if (parent_->hasFocus()) {
      auto actions = parent_->actions();
      for (QAction* action : actions) {
        if (action->shortcut() == key_sequence) {
          event->accept();
          action->trigger();
          break;
        }
      }
    }
  }
  return false;
}

}
