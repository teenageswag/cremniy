#ifndef IDEWINDOW_H
#define IDEWINDOW_H

#include "filestabwidget.h"
#include "filetreeview.h"
#include <QMainWindow>
#include <qboxlayout.h>
#include <qmenubar.h>
#include <qsplitter.h>
#include <qstatusbar.h>

class IDEWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit IDEWindow(QString ProjectPath, QWidget *parent = nullptr);
    ~IDEWindow() override;

private slots:
    void on_treeView_doubleClicked(const QModelIndex &index);

    void on_treeView_clicked(const QModelIndex &index);

    void onTreeContextMenu(const QPoint &pos);

    void onSaveFile();

    void on_menuBar_actionView_wordWrap_clicked();

private:
    QMenuBar* m_menuBar;
    QStatusBar* m_statusBar;

    QMenu* m_fileMenu;
    QMenu* m_editMenu;
    QMenu* m_viewMenu;

    QAction* m_file_newProject;
    QAction* m_file_openProject;
    QAction* m_file_saveFile;

    QAction* m_view_wordWrap;

    QWidget* m_mainWindow;
    QHBoxLayout* m_mainLayout;
    QSplitter* m_mainSplitter;

    FilesTabWidget* m_filesTabWidget;
    FileTreeView* m_filesTreeView;

    void SaveProjectInCache(const QString project_path);
    void openDirectory(const QString &path);
};
#endif // IDEWINDOW_H
