#include "playliststyle.h"
#include <QApplication>
#include <QPainter>

#include <QDebug>

PlaylistStyle::PlaylistStyle(QObject* parent)
    : QStyledItemDelegate { parent }
{
}

// void PlaylistStyle::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
// {
//     return QStyledItemDelegate::paint(painter, option, index);
//     painter->save();
//     QStyleOptionViewItem opt = option;
//     opt.font.setBold(true);
//     opt.features = opt.features | QStyleOptionViewItem::WrapText;
//     QApplication::style()->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, painter);
//     QApplication::style()->drawPrimitive(QStyle::PE_PanelItemViewRow, &opt, painter);
//     QString text = index.data().toString();
//     QString time = index.data(DATA_TIME).toString();
//     bool current = index.data(DATA_CURRENT).toBool();
//     QFontMetrics fm = option.fontMetrics;

//     const QRect& r = option.rect;
//     qDebug() << "Paint" << r << r.size();

//     // QRect textRect = QRect(r.x(), r.y(), r.width(), r.height() / 2);
//     // QRect timeRect = QRect(r.x(), textRect.y() + textRect.height(), r.width(), r.height() - textRect.height());
//     // text = fm.elidedText(text, Qt::ElideRight, textRect.width());
//     // auto fontBold = painter->font();
//     // fontBold.setBold(true);
//     // QTextOption o(Qt::AlignTop | Qt::AlignLeft);

//     // o.setWrapMode(QTextOption::WrapAnywhere);
//     // option.textElideMode = Qt::ElideRight;
//     // painter->drawText(option.rect, Qt::TextWrapAnywhere | Qt::AlignLeft, text);

//     QApplication::style()->drawItemText(painter, option.rect, Qt::AlignLeft | Qt::TextWordWrap, QApplication::palette(), true, text);
//     // painter->setFont(fontBold);
//     // QApplication::style()
//     //     ->drawItemText(
//     //         painter, timeRect, Qt::AlignRight, QApplication::palette(), true, time);

//     painter->restore();
// }

QSize PlaylistStyle::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{

    auto size = QStyledItemDelegate::sizeHint(option, index);
    size.setHeight(option.fontMetrics.height() * 2);

    return size;
}
