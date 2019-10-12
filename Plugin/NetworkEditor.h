
#include <QDockWidget>

class NetworkEditor : public QDockWidget
{
  Q_OBJECT
  typedef QDockWidget Superclass;

public:
  NetworkEditor(const QString& t, QWidget* p = 0, Qt::WindowFlags f = 0)
    : Superclass(t, p, f)
  {
    this->constructor();
  }
  NetworkEditor(QWidget* p = 0, Qt::WindowFlags f = 0)
    : Superclass(p, f)
  {
    this->constructor();
  }

private:
  void constructor();
};
