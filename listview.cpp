#include "listview.h"

ListView::ListView(QWidget* parent)
    : QListView(parent)
{
    setSelectionMode(QAbstractItemView::SingleSelection);
    setDragEnabled(true);
    viewport()->setAcceptDrops(true);
    setDragDropMode(QAbstractItemView::InternalMove);
    setDropIndicatorShown(true);
}
ListView::~ListView()
{
}
