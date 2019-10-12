#include "NetworkEditor.h"

#include <pqApplicationCore.h>

#include <QMainWindow>
#include <QProcessEnvironment>

#include <iostream>

void NetworkEditor::constructor()
{
  QWidget* t_widget = new QWidget(this);

  bool grab_center_widget = QProcessEnvironment::systemEnvironment().value("NETWORK_EDITOR_DOCK", "0") == "0";
  if (grab_center_widget) {
    auto main_window = dynamic_cast<QMainWindow *>(this->parent());
    if (!main_window) {
      grab_center_widget = false;
    } else {
      this->setWindowTitle("Render View");
      auto render_view = main_window->takeCentralWidget();
      main_window->setCentralWidget(t_widget);
      this->setWidget(render_view);
    }
  }
  if (!grab_center_widget) {
    this->setWindowTitle("Network Editor");
    this->setWidget(t_widget);
  }
}
