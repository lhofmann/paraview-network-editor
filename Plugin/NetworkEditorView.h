#ifndef PARAVIEWNETWORKEDITOR_PLUGIN_NETWORKEDITORVIEW_H_
#define PARAVIEWNETWORKEDITOR_PLUGIN_NETWORKEDITORVIEW_H_

#include <QGraphicsView>

class NetworkEditor;

class NetworkEditorView : public QGraphicsView {
public:
  NetworkEditorView(NetworkEditor* networkEditor, QWidget* parent = nullptr);
  ~NetworkEditorView();

private:
  NetworkEditor* editor_;
};

#endif //PARAVIEWNETWORKEDITOR_PLUGIN_NETWORKEDITORVIEW_H_
