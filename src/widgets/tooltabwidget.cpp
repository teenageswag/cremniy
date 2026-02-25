#include "tooltabwidget.h"
#include "QHexView/model/buffer/qmemorybuffer.h"
#include "QHexView/qhexview.h"
#include "filetab.h"
#include "tooltab.h"
#include <QCodeEditor.hpp>
#include <QFile>
#include <QSyntaxStyle.hpp>

#include <QCodeEditor.hpp>
#include <QCECompleter.hpp>
#include <QSyntaxStyle.hpp>
#include <QCXXHighlighter.hpp>
#include <QJSONHighlighter.hpp>
#include <qboxlayout.h>
#include <qfileinfo.h>

ToolTabWidget::ToolTabWidget(FileTab *fwparent, QString path) :
    m_codeEditor(nullptr),
    m_completers(),
    m_highlighters(),
    m_styles()
    {


    m_completers["c"] = new QCECompleter(":/languages/python.xml");
    m_completers["cpp"] = new QCECompleter(":/languages/python.xml");
    m_completers["asm"] = new QCECompleter(":/languages/python.xml");

    m_highlighters["c"] = new QCXXHighlighter;
    m_highlighters["cpp"] = new QCXXHighlighter;
    m_highlighters["asm"] = new QCXXHighlighter;

    m_styles["default"] = QSyntaxStyle::defaultStyle();


    // Code Editor
    QCodeEditor* codeEditorWidget = new QCodeEditor(this);
    codeEditorWidget->setSyntaxStyle(m_styles["default"]);
    QFileInfo fileInfo(path);
    QString ext = fileInfo.suffix();
    codeEditorWidget->setCompleter  (m_completers[ext]);
    codeEditorWidget->setHighlighter(m_highlighters[ext]);

    // Hex View
    QHexView* hexViewWidget = new QHexView();
    QHexDocument* document = QHexDocument::fromMemory<QMemoryBuffer>(data, nullptr);
    hexViewWidget->setDocument(document);

    // Tabs
    ToolWidget* codeEditorTab = new ToolWidget(this, path, codeEditorWidget);
    ToolWidget* hexViewTab = new ToolWidget(this, path, hexViewWidget);
    ToolWidget* DisassemblerTab = new ToolWidget(this, path, new QWidget(this));

    // Tab Icons
    QIcon codeIcon(":/icons/code.png");
    QIcon hexIcon(":/icons/hex.png");
    QIcon disasmIcon(":/icons/dasm.png");

    // Add Tabs
    this->addTab(codeEditorTab, codeIcon, "Code");
    this->addTab(hexViewTab, hexIcon, "Hex");
    this->addTab(DisassemblerTab, disasmIcon, "Disassembler");

}

QCodeEditor* ToolTabWidget::get_codeEditor(){
    return m_codeEditor;
}

void ToolTabWidget::loadStyle(QString path, QString name)
{
    QFile fl(path);

    if (!fl.open(QIODevice::ReadOnly))
    {
        return;
    }

    auto style = new QSyntaxStyle(this);

    if (!style->load(fl.readAll()))
    {
        delete style;
        return;
    }

    m_styles[name] = style;
}
