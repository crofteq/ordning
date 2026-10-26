#ifndef SALARYSLIPTABLEMODEL_H
#define SALARYSLIPTABLEMODEL_H

#include <QAbstractTableModel>
#include <QObject>

#include "data/salaryslip.h"

class SalarySlipTableModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit SalarySlipTableModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    void load(void);
    SalarySlip* getSalarySlip(const QModelIndex &index);

private:
    QList<SalarySlip*> m_list;
};

#endif // SALARYSLIPTABLEMODEL_H
