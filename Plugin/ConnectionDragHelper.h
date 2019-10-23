#ifndef PARAVIEWNETWORKEDITOR_PLUGIN_CONNECTIONDRAGHELPER_H_
#define PARAVIEWNETWORKEDITOR_PLUGIN_CONNECTIONDRAGHELPER_H_

#include <QObject>
#include <QPointF>

#include <memory>

class NetworkEditor;
class ConnectionDragGraphicsItem;
class OutputPortGraphicsItem;
class QEvent;

class ConnectionDragHelper : public QObject {
 Q_OBJECT

 public:
  ConnectionDragHelper(NetworkEditor &editor);
  virtual ~ConnectionDragHelper();

  void start(OutputPortGraphicsItem *outport, QPointF endPoint, QColor color);
  void reset();

  virtual bool eventFilter(QObject *obj, QEvent *event) override;

 private:
  NetworkEditor &editor_;
  std::unique_ptr<ConnectionDragGraphicsItem> connection_;
};

#endif //PARAVIEWNETWORKEDITOR_PLUGIN_CONNECTIONDRAGHELPER_H_
