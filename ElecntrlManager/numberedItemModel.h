#ifndef APPLIANCEMODEL_H
#define APPLIANCEMODEL_H

#include <QStringList>
#include <QModelIndex>
#include <QMimeData>
#include <QAbstractTableModel>

#include <numberedItem.h>

class NumberedItemModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    NumberedItemModel() {};

    NumberedItemModel( QObject *parent = 0 )
        : QAbstractTableModel(parent) {}

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    QVariant data(const QModelIndex &index, int role) const;

    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const;

    Qt::ItemFlags flags(const QModelIndex &index) const;

    bool setData(const QModelIndex &index, const QVariant &value,
                 int role = Qt::EditRole);

    bool insertRows(int position, int rows, const QModelIndex &index = QModelIndex());

    bool removeRows(int position, int rows, const QModelIndex &index = QModelIndex());
    void sort( int column, Qt::SortOrder order = Qt::AscendingOrder );
    QMimeData *mimeData(const QModelIndexList &indexes) const;
    float accumulatedOffset( int itemIndex );
    float calculateHeight( int itemIndex );

    void setItemOffset( int itemIndex, float newOffset );
    void updateComplete();
    NumberedItem* findItem( int id );
    void clear();

    QList<NumberedItem> numberedItemData;

signals:
    void modified();

};


#endif // APPLIANCEMODEL_H
