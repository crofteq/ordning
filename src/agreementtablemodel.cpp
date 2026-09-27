#include <QDebug>
#include <QVariant>

#include "database.h"

#include "agreementtablemodel.h"

AgreementTableModel::AgreementTableModel(QObject *parent)
    : QAbstractTableModel{parent}
{
}

void AgreementTableModel::load(int customerId, bool showInactive) {
    beginResetModel();
    m_list.clear();
    endResetModel();

    if (customerId < 0)
    {
        return;
    }

    Customer* c = DataBase::instance()->db()->m_customers.get(customerId);
    if ( c == nullptr )
    {
        return;
    }

    if (c->m_agreements.size() <= 0)
    {
        return;
    }

    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    for (int i=0; i<c->m_agreements.size();i++)
    {
        Agreement* e = c->m_agreements.at(i);
        if (e == nullptr)
        {
            continue;
        }

        if (e->m_active || showInactive)
        {
            m_list.append(e);
        }
    }
    endInsertRows();
}

Agreement* AgreementTableModel::getAgreement(const QModelIndex &index) {
    if (index.row() >= 0 && index.row() < m_list.size())
    {
        return m_list.at(index.row());
    }
    return nullptr;
}

int AgreementTableModel::rowCount(const QModelIndex &parent) const {
    return m_list.size();
}

int AgreementTableModel::columnCount(const QModelIndex &parent) const {
    return 3;
}

QVariant AgreementTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
    {
        return QVariant();
    }

    // if (role == Qt::TextAlignmentRole)
    // {
    //     switch (index.column())
    //     {
    //     case 0:
    //         return Qt::AlignCenter;
    //     case 1:
    //         return Qt::AlignCenter;
    //     case 2:
    //         return Qt::AlignCenter;
    //     }
    // }

    if (!index.isValid() || role != Qt::DisplayRole) {
        return QVariant();
    }

    Agreement* e = m_list.at(index.row());
    switch (index.column())
    {
    case 0:
        return e->m_agreement_name;
    case 1:
        return e->m_agreement_id;
    case 2:
        return e->m_reference_name;
    }

    return QVariant();
}

QVariant AgreementTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        switch (section) {
        case 0: return "Department / Project";
        case 1: return "AgreementId/Date";
        case 2: return "Reference";
        }
    }
    return QVariant();
}
