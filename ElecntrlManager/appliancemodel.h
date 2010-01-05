#ifndef APPLIANCEMODEL_H
#define APPLIANCEMODEL_H

#include <QStringList>
#include <QAbstractListModel>

class ApplianceModel : public QAbstractListModel
{
    Q_OBJECT

public:
    ApplianceModel() {};

    ApplianceModel(const QStringList &strings, QObject *parent = 0)
        : QAbstractListModel(parent), appliances(strings) {}

    int rowCount(const QModelIndex &parent = QModelIndex()) const;

    QVariant data(const QModelIndex &index, int role) const;

    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const;

    Qt::ItemFlags flags(const QModelIndex &index) const;

    bool setData(const QModelIndex &index, const QVariant &value,
                 int role = Qt::EditRole);

    bool insertRows(int position, int rows, const QModelIndex &index = QModelIndex());

    bool removeRows(int position, int rows, const QModelIndex &index = QModelIndex());

private:
    QStringList appliances;
};


#endif // APPLIANCEMODEL_H
