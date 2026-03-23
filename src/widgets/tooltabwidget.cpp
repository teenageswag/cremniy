#include <QFile>
#include <qboxlayout.h>
#include <qfileinfo.h>

#include "tooltabwidget.h"
#include "core/ToolTabFactory.h"
#include "core/ToolTab.h"
#include "utils/globalwidgetsmanager.h"

ToolTabWidget::ToolTabWidget(QWidget *parent, QString path)
    {

    // Tools

    auto& toolFactory = ToolTabFactory::instance();

    qDebug() << "ToolTabWidget constr: for id in avTabs";
    for (const QString& toolID : toolFactory.availableTabs()){
        ToolTab* tab = toolFactory.create(toolID);
        qDebug() << "availableTab: " << tab->toolName();

        tab->setFile(path);
        tab->setTabData();

        connect(tab, &ToolTab::refreshDataAllTabsSignal, this, &ToolTabWidget::refreshDataAllTabs);
        connect(tab, &ToolTab::modifyData, this, &ToolTabWidget::setupStar);
        connect(tab, &ToolTab::dataEqual, this, &ToolTabWidget::removeStar);

        if (tab) this->addTab(tab, tab->toolIcon(), tab->toolName());
    }

    // // - - Connects - -

    // // Trigger: Menu Bar: File->SaveFile or CTRL+S - saveTabData
    connect(GlobalWidgetsManager::instance().get_IDEWindow_menuBar_file_saveFile(),
            &QAction::triggered, this, &ToolTabWidget::saveCurrentTabData);
}

void ToolTabWidget::refreshDataAllTabs(){
    for (int tabIndex = 0; tabIndex < this->count(); tabIndex++){
        if (tabIndex != this->currentIndex()){
            ToolTab* tab = dynamic_cast<ToolTab*>(this->widget(tabIndex));
            tab->setTabData();
        }
    }
}

void ToolTabWidget::saveCurrentTabData(){
    ToolTab* tab = dynamic_cast<ToolTab*>(currentWidget());
    if (tab) tab->saveTabData();
}

void ToolTabWidget::removeStar(){

    qDebug() << "ToolTabWidget: removeStar()";

    // remove star at sender
    QObject* obj = sender();
    QWidget* widget = qobject_cast<QWidget*>(obj);

    if (!widget) return;

    int index = indexOf(widget);
    if (index < 0) return;

    QString text = tabText(index);
    if (text.endsWith('*')) text.chop(1);
    setTabText(index, text);

    int toolCount_WithoutModIndicator = 0;
    for (int tabIndex = 0; tabIndex < this->count(); tabIndex++){
        if (tabIndex != this->currentIndex()){
            ToolTab* tab = dynamic_cast<ToolTab*>(this->widget(tabIndex));
            qDebug() << "ToolTabWidget: removeStar(): " << tab->toolName();
            if (!tab->getModifyIndicator()) {
                qDebug() << "ToolTabWidget: removeStar(): toolCount_WithoutModIndicator++";
                toolCount_WithoutModIndicator++;
            }
        }
    }

    qDebug() << "ToolTabWidget: removeStar(): " << toolCount_WithoutModIndicator << " : " << this->count();

    if (toolCount_WithoutModIndicator == (this->count()-1)) {
        emit removeStarSignal();
        qDebug() << "ToolTabWidget: removeStar(): removeStarSignal";
    }

}

void ToolTabWidget::setupStar(){

    qDebug() << "ToolTabWidget: setupStar()";

    // setup star on tab
    QObject* obj = sender();
    QWidget* widget = qobject_cast<QWidget*>(obj);

    if (!widget) return;

    int index = indexOf(widget);
    if (index < 0) return;

    QString text = tabText(index);
    if (!text.endsWith("*")){
        setTabText(index, text + "*");
    }

    // signal "setup star" to up
    emit setupStarSignal();

}