#ifndef REPORTSTAB_H
#define REPORTSTAB_H

#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QTableView>
#include <QWidget>

#include "reportmodel.h"

// Reports on what has been paid out and what has been invoiced: pick a report,
// a year, and a month where the report is of one.
class ReportsTab : public QWidget
{
    Q_OBJECT
public:
    // The reports on offer, in the order they are listed.
    enum Report {
        SalaryMonth,    // the underlag for Skatteverket's arbetsgivardeklaration
        SalaryYear,
        InvoiceMonth,
        InvoiceYear,
        InvoiceAgreementMonth,
        InvoiceAgreementYear,
    };

    explicit ReportsTab(QWidget *parent = nullptr);

    void load(void);

public slots:
    void onDatabaseUpdated(void);

private:
    QComboBox* m_report_type;
    QComboBox* m_year;
    QComboBox* m_month;
    QPushButton* m_show;
    QPushButton* m_pdf_view;
    QPushButton* m_pdf_generate;

    QLabel* m_heading;
    QLabel* m_empty;   // shown only when the period holds nothing
    QTableView* m_report;
    ReportModel* m_reportModel;

    Report currentReport(void) const;
    void setEmptyMessage(const QString& message);
    int startMonth(void) const;
    void fillYears(void);

private slots:
    void onReportTypeChanged(int index);
    void onShow(void);
    void onViewReport(void);
    void onGenerateReport(void);
};

#endif // REPORTSTAB_H
