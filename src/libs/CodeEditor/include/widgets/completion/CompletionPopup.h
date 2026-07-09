#ifndef COMPLETIONPOPUP_H
#define COMPLETIONPOPUP_H

#include <QWidget>
#include <QModelIndex>

class QListView;
class QStyledItemDelegate;
class CompletionModel;

class CompletionPopup : public QWidget {
    Q_OBJECT

public:
    explicit CompletionPopup(QWidget* parent = nullptr);

    void setModel(CompletionModel* model);
    CompletionModel* model() const;

    void showNearPoint(const QPoint& viewportPoint, int lineHeight);
    void updatePosition(const QPoint& viewportPoint, int lineHeight);

    int selectedIndex() const;
    void setSelectedIndex(int index);
    void selectNext();
    void selectPrevious();
    bool hasSelection() const;

signals:
    void completionAccepted(const QString& text);
    void completionCancelled();

private slots:
    void onActivated(const QModelIndex& index);

private:
    QListView* m_listView;
    QStyledItemDelegate* m_itemDelegate;
    CompletionModel* m_model;
};

#endif // COMPLETIONPOPUP_H
