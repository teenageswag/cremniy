#include "widgets/completion/CompletionModel.h"

CompletionModel::CompletionModel(QObject* parent)
    : QAbstractTableModel(parent)
{
}

int CompletionModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    return m_filteredIndices.size();
}

int CompletionModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    return ColumnCount;
}

QVariant CompletionModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= m_filteredIndices.size())
        return {};

    const CompletionItem& item = m_allItems[m_filteredIndices[index.row()]];

    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        if (index.column() == ColText)
            return item.text;
        if (index.column() == ColCategory)
            return item.category;
    }

    if (role == Qt::UserRole)
        return item.text;

    if (role == Qt::UserRole + 1)
        return item.category;

    if (role == Qt::UserRole + 2)
        return item.snippet;

    if (role == Qt::UserRole + 3)
        return item.detail;

    return {};
}

Qt::ItemFlags CompletionModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

void CompletionModel::setItems(const QVector<CompletionItem>& items)
{
    beginResetModel();
    m_allItems = items;
    rebuildFiltered();
    endResetModel();
}

void CompletionModel::setFilterPrefix(const QString& prefix)
{
    if (m_filterPrefix == prefix)
        return;

    beginResetModel();
    m_filterPrefix = prefix;
    rebuildFiltered();
    endResetModel();
}

void CompletionModel::setCategoryFilter(const QString& category)
{
    if (m_filterCategory == category)
        return;

    beginResetModel();
    m_filterCategory = category;
    rebuildFiltered();
    endResetModel();
}

CompletionItem CompletionModel::itemAt(int filteredRow) const
{
    if (filteredRow < 0 || filteredRow >= m_filteredIndices.size())
        return {};
    return m_allItems[m_filteredIndices[filteredRow]];
}

int CompletionModel::filteredCount() const
{
    return m_filteredIndices.size();
}

QString CompletionModel::currentPrefix() const
{
    return m_filterPrefix;
}

void CompletionModel::rebuildFiltered()
{
    m_filteredIndices.clear();
    m_filteredIndices.reserve(m_allItems.size());

    const QString prefixLower = m_filterPrefix.toLower();

    for (int i = 0; i < m_allItems.size(); ++i) {
        if (!m_filterCategory.isEmpty() && m_allItems[i].category != m_filterCategory)
            continue;
        if (prefixLower.isEmpty() || m_allItems[i].text.toLower().startsWith(prefixLower))
            m_filteredIndices.append(i);
    }

    std::sort(m_filteredIndices.begin(), m_filteredIndices.end(),
        [this](int a, int b) {
            if (m_allItems[a].priority != m_allItems[b].priority)
                return m_allItems[a].priority < m_allItems[b].priority;
            return m_allItems[a].text.toLower() < m_allItems[b].text.toLower();
        });
}
