#ifndef REPORTMODEL_H
#define REPORTMODEL_H

#include <QAbstractTableModel>
#include <QObject>
#include <QStringList>

// Shows a report: its headings and its lines, already put into words by
// whoever asked for it. Unlike the other models it is not filled from the
// database, since each report has columns of its own.
class ReportModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit ReportModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Columns from `first_figure` on are figures, and line up to the right.
    void setReport(const QStringList& headers, const QList<QStringList>& rows, int first_figure);
    void clear(void);

private:
    QStringList m_headers;
    QList<QStringList> m_rows;
    int m_first_figure = 0;
};

#endif // REPORTMODEL_H
