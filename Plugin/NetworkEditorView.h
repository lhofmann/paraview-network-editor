#ifndef PARAVIEWNETWORKEDITOR_PLUGIN_NETWORKEDITORVIEW_H_
#define PARAVIEWNETWORKEDITOR_PLUGIN_NETWORKEDITORVIEW_H_

#include <QGraphicsView>

class NetworkEditor;

class NetworkEditorView : public QGraphicsView {
 public:
  NetworkEditorView(NetworkEditor* networkEditor, QWidget* parent = nullptr);
  ~NetworkEditorView();

 protected:
  void wheelEvent(QWheelEvent* e) override;

 private:
  NetworkEditor* editor_;
  void zoom(double dz);
};

#endif //PARAVIEWNETWORKEDITOR_PLUGIN_NETWORKEDITORVIEW_H_
