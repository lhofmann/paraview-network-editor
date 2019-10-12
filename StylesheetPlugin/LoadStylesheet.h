#ifndef LoadStylesheet_h
#define LoadStylesheet_h

#include <QObject>

class LoadStylesheet : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  LoadStylesheet(QObject* p = 0);
  ~LoadStylesheet();

  // Callback for shutdown.
  void onShutdown();

  // Callback for startup.
  void onStartup();

private:
  Q_DISABLE_COPY(LoadStylesheet)
};

#endif
