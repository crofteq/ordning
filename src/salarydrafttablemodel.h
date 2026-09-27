#ifndef SALARYDRAFTTABLEMODEL_H
#define SALARYDRAFTTABLEMODEL_H

#include <QAbstractTableModel>
#include <QObject>

#include "data/salarydraft.h"

class SalaryDraftTableModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit SalaryDraftTableModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    void load(void);
    SalaryDraft* getSalaryDraft(const QModelIndex &index);

private:
    QList<SalaryDraft*> m_list;
};

#endif // SALARYDRAFTTABLEMODEL_H
