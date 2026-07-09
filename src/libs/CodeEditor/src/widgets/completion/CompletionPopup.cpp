#include "widgets/completion/CompletionPopup.h"
#include "widgets/completion/CompletionModel.h"

#include <QListView>
#include <QVBoxLayout>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QFontMetrics>
#include <QScrollBar>
#include <QApplication>
#include <QScreen>

class CompletionItemDelegate : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override
    {
        painter->save();

        bool isSelected = option.state & QStyle::State_Selected;
        if (isSelected) {
            painter->fillRect(option.rect, QColor(45, 45, 45));
        }

        const QString text = index.data(Qt::DisplayRole).toString();
        const QString category = index.data(Qt::UserRole + 1).toString();
        const QString detail = index.data(Qt::UserRole + 3).toString();

        QFont textFont = option.font;
        textFont.setFamily("JetBrains Mono");
        textFont.setPointSize(10);

        QFontMetrics fm(textFont);
        const int padding = 8;
        const int textX = option.rect.left() + padding;
        const int textY = option.rect.top() + (option.rect.height() - fm.height()) / 2 + fm.ascent();

        painter->setFont(textFont);
        painter->setPen(isSelected ? QColor(255, 255, 255) : QColor(212, 212, 212));
        painter->drawText(textX, textY, text);

        // Draw detail on the right side
        if (!detail.isEmpty()) {
            QFont detailFont = textFont;
            detailFont.setPointSize(9);
            painter->setFont(detailFont);
            painter->setPen(QColor(100, 100, 100));
            QRect detailRect = fm.boundingRect(detail);
            const int detailX = option.rect.right() - padding - detailRect.width();
            painter->drawText(detailX, textY, detail);
        }

        // Draw category after text
        if (!category.isEmpty()) {
            QFont catFont = textFont;
            catFont.setPointSize(9);
            painter->setFont(catFont);
            painter->setPen(QColor(120, 120, 120));

            QRect textRect = fm.boundingRect(text);
            const int catX = textX + textRect.width() + 12;
            painter->drawText(catX, textY, category);
        }

        painter->restore();
    }

    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override
    {
        Q_UNUSED(option);
        Q_UNUSED(index);
        return QSize(500, 24);
    }
};

CompletionPopup::CompletionPopup(QWidget* parent)
    : QWidget(parent, Qt::Tool | Qt::FramelessWindowHint | Qt::WindowDoesNotAcceptFocus)
{
    setFixedWidth(500);
    setMaximumHeight(200);
    setAttribute(Qt::WA_ShowWithoutActivating);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_listView = new QListView(this);
    m_listView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_listView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_listView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_listView->setFocusPolicy(Qt::NoFocus);
    m_listView->setFrameShape(QFrame::NoFrame);

    QPalette pal = m_listView->palette();
    pal.setColor(QPalette::Base, QColor(30, 30, 30));
    pal.setColor(QPalette::Text, QColor(212, 212, 212));
    pal.setColor(QPalette::Highlight, QColor(45, 45, 45));
    pal.setColor(QPalette::HighlightedText, QColor(255, 255, 255));
    m_listView->setPalette(pal);

    m_itemDelegate = new CompletionItemDelegate(m_listView);
    m_listView->setItemDelegate(m_itemDelegate);

    layout->addWidget(m_listView);

    connect(m_listView, &QListView::activated, this, &CompletionPopup::onActivated);
}

void CompletionPopup::setModel(CompletionModel* model)
{
    m_model = model;
    m_listView->setModel(model);
}

CompletionModel* CompletionPopup::model() const
{
    return m_model;
}

void CompletionPopup::showNearPoint(const QPoint& viewportPoint, int lineHeight)
{
    updatePosition(viewportPoint, lineHeight);
    show();
    raise();
}

void CompletionPopup::updatePosition(const QPoint& viewportPoint, int lineHeight)
{
    QWidget* editor = parentWidget();
    if (!editor)
        return;

    QPoint globalPos = editor->mapToGlobal(viewportPoint);

    const int popupHeight = qMin(m_listView->model()->rowCount() * 24 + 4, 200);
    QScreen* screen = QApplication::screenAt(globalPos);
    if (!screen)
        screen = QApplication::primaryScreen();
    QRect screenGeom = screen->availableGeometry();

    int x = globalPos.x();
    int y = globalPos.y() + lineHeight;

    if (y + popupHeight > screenGeom.bottom())
        y = globalPos.y() - popupHeight;

    if (x + width() > screenGeom.right())
        x = screenGeom.right() - width();

    if (x < screenGeom.left())
        x = screenGeom.left();

    setGeometry(x, y, width(), popupHeight);
}

int CompletionPopup::selectedIndex() const
{
    return m_listView->currentIndex().row();
}

void CompletionPopup::setSelectedIndex(int index)
{
    if (index >= 0 && index < m_model->filteredCount())
        m_listView->setCurrentIndex(m_model->index(index, 0));
}

void CompletionPopup::selectNext()
{
    int current = m_listView->currentIndex().row();
    int next = current + 1;
    if (next >= m_model->filteredCount())
        next = 0;
    setSelectedIndex(next);
}

void CompletionPopup::selectPrevious()
{
    int current = m_listView->currentIndex().row();
    int prev = current - 1;
    if (prev < 0)
        prev = m_model->filteredCount() - 1;
    setSelectedIndex(prev);
}

bool CompletionPopup::hasSelection() const
{
    return m_listView->currentIndex().isValid();
}

void CompletionPopup::onActivated(const QModelIndex& index)
{
    if (index.isValid()) {
        emit completionAccepted(index.data(Qt::UserRole).toString());
    }
}
