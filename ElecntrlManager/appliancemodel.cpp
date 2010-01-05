#include "appliancemodel.h"

int ApplianceModel::rowCount(const QModelIndex &parent) const
{
    return appliances.count();
}

//---------------------------------------------------------------------------

QVariant ApplianceModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (index.row() >= appliances.size())
        return QVariant();

    if (role == Qt::DisplayRole || role == Qt::EditRole)
        return appliances.at(index.row());
    else
        return QVariant();
}

//---------------------------------------------------------------------------

QVariant ApplianceModel::headerData(int section, Qt::Orientation orientation,
                                    int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    if (orientation == Qt::Horizontal)
        return QString("%1").arg(section);
    else
        return QString("%1").arg(section);
}

//---------------------------------------------------------------------------

Qt::ItemFlags ApplianceModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemIsEnabled;

    return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
}

//---------------------------------------------------------------------------

bool ApplianceModel::setData(const QModelIndex &index,
                             const QVariant &value, int role)
{
    if (index.isValid() && role == Qt::EditRole) {

        appliances.replace(index.row(), value.toString());
        emit dataChanged(index, index);
        return true;
    }
    return false;
}

//---------------------------------------------------------------------------

bool ApplianceModel::insertRows(int position, int rows, const QModelIndex &parent)
{
    beginInsertRows(QModelIndex(), position, position+rows-1);

    for (int row = 0; row < rows; ++row) {
        appliances.insert(position, "");
    }

    endInsertRows();
    return true;
}

//---------------------------------------------------------------------------

bool ApplianceModel::removeRows(int position, int rows, const QModelIndex &parent)
{
    beginRemoveRows(QModelIndex(), position, position+rows-1);

    for (int row = 0; row < rows; ++row) {
        appliances.removeAt(position);
    }

    endRemoveRows();
    return true;
}
