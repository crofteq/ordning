#include <QVariant>

#include "data/database.h"

#include "taxtableyearsmodel.h"

TaxTableYearsModel::TaxTableYearsModel(QObject *parent)
    : QAbstractTableModel{parent}
{
}

void TaxTableYearsModel::load(void)
{
    beginResetModel();
    m_years = DataBase::instance()->db()->m_taxtables.years();
    endResetModel();
}

int TaxTableYearsModel::getYear(const QModelIndex &index) const
{
    if (index.row() >= 0 && index.row() < m_years.size())
    {
        return m_years.at(index.row());
    }
    return 0;
}

int TaxTableYearsModel::rowCount(const QModelIndex &parent) const {
    Q_UNUSED(parent)
    return m_years.size();
}

int TaxTableYearsModel::columnCount(const QModelIndex &parent) const {
    Q_UNUSED(parent)
    return 1; // Year
}

QVariant TaxTableYearsModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() >= m_years.size() || role != Qt::DisplayRole) {
        return QVariant();
    }

    if (index.column() == 0)
    {
        return QString::number(m_years.at(index.row()));
    }
    return QVariant();
}

QVariant TaxTableYearsModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        if (section == 0) {
            return "År";
        }
    }
    return QVariant();
}
