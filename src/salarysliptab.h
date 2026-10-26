#ifndef SALARYSLIPTAB_H
#define SALARYSLIPTAB_H

#include <QPushButton>
#include <QTableView>
#include <QWidget>

#include "salarysliptablemodel.h"

// The approved salary slips. They cannot be edited or deleted: a slip is the
// record of a salary that has been paid out. They can be viewed and exported
// as a PDF lönebesked.
class SalarySlipTab : public QWidget
{
    Q_OBJECT
public:
    explicit SalarySlipTab(QWidget *parent = nullptr);

    void load(void);

    // Opens `slip` as a lönebesked in a window of its own, with the year-to-
    // date totals from the stored slips. A preview carries a UTKAST watermark.
    static void showPdf(const SalarySlip& slip, bool preview);

public slots:
    void onDatabaseUpdated(void);

private:
    QTableView* m_slips;
    SalarySlipTableModel* m_slipModel;

    QPushButton* m_view;
    QPushButton* m_export;

    void updateButtons(void);

private slots:
    void onSlipSelected(const QModelIndex &index);
    void onView(void);
    void onExport(void);
};

#endif // SALARYSLIPTAB_H
