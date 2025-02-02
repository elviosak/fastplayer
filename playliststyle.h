#pragma once

#include <QStyledItemDelegate>

#define DATA_TIME Qt::UserRole + 1
#define DATA_PATH Qt::UserRole + 2
#define DATA_CURRENT Qt::UserRole + 3

class PlaylistStyle : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit PlaylistStyle(QObject* parent = nullptr);

private:
    // void paint(QPainter* painter,
    //     const QStyleOptionViewItem& option,
    //     const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
};
