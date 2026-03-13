#ifndef GLOBALWIDGETSMANAGER_H
#define GLOBALWIDGETSMANAGER_H

#include <QObject>
#include <qmenubar.h>

class GlobalWidgetsManager : public QObject {
    Q_OBJECT
public:
    static GlobalWidgetsManager& instance() {
        static GlobalWidgetsManager inst;
        return inst;
    }

    void set_IDEWindow_menuBar_view_wordWrap(QAction* act) { m_IDEWindow_menuBar_view_wordWrap = act; }
    QAction* get_IDEWindow_menuBar_view_wordWrap() const { return m_IDEWindow_menuBar_view_wordWrap; }

signals:
    void actionTriggered(const QString& actionName);

private:
    GlobalWidgetsManager() = default;
    QAction* m_IDEWindow_menuBar_view_wordWrap = nullptr;
};

#endif // GLOBALWIDGETSMANAGER_H
