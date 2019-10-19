#include "NetworkEditorWidget.h"

#include "NetworkEditor.h"
#include "NetworkEditorView.h"

#include "vtkPVNetworkEditorSettings.h"

#include <pqApplicationCore.h>
#include <pqPipelineSource.h>
#include <pqOutputPort.h>
#include <pqServerManagerModel.h>

#include <vtkSMSourceProxy.h>

#include <QAction>
#include <QApplication>
#include <QFile>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QProcessEnvironment>
#include <QToolButton>
#include <QProxyStyle>

#include <iostream>

class MyProxyStyle : public QProxyStyle
{
 public:
  using QProxyStyle::QProxyStyle;

  int styleHint(StyleHint hint, const QStyleOption* option = nullptr, const QWidget* widget = nullptr, QStyleHintReturn* returnData = nullptr) const override
  {
    if (hint == QStyle::SH_ToolTip_WakeUpDelay) {
      return vtkPVNetworkEditorSettings::GetInstance()->GetTooltipWakeupDelay();
    }

    return QProxyStyle::styleHint(hint, option, widget, returnData);
  }
};


void NetworkEditorWidget::constructor()
{
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

  hLayout->addStretch();

  vLayout->addWidget(titleBar);
  vLayout->addWidget(networkEditorView_);
  networkEditorWidget_->setLayout(vLayout);

  if (vtkPVNetworkEditorSettings::GetInstance()->GetSwapOnStartup()) {
    this->swapWithCentralWidget();
  } else {
    this->setWidget(networkEditorWidget_);
    this->setWindowTitle("Network Editor");
  }

}

void NetworkEditorWidget::swapWithCentralWidget() {
  auto main_window = dynamic_cast<QMainWindow *>(this->parent());

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
