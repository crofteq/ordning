#ifndef AGREEMENTTABLEMODEL_H
#define AGREEMENTTABLEMODEL_H

#include <QAbstractTableModel>
#include <QObject>

#include "data/agreement.h"

class AgreementTableModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit AgreementTableModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    void load(int customerId, bool showInactive);
    Agreement* getAgreement(const QModelIndex &index);

private:
    QList<Agreement*> m_list;
};

#endif // AGREEMENTTABLEMODEL_H
