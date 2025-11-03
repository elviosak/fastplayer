#include "listmodel.h"

#include <QModelIndex>

ListModel::ListModel(QObject* parent)
    : QStandardItemModel(parent)
{
}

ListModel::~ListModel() { }

bool ListModel::moveRows(const QModelIndex& sourceParent, int sourceRow, int count, const QModelIndex& destinationParent, int destinationChild)
{
    int dest = destinationChild;
    if (sourceRow == destinationChild || sourceParent != destinationParent) {
        return false;
    }
    emit playlistMove(sourceRow, destinationChild);
    return false;
}

Qt::ItemFlags ListModel::flags(const QModelIndex& index) const
{
    if (!index.isValid()) {
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemNeverHasChildren | Qt::ItemIsDropEnabled;
    }
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemNeverHasChildren | Qt::ItemIsDragEnabled;
}
