#include <QDebug>
#include <QVariant>

#include "data/database.h"

#include "employeetablemodel.h"

EmployeeTableModel::EmployeeTableModel(QObject *parent)
    : QAbstractTableModel{parent}
{
}

void EmployeeTableModel::load(void)
{
    beginResetModel();
    m_list.clear();
    Employees& employees = DataBase::instance()->db()->m_employees;
    for (int i = 0; i < employees.size(); i++)
    {
        m_list.append(employees.at(i));
    }
    endResetModel();
}

Employee* EmployeeTableModel::getEmployee(const QModelIndex &index)
{
    if (index.row() >= 0 && index.row() < m_list.size())
    {
        return m_list.at(index.row());
    }
    return nullptr;
}

int EmployeeTableModel::rowCount(const QModelIndex &parent) const {
    Q_UNUSED(parent)
    return m_list.size();
}

int EmployeeTableModel::columnCount(const QModelIndex &parent) const {
    Q_UNUSED(parent)
    return 3; // Name, Employment number, Active
}

QVariant EmployeeTableModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || role != Qt::DisplayRole) {
        return QVariant();
    }

    Employee* e = m_list.at(index.row());
    switch (index.column())
    {
    case 0:
        return e->name();
    case 1:
        return e->employmentNumber();
    case 2:
        return e->m_active ? QString("Yes") : QString("No");
    }

    return QVariant();
}

QVariant EmployeeTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        switch (section) {
        case 0: return "Name";
        case 1: return "Employment number";
        case 2: return "Active";
        }
    }
    return QVariant();
}
