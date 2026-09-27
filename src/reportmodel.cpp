#include <QVariant>

#include "reportmodel.h"

ReportModel::ReportModel(QObject *parent)
    : QAbstractTableModel{parent}
{
}

void ReportModel::setReport(const QStringList& headers, const QList<QStringList>& rows, int first_figure)
{
    beginResetModel();
    m_headers = headers;
    m_rows = rows;
    m_first_figure = first_figure;
    endResetModel();
}

void ReportModel::clear(void)
{
    setReport({}, {}, 0);
}

int ReportModel::rowCount(const QModelIndex &parent) const {
    Q_UNUSED(parent)
    return m_rows.size();
}

int ReportModel::columnCount(const QModelIndex &parent) const {
    Q_UNUSED(parent)
    return m_headers.size();
}

QVariant ReportModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() >= m_rows.size() || index.column() >= m_headers.size()) {
        return QVariant();
    }

    // The figures read as a column, so they are right aligned. Qt::Alignment
    // is QtCore, unlike a font, so the model can say this much itself.
    if (role == Qt::TextAlignmentRole) {
        return index.column() >= m_first_figure ? QVariant(int(Qt::AlignRight | Qt::AlignVCenter)) : QVariant();
    }

    if (role != Qt::DisplayRole) {
        return QVariant();
    }

    const QStringList& row = m_rows.at(index.row());
    return index.column() < row.size() ? QVariant(row.at(index.column())) : QVariant();
}

QVariant ReportModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal && section < m_headers.size()) {
        return m_headers.at(section);
    }
    return QVariant();
}
