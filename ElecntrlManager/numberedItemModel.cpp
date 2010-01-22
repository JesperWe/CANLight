#include "numberedItem.h"
#include "numberedItemModel.h"

int NumberedItemModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return numberedItemData.count();
}

//---------------------------------------------------------------------------

int NumberedItemModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 2;
}

//---------------------------------------------------------------------------

QVariant NumberedItemModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (index.row() >= numberedItemData.size())
        return QVariant();

    if (role == Qt::DisplayRole || role == Qt::EditRole) {

        switch( index.column() ) {
        case 0: { return QString::number(numberedItemData.at(index.row()).id); }
        case 1: { return numberedItemData.at(index.row()).description; }
        case 2: { return QVariant(); }
        }
    }

    if (role == Qt::DecorationRole && index.column() == 1 ) {
        return numberedItemData.at(index.row()).typeIcon();
    }

    if (role == Qt::TextAlignmentRole) {
        switch( index.column() ) {
        case 0: { return (QVariant) ( Qt::AlignCenter | Qt::AlignVCenter ); }
        case 1: { return (QVariant) ( Qt::AlignLeft | Qt::AlignVCenter ); }
        case 2: { return (QVariant) ( Qt::AlignCenter | Qt::AlignVCenter ); }
        }
    }

    // We use UserRole to determine what type of objects are represented by this model.

    if (role == Qt::UserRole) {
        return (QVariant) ( objectType );
    }

    else
        return QVariant();
}

//---------------------------------------------------------------------------

QVariant NumberedItemModel::headerData(int section, Qt::Orientation orientation,
                                    int role) const
{
    QString header;

    if( role == Qt::TextAlignmentRole && orientation == Qt::Horizontal ) {

        switch( section ) {
        case 0: { return Qt::AlignHCenter; }
        case 1: { return Qt::AlignLeft; }
        case 2: { return Qt::AlignHCenter; }
        }
    }

    if (role != Qt::DisplayRole)
        return QVariant();

    if (orientation == Qt::Horizontal) {
        switch( section ) {
        case 0: { header = tr("ID"); break; }
        case 1: { header = tr("Description"); break; }
        case 2: { header = tr("Type"); break; }
        }

        return header;
    }
    else {
        return QVariant();
        //return QString("%1").arg(section);
    }
}

//---------------------------------------------------------------------------

Qt::ItemFlags NumberedItemModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags flags = 0;

    if (!index.isValid())
        return Qt::ItemIsEnabled;

    if( index.column() == 0 ) flags |= Qt::ItemIsDragEnabled;
    if( index.column() != 2 ) flags |= Qt::ItemIsEditable;

    return QAbstractItemModel::flags(index) | flags;
}

//---------------------------------------------------------------------------

bool NumberedItemModel::setData(const QModelIndex &index,
                             const QVariant &value, int role)
{
    if( ! index.isValid() ) return false;

    if( role == Qt::EditRole) {
        switch( index.column() ) {
        case 0: { numberedItemData[index.row()].id = value.toInt(); break; }
        case 1: { numberedItemData[index.row()].description = value.toString(); break; }
        }

        emit dataChanged(index, index);
        emit modified();
        return true;
    }

    if( role == Qt::UserRole) {
        numberedItemData[index.row()].itemType = 3 - numberedItemData[index.row()].itemType;
        emit dataChanged(index, index);
        emit modified();
        return true;
    }

    return false;
}

//---------------------------------------------------------------------------

bool NumberedItemModel::insertRows(int position, int rows, const QModelIndex &parent)
{
    Q_UNUSED(parent);

    int maxId = 0;
    for( int i=0; i<numberedItemData.count(); i++ )
        if( numberedItemData[i].id > maxId ) maxId = numberedItemData[i].id;

    beginInsertRows( QModelIndex(), position, position+rows-1 );

    for( int row = 0; row < rows; ++row ) {
        numberedItemData.append(NumberedItem());
        numberedItemData.last().id = ++maxId;
    }

    endInsertRows();
    return true;
}

//---------------------------------------------------------------------------

bool NumberedItemModel::removeRows(int position, int rows, const QModelIndex &parent)
{
    Q_UNUSED(parent);

    beginRemoveRows(QModelIndex(), position, position+rows-1);
    for (int row = 0; row < rows; ++row) {
        numberedItemData.removeAt(position);
    }
    endRemoveRows();
    emit modified();
    return true;
}

//---------------------------------------------------------------------------

void NumberedItemModel::sort( int column, Qt::SortOrder order ) {
    beginResetModel();
    if( order == Qt::AscendingOrder ) {
        switch( column ) {
        case 0: qSort(numberedItemData.begin(), numberedItemData.end(), NumberedItem::compareIdsAsc ); break;
        case 1: qSort(numberedItemData.begin(), numberedItemData.end(), NumberedItem::compareDscrAsc ); break;
        }
    }

    if( order == Qt::DescendingOrder ) {
        switch( column ) {
        case 0: qSort(numberedItemData.begin(), numberedItemData.end(), NumberedItem::compareIdsDesc ); break;
        case 1: qSort(numberedItemData.begin(), numberedItemData.end(), NumberedItem::compareDscrDesc ); break;
        }
    }
    endResetModel();
    emit modified();
}

//---------------------------------------------------------------------------

QMimeData *NumberedItemModel::mimeData(const QModelIndexList &indexes) const
{
    QMimeData *mimeData = new QMimeData();

    mimeData->setData( objectType, data(indexes[0], Qt::DisplayRole).toByteArray());
    return mimeData;
}

//---------------------------------------------------------------------------

float NumberedItemModel::accumulatedOffset( int itemIndex )
{
    float myOffset = 0;
    if( itemIndex == 0 ) {
        numberedItemData[0].offset = 0;
        return 0;
    }

    myOffset = numberedItemData[itemIndex-1].offset + calculateHeight(itemIndex-1);
    numberedItemData[itemIndex].offset = myOffset;
    return myOffset;
}

//---------------------------------------------------------------------------

float NumberedItemModel::calculateHeight( int itemIndex )
{
    float myHeight = 0;

    myHeight = 55 + numberedItemData[itemIndex].links.count()*16;

    // Try height based on number of events, use it if it is larger.

    float myHeight2 = 40 + numberedItemData[itemIndex].events.count()*60;

    if( myHeight2 > myHeight ) {
        myHeight = myHeight2;
    }

    return myHeight;
}

//---------------------------------------------------------------------------

void NumberedItemModel::setItemOffset( int itemIndex, float newOffset )
{
    numberedItemData[itemIndex].offset = newOffset;
}

//---------------------------------------------------------------------------

void NumberedItemModel::updateComplete()
{
    emit modified();
}

//---------------------------------------------------------------------------

NumberedItem* NumberedItemModel::findItem( int id )
{
    for( int i = 0; i < numberedItemData.count(); i++ ) {
        if( numberedItemData[i].id == id ) return &numberedItemData[i];
    }
    return NULL;
}

//---------------------------------------------------------------------------

void NumberedItemModel::clear()
{
    beginResetModel();
    numberedItemData.clear();
    endResetModel();
}
