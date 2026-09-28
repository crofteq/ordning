#include <QVariant>

#include "data/database.h"
#include "util/amount.h"

#include "salarysliptablemodel.h"

SalarySlipTableModel::SalarySlipTableModel(QObject *parent)
    : QAbstractTableModel{parent}
{
}

void SalarySlipTableModel::load(void)
{
    beginResetModel();
    m_list.clear();
    SalarySlips& slips = DataBase::instance()->db()->m_salaryslips;
    for (int i = 0; i < slips.size(); i++)
    {
        m_list.append(slips.at(i));
    }
    endResetModel();
}

SalarySlip* SalarySlipTableModel::getSalarySlip(const QModelIndex &index)
{
    if (index.row() >= 0 && index.row() < m_list.size())
    {
        return m_list.at(index.row());
    }
    return nullptr;
}

int SalarySlipTableModel::rowCount(const QModelIndex &parent) const {
    Q_UNUSED(parent)
    return m_list.size();
}

int SalarySlipTableModel::columnCount(const QModelIndex &parent) const {
    Q_UNUSED(parent)
    return 5; // Payment date, Employee, Gross, Tax, Net
}

QVariant SalarySlipTableModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || role != Qt::DisplayRole) {
        return QVariant();
    }

    // Everything shown is the slip's own copy, so a slip reads the same after
    // the employee has changed.
    SalarySlip* p = m_list.at(index.row());
    switch (index.column())
    {
    case 0:
        return p->m_payment_date.toString("yyyy-MM-dd");
    case 1:
        return p->m_employee_name;
    case 2:
        return amount_format_ore(p->m_gross);
    case 3:
        return amount_format_ore(p->m_tax);
    case 4:
        return amount_format_ore(p->m_net);
    }

    return QVariant();
}

QVariant SalarySlipTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        switch (section) {
        case 0: return "Utbetalningsdag";
        case 1: return "Anställd";
        case 2: return "Bruttolön";
        case 3: return "Skatt";
        case 4: return "Utbetalt";
        }
    }
    return QVariant();
}
