#ifndef EMPLOYEETABLEMODEL_H
#define EMPLOYEETABLEMODEL_H

#include <QAbstractTableModel>
#include <QObject>

#include "data/employee.h"

class EmployeeTableModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit EmployeeTableModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    void load(void);
    Employee* getEmployee(const QModelIndex &index);

private:
    QList<Employee*> m_list;
};

#endif // EMPLOYEETABLEMODEL_H
