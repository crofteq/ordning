#include <QVariant>

#include "data/database.h"
#include "util/amount.h"

#include "salarydrafttablemodel.h"

SalaryDraftTableModel::SalaryDraftTableModel(QObject *parent)
    : QAbstractTableModel{parent}
{
}

void SalaryDraftTableModel::load(void)
{
    beginResetModel();
    m_list.clear();
    SalaryDrafts& drafts = DataBase::instance()->db()->m_salarydrafts;
    for (int i = 0; i < drafts.size(); i++)
    {
        m_list.append(drafts.at(i));
    }
    endResetModel();
}

SalaryDraft* SalaryDraftTableModel::getSalaryDraft(const QModelIndex &index)
{
    if (index.row() >= 0 && index.row() < m_list.size())
    {
        return m_list.at(index.row());
    }
    return nullptr;
}

int SalaryDraftTableModel::rowCount(const QModelIndex &parent) const {
    Q_UNUSED(parent)
    return m_list.size();
}

int SalaryDraftTableModel::columnCount(const QModelIndex &parent) const {
    Q_UNUSED(parent)
    return 3; // Employee, Payment date, Gross
}

QVariant SalaryDraftTableModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || role != Qt::DisplayRole) {
        return QVariant();
    }

    SalaryDraft* d = m_list.at(index.row());
    switch (index.column())
    {
    case 0:
    {
        // An employee cannot be deleted while a draft uses them, but a
        // hand-edited file could still point nowhere.
        Employee* e = DataBase::instance()->db()->m_employees.get(d->m_employee_id);
        return e ? e->name() : QString("(unknown employee)");
    }
    case 1:
        return d->m_payment_date.toString("yyyy-MM-dd");
    case 2:
        return amount_format_ore(d->gross_ore());
    }

    return QVariant();
}

QVariant SalaryDraftTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        switch (section) {
        case 0: return "Employee";
        case 1: return "Payment date";
        case 2: return "Bruttolön";
        }
    }
    return QVariant();
}
