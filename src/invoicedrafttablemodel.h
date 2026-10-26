#ifndef INVOICEDRAFTTABLEMODEL_H
#define INVOICEDRAFTTABLEMODEL_H

#include <QAbstractTableModel>
#include <QObject>

#include "data/invoicedraft.h"

class InvoiceDraftTableModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit InvoiceDraftTableModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    void load(void);
    InvoiceDraft* getInvoice(const QModelIndex &index);

private:
    QList<InvoiceDraft*> m_list;
};

#endif // INVOICEDRAFTTABLEMODEL_H
