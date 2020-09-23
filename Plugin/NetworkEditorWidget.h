#include "NetworkEditor.h"
#include <QDockWidget>
#include <memory>

namespace ParaViewNetworkEditor {
class NetworkEditorView;
}


class NetworkEditorWidget : public QDockWidget {
 Q_OBJECT
  typedef QDockWidget Superclass;

 public:
  NetworkEditorWidget(const QString &t, QWidget *p = nullptr, Qt::WindowFlags f = Qt::WindowFlags())
      : Superclass(t, p, f) {
    this->constructor();
  }
  NetworkEditorWidget(QWidget *p = nullptr, Qt::WindowFlags f = Qt::WindowFlags())
      : Superclass(p, f) {
    this->constructor();
  }

  void swapWithCentralWidget();
 private:
  bool isCentralWidget_ {false};
  std::unique_ptr<ParaViewNetworkEditor::NetworkEditor> networkEditor_;
  ParaViewNetworkEditor::NetworkEditorView *networkEditorView_ {nullptr};
  QWidget *networkEditorWidget_ {nullptr};
  QWidget *renderView_ {nullptr};
  void savePipelineScreenshot(const QString& path) const;

  void constructor();
};

namespace ParaViewNetworkEditor {

class MainWindowEventFilter : public QObject {
 Q_OBJECT

 public:
  MainWindowEventFilter(QWidget *parent) : parent_(parent) {}
  virtual bool eventFilter(QObject *obj, QEvent *event) override;
 private:
  QWidget *parent_;
};

}
