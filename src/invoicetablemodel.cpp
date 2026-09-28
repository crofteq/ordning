#include <QDebug>
#include <QVariant>

#include "data/database.h"

#include "util/amount.h"

#include "invoicetablemodel.h"

//-----------------------------------------------------------------------------
InvoiceTableModel::InvoiceTableModel(QObject *parent)
    : QAbstractTableModel{parent}
{}

//-----------------------------------------------------------------------------
void InvoiceTableModel::load(void) {
    beginResetModel();
    m_list.clear();
    endResetModel();

    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    for (int i=0; i<DataBase::instance()->db()->m_invoices.size();i++)
    {
        Invoice* e = DataBase::instance()->db()->m_invoices.at(i);
        if (e == nullptr)
        {
            continue;
        }
        m_list.append(e);
    }
    endInsertRows();
}

//-----------------------------------------------------------------------------
Invoice* InvoiceTableModel::getInvoice(const QModelIndex &index) {
    if (index.row() >= 0 && index.row() < m_list.size())
    {
        return m_list.at(index.row());
    }
    return nullptr;
}

//-----------------------------------------------------------------------------
int InvoiceTableModel::rowCount(const QModelIndex &parent) const {
    return m_list.size();
}

//-----------------------------------------------------------------------------
int InvoiceTableModel::columnCount(const QModelIndex &parent) const {
    return 7;
}

//-----------------------------------------------------------------------------
QVariant InvoiceTableModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || role != Qt::DisplayRole) {
        return QVariant();
    }

    Invoice* e = m_list.at(index.row());
    switch (index.column())
    {
    case 0:
        return e->m_number;
    case 1:
        return e->m_date;
    case 2:
        return e->m_customer_name;
    case 3:
        return e->m_customer_reference_name;
    case 4:
        // An invoice from before the uppdrag was kept with it has none.
        return e->m_agreement_name;
    case 5:
        // An invoice whose consultant could not be worked out from the name it
        // was printed with has none.
        return e->m_consultant_name;
    case 6:
        return amount_format_ore(e->m_sum_including_vat);
    }

    return QVariant();
}

//-----------------------------------------------------------------------------
QVariant InvoiceTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        switch (section) {
        case 0: return "Nummer";
        case 1: return "Datum";
        case 2: return "Kund";
        case 3: return "Referens";
        case 4: return "Uppdrag";
        case 5: return "Konsult";
        case 6: return "Summa";
        }
    }
    return QVariant();
}
