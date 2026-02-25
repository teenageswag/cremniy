#include "tooltab.h"
#include <qdir.h>

ToolWidget::ToolWidget(QString path, QWidget* tw)
{
    setDataFromFile(path);
}

void ToolWidget::setDataFromFile(QString path){
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return;
    QByteArray buffer = f.readAll();
    f.close();
    setData(&buffer);
}

void ToolWidget::saveDataToFile(QString path){
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly)) return;
    f.write(getData());
    f.close();
}