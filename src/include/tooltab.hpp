#ifndef TOOLTAB_H
#define TOOLTAB_H

#include <QWidget>

class ToolWidget
{
    Q_OBJECT
public:
    explicit ToolWidget(QString path, QWidget* tw);

    void setDataFromFile(QString path);
    void saveDataToFile(QString path);

    virtual void setBData(const QByteArray& data) = 0;
    virtual QByteArray getBData() = 0;

};

#endif // TOOLTAB_H
