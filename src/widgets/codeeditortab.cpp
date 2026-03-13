#include "codeeditortab.h"
#include "QCodeEditor.hpp"
#include <qboxlayout.h>
#include <qfileinfo.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qstackedlayout.h>
#include "globalwidgetsmanager.h"

CodeEditorTab::CodeEditorTab(QWidget *parent, QString path)
    : QWidget{parent}
{

    QFileInfo fileInfo(path);
    QString ext = fileInfo.suffix();

    m_codeEditorWidget = new QCodeEditor(ext, this);
    m_codeEditorWidget->setTabReplace(false);

    QTextOption opt = m_codeEditorWidget->document()->defaultTextOption();
    opt.setTabStopDistance(20);
    m_codeEditorWidget->document()->setDefaultTextOption(opt);

    m_codeEditorWidget->document()->markContentsDirty(0, m_codeEditorWidget->document()->characterCount());
    m_codeEditorWidget->viewport()->update();

    m_overlayWidget = new QWidget(this);

    auto overlayLayout = new QVBoxLayout(m_overlayWidget);
    overlayLayout->setAlignment(Qt::AlignCenter);

    QLabel* title = new QLabel("Binary file detected");
    title->setStyleSheet("color: white; font-size: 20px;");
    title->setAlignment(Qt::AlignCenter);
    title->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    overlayLayout->addWidget(title);
    overlayLayout->addSpacing(15);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->setAlignment(Qt::AlignCenter);

    QPushButton* openHexBtn = new QPushButton("Open in Hex Viewer");
    QPushButton* anywayOpenBtn = new QPushButton("Open anyway");

    openHexBtn->setStyleSheet(""
                              "QPushButton { border: 1px solid #2c7c32; } "
                              "QPushButton:hover { background-color: #163318; border: 1px solid #2c7c32; } "
                              "QPushButton:pressed { background-color: #163318; border: 2px solid #2c7c32; font-weight: bold; } "
                              );

    btnLayout->addWidget(openHexBtn);
    btnLayout->addWidget(anywayOpenBtn);
    overlayLayout->addLayout(btnLayout);

    auto stack = new QStackedLayout;
    stack->setStackingMode(QStackedLayout::StackAll);
    stack->addWidget(m_codeEditorWidget);
    stack->addWidget(m_overlayWidget);

    m_overlayWidget->hide();

    setLayout(stack);

    connect(GlobalWidgetsManager::instance().get_IDEWindow_menuBar_view_wordWrap(),
            &QAction::changed,
            this, [this]{
                if (GlobalWidgetsManager::instance().get_IDEWindow_menuBar_view_wordWrap()->isChecked())
                    m_codeEditorWidget->setWordWrapMode(QTextOption::WordWrap);
                else
                    m_codeEditorWidget->setWordWrapMode(QTextOption::NoWrap);
            });

    connect(openHexBtn, &QPushButton::clicked, this, [this]{
        emit setHexViewTab();
    });

    connect(anywayOpenBtn, &QPushButton::clicked, this, [this]{
        forceSetData = true;
        emit askData();
    });

    connect(m_codeEditorWidget->document(),
            &QTextDocument::modificationChanged,
            this,
            [this](bool modified){
                if (!m_codeEditorWidget->m_ignoreModification)
                    emit modifyData(true);
            });

    connect(m_codeEditorWidget->document(),
            &QTextDocument::contentsChanged,
            this,
            [this](){
                QByteArray data = m_codeEditorWidget->getBData();
                uint newDataHash = qHash(data, 0);
                if (dataHash == newDataHash) {
                    emit dataEqual();
                }
                else{
                    if (!m_codeEditorWidget->m_ignoreModification)
                        emit modifyData(true);
                }
            });

}