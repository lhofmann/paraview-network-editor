#include "LoadStylesheet.h"

#include <QApplication>
#include <QFile>
#include <QtDebug>
#include <QPalette>

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
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(53,53,53));
    darkPalette.setColor(QPalette::WindowText, Qt::white);
    darkPalette.setColor(QPalette::Base, QColor(25,25,25));
    darkPalette.setColor(QPalette::AlternateBase, QColor(53,53,53));
    darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::Text, Qt::white);
    darkPalette.setColor(QPalette::Button, QColor(53,53,53));
    darkPalette.setColor(QPalette::ButtonText, Qt::white);
    darkPalette.setColor(QPalette::BrightText, Qt::red);
    darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));

    darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::HighlightedText, Qt::black);

    app->setPalette(darkPalette);

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
