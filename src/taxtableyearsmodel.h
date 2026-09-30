#ifndef TAXTABLEYEARSMODEL_H
#define TAXTABLEYEARSMODEL_H

#include <QAbstractTableModel>
#include <QObject>

// The imported tax tables, one row per year. A year holds all of
// Skatteverket's tables, so the year is all there is to tell them apart by.
class TaxTableYearsModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit TaxTableYearsModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    void load(void);

    // The year of that row, or 0 for a row that is not there.
    int getYear(const QModelIndex &index) const;

private:
    QList<int> m_years;
};

#endif // TAXTABLEYEARSMODEL_H
