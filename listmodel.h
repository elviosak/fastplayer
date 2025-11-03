#pragma once

#include <QStandardItemModel>
#include <QObject>

class ListModel : public QStandardItemModel
{
    Q_OBJECT
public:
    ListModel(QObject* parent = nullptr);
    ~ListModel();

    bool moveRows(const QModelIndex& sourceParent, int sourceRow, int count, const QModelIndex& destinationParent, int destinationChild) override;
    Qt::ItemFlags flags(const QModelIndex & index) const override;

signals:
    void playlistMove(int from, int to);
};
