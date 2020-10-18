#ifndef PARAVIEWNETWORKEDITOR_PLUGIN_NETWORKEDITORVIEW_H_
#define PARAVIEWNETWORKEDITOR_PLUGIN_NETWORKEDITORVIEW_H_

#include <QGraphicsView>

namespace ParaViewNetworkEditor {

class NetworkEditor;

class NetworkEditorView : public QGraphicsView {
 public:
  NetworkEditorView(NetworkEditor *networkEditor, QWidget *parent = nullptr);
  ~NetworkEditorView();

  void fitNetwork();

 protected:
  bool viewportEvent(QEvent *event) override;
  void wheelEvent(QWheelEvent *e) override;
  virtual void keyPressEvent(QKeyEvent *keyEvent) override;
  virtual void keyReleaseEvent(QKeyEvent *keyEvent) override;
  virtual void focusOutEvent(QFocusEvent *) override;
  virtual void mouseDoubleClickEvent(QMouseEvent *e) override;

 private:
  NetworkEditor *editor_;
  void zoom(double dz);
};

}

#endif //PARAVIEWNETWORKEDITOR_PLUGIN_NETWORKEDITORVIEW_H_
