#include "NetworkEditor.h"

void NetworkEditor::constructor()
{
  this->setWindowTitle("Network Editor");
  QWidget* t_widget = new QWidget(this);
  this->setWidget(t_widget);
}
