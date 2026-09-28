#ifndef CUSTOMERTABLEMODEL_H
#define CUSTOMERTABLEMODEL_H

#include <QAbstractTableModel>
#include <QObject>

#include "data/customer.h"

class CustomerTableModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit CustomerTableModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    void load(void);
    Customer* getCustomer(const QModelIndex &index);

private:
    QList<Customer*> m_list;
};

#endif // CUSTOMERTABLEMODEL_H
