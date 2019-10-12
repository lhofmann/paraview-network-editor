#include "NetworkEditorWidget.h"

#include "NetworkEditor.h"
#include "NetworkEditorView.h"

#include <pqApplicationCore.h>
#include <QApplication>
#include <QFile>
#include <QMainWindow>
#include <QProcessEnvironment>

#include <iostream>

void NetworkEditorWidget::constructor()
{
  auto app = dynamic_cast<QApplication*>(QApplication::instance());
  if (app) {
    QFile styleSheetFile(":/resources/dark.qss");
    styleSheetFile.open(QFile::ReadOnly);
    QString styleSheet = QString::fromUtf8(styleSheetFile.readAll());
    app->setStyleSheet(styleSheet);
    styleSheetFile.close();
  }

  networkEditor_ = std::make_unique<NetworkEditor>();
  networkEditorView_ = new NetworkEditorView(networkEditor_.get(), this);

  bool grab_center_widget = QProcessEnvironment::systemEnvironment().value("NETWORK_EDITOR_DOCK", "0") == "0";
  if (grab_center_widget) {
    auto main_window = dynamic_cast<QMainWindow *>(this->parent());
    if (!main_window) {
      grab_center_widget = false;
    } else {
      this->setWindowTitle("Render View");
      auto render_view = main_window->takeCentralWidget();
      main_window->setCentralWidget(networkEditorView_);
      this->setWidget(render_view);
    }
  }
  if (!grab_center_widget) {
    this->setWindowTitle("Network Editor");
    this->setWidget(networkEditorView_);
  }
}
