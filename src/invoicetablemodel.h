#ifndef INVOICETABLEMODEL_H
#define INVOICETABLEMODEL_H

#include <QAbstractTableModel>
#include <QObject>

#include "data/invoice.h"

class InvoiceTableModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit InvoiceTableModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    void load(void);
    Invoice* getInvoice(const QModelIndex &index);

private:
    QList<Invoice*> m_list;
};

#endif // INVOICETABLEMODEL_H
