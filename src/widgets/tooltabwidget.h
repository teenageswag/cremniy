#ifndef TOOLTABWIDGET_H
#define TOOLTABWIDGET_H

#include "filetab.h"
#include "utils/syncfiledata.h"
#include <QTabWidget>

class QVBoxLayout;
class QSyntaxStyle;
class QComboBox;
class QCheckBox;
class QSpinBox;
class QCompleter;
class QStyleSyntaxHighlighter;
class QCodeEditor;

class ToolTabWidget : public QTabWidget
{
    Q_OBJECT
public:
    ToolTabWidget(FileTab *fwparent, QString path);

    QCodeEditor* get_codeEditor();
    SyncFileData* m_syncfiledata;

private:

    void loadStyle(QString path, QString name);

    QCodeEditor* m_codeEditor;

    QMap<QString, QCompleter*> m_completers;
    QMap<QString, QStyleSyntaxHighlighter*> m_highlighters;
    QMap<QString, QSyntaxStyle*> m_styles;
};

#endif // TOOLTABWIDGET_H
