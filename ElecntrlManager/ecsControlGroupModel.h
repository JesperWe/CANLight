#ifndef ecsControlGroupMODEL_H
#define ecsControlGroupMODEL_H

#include <QStringList>
#include <QModelIndex>
#include <QMimeData>
#include <QAbstractTableModel>

#include <ecsControlGroup.h>

class ecsControlGroupModel : public QAbstractTableModel
{
	Q_OBJECT

public:
	ecsControlGroupModel() {};

	ecsControlGroupModel( QObject *parent = 0 )
		: QAbstractTableModel(parent) {};

	int rowCount(const QModelIndex &parent = QModelIndex()) const;
	int columnCount(const QModelIndex &parent = QModelIndex()) const;

	QVariant data(const QModelIndex &index, int role) const;

	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

	Qt::ItemFlags flags(const QModelIndex &index) const;

	bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);

	QModelIndex insertRow();
	bool insertRows(int position, int rows, const QModelIndex &index = QModelIndex());
	bool removeRows(int position, int rows, const QModelIndex &index = QModelIndex());

	void sort( int column, Qt::SortOrder order = Qt::AscendingOrder );
	QMimeData *mimeData(const QModelIndexList &indexes) const;

	void updateComplete();
	ecsControlGroup* findItem( int id );
	int findItemIndex( int id );
	void clear();

	QList<ecsControlGroup*> ecsControlGroups;
	QString objectType;

signals:
	void modified();
};


#endif // ecsControlGroupMODEL_H
