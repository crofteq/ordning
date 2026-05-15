#include <QDebug>
#include <QVariant>

#include "data/database.h"

#include "customertablemodel.h"

CustomerTableModel::CustomerTableModel(QObject *parent)
    : QAbstractTableModel{parent}
{
}

void CustomerTableModel::load(void)
{
    beginResetModel();
    m_list.clear();
    endResetModel();

    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    m_list.append(DataBase::instance()->db()->m_customers.m_list);
    endInsertRows();
}

Customer* CustomerTableModel::getCustomer(const QModelIndex &index)
{
    if (index.row() < m_list.size())
    {
        return m_list.at(index.row());
    }
    return nullptr;
}

int CustomerTableModel::rowCount(const QModelIndex &parent) const {
    return m_list.size();
}

int CustomerTableModel::columnCount(const QModelIndex &parent) const {
    return 3; // Name, Orgnbr
}

QVariant CustomerTableModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || role != Qt::DisplayRole) {
        return QVariant();
    }

    Customer* c = m_list.at(index.row());
    switch (index.column())
    {
    case 0:
        return c->m_name;
    case 1:
        return c->m_org_number;
    case 2:
        return c->m_email;
    }

    return QVariant();
}

QVariant CustomerTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        switch (section) {
        case 0: return "Name";
        case 1: return "Org Number";
        case 2: return "Email";
        }
    }
    return QVariant();
}
