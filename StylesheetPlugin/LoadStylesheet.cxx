#include "LoadStylesheet.h"

#include <QApplication>
#include <QFile>
#include <QtDebug>

//-----------------------------------------------------------------------------
LoadStylesheet::LoadStylesheet(QObject* p /*=0*/)
  : QObject(p)
{
}

//-----------------------------------------------------------------------------
LoadStylesheet::~LoadStylesheet()
{
}

//-----------------------------------------------------------------------------
void LoadStylesheet::onStartup()
{
  auto app = dynamic_cast<QApplication*>(QApplication::instance());
  if (app) {
    QFile styleSheetFile(":/resources/dark.qss");
    styleSheetFile.open(QFile::ReadOnly);
    QString styleSheet = QString::fromUtf8(styleSheetFile.readAll());
    app->setStyleSheet(styleSheet);
    styleSheetFile.close();
  }
}

//-----------------------------------------------------------------------------
void LoadStylesheet::onShutdown()
{

}
