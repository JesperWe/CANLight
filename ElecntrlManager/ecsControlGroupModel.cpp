#include <QIcon>

#include "ecsManager.h"
#include "ecsControlGroup.h"
#include "ecsControlGroupGraphic.h"
#include "ecsControlGroupModel.h"

int ecsControlGroupModel::rowCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent);
	return ecsControlGroups.count();
}

//---------------------------------------------------------------------------

int ecsControlGroupModel::columnCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent);
	return 2;
}

//---------------------------------------------------------------------------

QVariant ecsControlGroupModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();

	if (index.row() >= ecsControlGroups.size())
		return QVariant();

	if (role == Qt::DisplayRole || role == Qt::EditRole) {
		switch( index.column() ) {
		case 0: { return QString::number( ecsControlGroups[ index.row() ]->id ); }
		case 1: { return ecsControlGroups[ index.row() ]->description; }
		case 2: { return QVariant(); }
		}
	}

	if (role == Qt::DecorationRole && index.column() == 1 ) {

		if( this->objectType == "x-application/ecs-appliance-id" ) {
			return QIcon(":/graphics/appliance.svg");
		}
		else {
			return ecsControlGroups.at(index.row())->typeIcon();
		}
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

QVariant ecsControlGroupModel::headerData(int section, Qt::Orientation orientation,
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

Qt::ItemFlags ecsControlGroupModel::flags(const QModelIndex &index) const
{
	Qt::ItemFlags flags = 0;

	if (!index.isValid())
		return Qt::ItemIsEnabled;

	if( index.column() == 0 ) flags |= Qt::ItemIsDragEnabled;
	if( index.column() != 2 ) flags |= Qt::ItemIsEditable;

	return QAbstractItemModel::flags(index) | flags;
}

//---------------------------------------------------------------------------

QModelIndex ecsControlGroupModel:: insertRow() {
	int row = rowCount();
	insertRows( row, 1, QModelIndex() );
	return index( row, 0, QModelIndex() );
}

//---------------------------------------------------------------------------

bool ecsControlGroupModel::setData(const QModelIndex &index,
							 const QVariant &value, int role)
{
	if( ! index.isValid() ) return false;

	if( role == Qt::EditRole) {
		switch( index.column() ) {
		case 0: { ecsControlGroups[index.row()]->id = value.toInt(); break; }
		case 1: { ecsControlGroups[index.row()]->description = value.toString(); break; }
		}

		emit dataChanged(index, index);
		emit modified();
		return true;
	}

	if( role == Qt::UserRole) {

		if( value.toInt() == -1) ecsControlGroups[index.row()]->toggleItemType();
		else ecsControlGroups[index.row()]->itemType = value.toInt();

		emit dataChanged(index, index);
		emit modified();
		return true;
	}

	return false;
}

//---------------------------------------------------------------------------

bool ecsControlGroupModel::insertRows(int position, int rows, const QModelIndex &parent)
{
	Q_UNUSED(parent);
	ecsControlGroup* newItem;
	int maxId = 0;

	for( int i=0; i<ecsControlGroups.count(); i++ )  {
		if( ecsControlGroups[i]->id > maxId ) maxId = ecsControlGroups[i]->id;
	}

	beginInsertRows( QModelIndex(), position, position+rows-1 );

	for( int row = 0; row < rows; ++row ) {
		newItem = new ecsControlGroup();
		newItem->id = ++maxId;
		newItem->graphic = new ecsControlGroupGraphic( newItem );
		newItem->graphic->recalcBoxSize();
		ecsControlGroups.append(  newItem );
	}

	endInsertRows();

	return true;
}

//---------------------------------------------------------------------------

bool ecsControlGroupModel::removeRows(int position, int rows, const QModelIndex &parent)
{
	Q_UNUSED(parent);

	beginRemoveRows(QModelIndex(), position, position+rows-1);
	for (int row = 0; row < rows; ++row) {
		ecsControlGroups.removeAt(position);
	}
	endRemoveRows();
	emit modified();
	return true;
}

//---------------------------------------------------------------------------

void ecsControlGroupModel::sort( int column, Qt::SortOrder order ) {
	beginResetModel();
	if( order == Qt::AscendingOrder ) {
		switch( column ) {
		case 0: qSort(ecsControlGroups.begin(), ecsControlGroups.end(), ecsControlGroup::compareIdsAsc ); break;
		case 1: qSort(ecsControlGroups.begin(), ecsControlGroups.end(), ecsControlGroup::compareDscrAsc ); break;
		}
	}

	if( order == Qt::DescendingOrder ) {
		switch( column ) {
		case 0: qSort(ecsControlGroups.begin(), ecsControlGroups.end(), ecsControlGroup::compareIdsDesc ); break;
		case 1: qSort(ecsControlGroups.begin(), ecsControlGroups.end(), ecsControlGroup::compareDscrDesc ); break;
		}
	}
	endResetModel();
	emit modified();
}

//---------------------------------------------------------------------------

QMimeData *ecsControlGroupModel::mimeData(const QModelIndexList &indexes) const
{
	QMimeData *mimeData = new QMimeData();

	mimeData->setData( objectType, data(indexes[0], Qt::DisplayRole).toByteArray());
	return mimeData;
}

//---------------------------------------------------------------------------

void ecsControlGroupModel::updateComplete()
{
	emit modified();
}

//---------------------------------------------------------------------------

ecsControlGroup* ecsControlGroupModel::findItem( int id )
{
	for( int i = 0; i < ecsControlGroups.count(); i++ ) {
		if( ecsControlGroups[i]->id == id ) return ecsControlGroups[i];
	}
	return NULL;
}

//---------------------------------------------------------------------------

int ecsControlGroupModel::findItemIndex( int id )
{
	for( int i = 0; i < ecsControlGroups.count(); i++ ) {
		if( ecsControlGroups[i]->id == id ) return i;
	}
	return NULL;
}

//---------------------------------------------------------------------------

void ecsControlGroupModel::clear()
{
	beginResetModel();
	ecsControlGroups.clear();
	endResetModel();
}
