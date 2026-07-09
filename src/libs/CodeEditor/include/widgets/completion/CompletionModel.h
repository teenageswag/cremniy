#ifndef COMPLETIONMODEL_H
#define COMPLETIONMODEL_H

#include <QAbstractTableModel>
#include <QVector>
#include "CompletionItem.h"

class CompletionModel : public QAbstractTableModel {
    Q_OBJECT

public:
    enum Column { ColText = 0, ColCategory, ColumnCount };

    explicit CompletionModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    void setItems(const QVector<CompletionItem>& items);
    void setFilterPrefix(const QString& prefix);
    void setCategoryFilter(const QString& category);
    CompletionItem itemAt(int filteredRow) const;
    int filteredCount() const;
    QString currentPrefix() const;

private:
    void rebuildFiltered();

    QVector<CompletionItem> m_allItems;
    QVector<int> m_filteredIndices;
    QString m_filterPrefix;
    QString m_filterCategory;
};

#endif // COMPLETIONMODEL_H
