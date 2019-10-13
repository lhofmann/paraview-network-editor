#include "NetworkEditor.h"
#include <QDockWidget>
#include <memory>

class NetworkEditorView;

class NetworkEditorWidget : public QDockWidget
{
  Q_OBJECT
  typedef QDockWidget Superclass;

public:
  NetworkEditorWidget(const QString& t, QWidget* p = 0, Qt::WindowFlags f = 0)
    : Superclass(t, p, f)
  {
    this->constructor();
  }
  NetworkEditorWidget(QWidget* p = 0, Qt::WindowFlags f = 0)
    : Superclass(p, f)
  {
    this->constructor();
  }

  void swapWithCentralWidget();

private:
  bool isCentralWidget_;
  std::unique_ptr<NetworkEditor> networkEditor_;
  NetworkEditorView* networkEditorView_;
  QWidget* networkEditorWidget_;
  QWidget* renderView_;

  void constructor();
};
