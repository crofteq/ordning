#include <QDebug>
#include <QVariant>

#include "data/database.h"

#include "invoicedrafttablemodel.h"

//-----------------------------------------------------------------------------
InvoiceDraftTableModel::InvoiceDraftTableModel(QObject *parent)
    : QAbstractTableModel{parent}
{}

//-----------------------------------------------------------------------------
void InvoiceDraftTableModel::load(void) {
    beginResetModel();
    m_list.clear();
    endResetModel();

    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    for (int i=0; i<DataBase::instance()->db()->m_invoicedrafts.size();i++)
    {
        InvoiceDraft* e = DataBase::instance()->db()->m_invoicedrafts.at(i);
        if (e == nullptr)
        {
            continue;
        }
        m_list.append(e);
    }
    endInsertRows();
}

//-----------------------------------------------------------------------------
InvoiceDraft* InvoiceDraftTableModel::getInvoice(const QModelIndex &index) {
    if (index.row() < m_list.size())
    {
        return m_list.at(index.row());
    }
    return nullptr;
}

//-----------------------------------------------------------------------------
int InvoiceDraftTableModel::rowCount(const QModelIndex &parent) const {
    return m_list.size();
}

//-----------------------------------------------------------------------------
int InvoiceDraftTableModel::columnCount(const QModelIndex &parent) const {
    return 4;
}

//-----------------------------------------------------------------------------
QVariant InvoiceDraftTableModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || role != Qt::DisplayRole)
    {
        return QVariant();
    }

    InvoiceDraft* e = m_list.at(index.row());
    if (e == nullptr)
    {
        return QVariant();
    }

    Customer* c = DataBase::instance()->db()->m_customers.get(e->m_customer_id);
    if (c == nullptr)
    {
        return QVariant();
    }

    Agreement* a = c->m_agreements.get(e->m_agreement_id);
    if (a == nullptr)
    {
        return QVariant();
    }

    switch (index.column())
    {
    case 0:
        return c->m_name;
    case 1:
        return a->m_agreement_name;
    case 2:
        return e->m_invoice_date.toString("yyyy-MM-dd");
    case 3:
        return e->m_hours_standard;
    }

    return QVariant();
}

//-----------------------------------------------------------------------------
QVariant InvoiceDraftTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        switch (section) {
        case 0: return "Customer";
        case 1: return "Agreement";
        case 2: return "InvoiceDate";
        case 3: return "Hours";
        }
    }
    return QVariant();
}
