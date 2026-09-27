#include <QDate>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QVBoxLayout>

#include <QBuffer>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QStandardPaths>

#include "companyreportprinter.h"
#include "data/companyreport.h"
#include "data/database.h"
#include "data/fiscalyear.h"
#include "pdfviewer.h"
#include "data/invoicereport.h"
#include "data/salaryreport.h"
#include "util/amount.h"

#include "reportstab.h"

//-----------------------------------------------------------------------------
static QStringList monthNames(void)
{
    return { "Januari", "Februari", "Mars", "April", "Maj", "Juni",
             "Juli", "Augusti", "September", "Oktober", "November", "December" };
}

//-----------------------------------------------------------------------------
ReportsTab::ReportsTab(QWidget *parent)
    : QWidget{parent}
{
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setContentsMargins(5, 5, 5, 5);

    const QDate today = QDate::currentDate();

    // One row, in two groups: the reports that come out as a table, and the
    // company report, which is a document.
    {
        QHBoxLayout *top = new QHBoxLayout;

        QGroupBox *p_reports = new QGroupBox("Monthly/yearly reports");
        QHBoxLayout *hbox = new QHBoxLayout;

        m_report_type = new QComboBox();
        m_report_type->addItem("Underlag för arbetsgivardeklaration", SalaryMonth);
        m_report_type->addItem("Utbetald lön per år och anställd", SalaryYear);
        m_report_type->addItem("Fakturerat per månad och kund", InvoiceMonth);
        m_report_type->addItem("Fakturerat per år och kund", InvoiceYear);
        m_report_type->addItem("Fakturerat per månad och uppdrag", InvoiceAgreementMonth);
        m_report_type->addItem("Fakturerat per år och uppdrag", InvoiceAgreementYear);
        connect(m_report_type, &QComboBox::currentIndexChanged, this, &ReportsTab::onReportTypeChanged);
        // The report type holds the longest text and is the only one worth
        // widening: it takes whatever room the others leave.
        hbox->addWidget(m_report_type, 1);

        m_year = new QComboBox();
        // A year and a month are as wide as what stands in them. The year list
        // is filled later, so the width has to follow the contents rather than
        // be settled here.
        m_year->setSizeAdjustPolicy(QComboBox::AdjustToContents);
        m_year->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        hbox->addWidget(m_year);

        m_month = new QComboBox();
        m_month->setSizeAdjustPolicy(QComboBox::AdjustToContents);
        m_month->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        const QStringList months = monthNames();
        for (int i = 0; i < months.size(); i++)
        {
            m_month->addItem(months.at(i), i + 1);
        }
        m_month->setCurrentIndex(today.month() - 1);
        hbox->addWidget(m_month);

        m_show = new QPushButton("Show");
        connect(m_show, &QPushButton::clicked, this, &ReportsTab::onShow);
        hbox->addWidget(m_show);

        p_reports->setLayout(hbox);
        top->addWidget(p_reports, 1);

        // The company report is a document rather than a table: one button to
        // look at it, one to keep it, named as on the Invoices tab.
        QGroupBox *p_pdf = new QGroupBox("Company report (PDF)");
        QHBoxLayout *pdfbox = new QHBoxLayout;

        m_pdf_view = new QPushButton("View");
        connect(m_pdf_view, &QPushButton::clicked, this, &ReportsTab::onViewReport);
        pdfbox->addWidget(m_pdf_view);

        m_pdf_generate = new QPushButton("Generate");
        connect(m_pdf_generate, &QPushButton::clicked, this, &ReportsTab::onGenerateReport);
        pdfbox->addWidget(m_pdf_generate);

        p_pdf->setLayout(pdfbox);
        top->addWidget(p_pdf);

        layout->addLayout(top);
    }

    m_heading = new QLabel();
    QFont heading = m_heading->font();
    heading.setBold(true);
    m_heading->setFont(heading);
    layout->addWidget(m_heading);

    m_report = new QTableView();
    m_report->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_report->setSelectionMode(QAbstractItemView::SingleSelection);
    m_report->verticalHeader()->setVisible(false);
    m_reportModel = new ReportModel(this);
    m_report->setModel(m_reportModel);
    layout->addWidget(m_report);

    // Only for a period there is nothing to report for: an empty table under a
    // heading says little on its own. It is out of the way otherwise, so a
    // report is followed by its figures and nothing else.
    m_empty = new QLabel();
    m_empty->setVisible(false);
    layout->addWidget(m_empty);

    setLayout(layout);
    fillYears();
    onReportTypeChanged(m_report_type->currentIndex());
}

//-----------------------------------------------------------------------------
void ReportsTab::load(void)
{
    fillYears();
}

//-----------------------------------------------------------------------------
void ReportsTab::onDatabaseUpdated(void)
{
    // A report is of the business that was open when it was asked for.
    m_reportModel->clear();
    m_heading->clear();
    setEmptyMessage(QString());
    fillYears();
}

//-----------------------------------------------------------------------------
// The month the company's räkenskapsår begins in; 1, the calendar year, until
// it says otherwise.
int ReportsTab::startMonth(void) const
{
    return qBound(1, DataBase::instance()->db()->m_companyinfo.m_fiscal_year_start_month, 12);
}

//-----------------------------------------------------------------------------
ReportsTab::Report ReportsTab::currentReport(void) const
{
    return (Report)m_report_type->currentData().toInt();
}

//-----------------------------------------------------------------------------
// The years there is anything to report on, and this year whether or not
// there is. The latest is the one to start from.
void ReportsTab::fillYears(void)
{
    Business* b = DataBase::instance()->db();
    const int start_month = startMonth();
    QList<int> years = salary_report_years(b->m_salaryslips, start_month);
    for (int year : invoice_report_years(b->m_invoices, start_month))
    {
        if (!years.contains(year))
        {
            years.append(year);
        }
    }
    const int current = fiscal_year_of(QDate::currentDate(), start_month);
    if (!years.contains(current))
    {
        years.append(current);
    }
    std::sort(years.begin(), years.end());

    const int was = m_year->currentData().isValid() ? m_year->currentData().toInt() : current;
    m_year->clear();
    for (int year : years)
    {
        // "2025", or "2025/26" for a broken räkenskapsår.
        m_year->addItem(fiscal_year_name(year, start_month), year);
    }
    const int index = m_year->findData(was);
    m_year->setCurrentIndex(index >= 0 ? index : m_year->count() - 1);
}

//-----------------------------------------------------------------------------
void ReportsTab::onReportTypeChanged(int index)
{
    Q_UNUSED(index)

    // A yearly report has no month to pick.
    const Report report = currentReport();
    m_month->setEnabled(report == SalaryMonth || report == InvoiceMonth || report == InvoiceAgreementMonth);
}

//-----------------------------------------------------------------------------
// Says why the table is empty, and says nothing at all when it is not: the
// label is hidden so that it takes no room under a report that has figures.
void ReportsTab::setEmptyMessage(const QString& message)
{
    m_empty->setText(message);
    m_empty->setVisible(!message.isEmpty());
}

//-----------------------------------------------------------------------------
void ReportsTab::onShow(void)
{
    Business* b = DataBase::instance()->db();
    const int year = m_year->currentData().toInt();
    const int month = m_month->currentData().toInt();
    // A month belongs to a calendar year, which is not the year the fiscal
    // one is named after once the fiscal year is broken.
    const int month_year = fiscal_year_calendar_year(year, startMonth(), month);
    const QString period = m_month->isEnabled()
        ? QString("%1 %2").arg(m_month->currentText()).arg(month_year)
        : fiscal_year_name(year, startMonth());

    QStringList headers;
    QList<QStringList> rows;
    int first_figure = 0;

    switch (currentReport())
    {
    case SalaryMonth:
    case SalaryYear:
    {
        const QList<SalaryReportRow> report = (currentReport() == SalaryMonth)
            ? salary_report_month(b->m_salaryslips, month_year, month)
            : salary_report_year(b->m_salaryslips, year, startMonth());
        headers = { "Anställd", "Personnummer", "Lönebesked", "Bruttolön", "Avdragen skatt", "Arbetsgivaravgift", "Skattefritt" };
        first_figure = 2;
        long long to_pay = 0;
        for (const SalaryReportRow& r : report)
        {
            rows.append({ r.employee_name, r.personnummer, QString::number(r.slips),
                          amount_format_ore(r.gross), amount_format_ore(r.tax),
                          amount_format_ore(r.employer_fee), amount_format_ore(r.tax_free) });
            if (r.total)
            {
                to_pay = r.tax + r.employer_fee;
            }
        }
        // What the arbetsgivardeklaration is paid with: the avdragen skatt and
        // the arbetsgivaravgifter together. It is no part of any employee's
        // line, so it stands under the avgifterna, the last of the two columns
        // it adds up, on a line of its own after the total.
        if (currentReport() == SalaryMonth && !rows.isEmpty())
        {
            rows.append({ "Till skattekonto", "", "", "", "", amount_format_ore(to_pay), "" });
        }
        break;
    }
    case InvoiceMonth:
    case InvoiceYear:
    {
        const QList<InvoiceReportRow> report = (currentReport() == InvoiceMonth)
            ? invoice_report_month(b->m_invoices, month_year, month)
            : invoice_report_year(b->m_invoices, year, startMonth());
        headers = { "Kund", "Fakturor", "Belopp exkl. moms", "Moms", "Belopp inkl. moms" };
        first_figure = 1;
        for (const InvoiceReportRow& r : report)
        {
            rows.append({ r.customer_name, QString::number(r.invoices), amount_format_ore(r.sum),
                          amount_format_ore(r.vat), amount_format_ore(r.sum_including_vat) });
        }
        break;
    }
    case InvoiceAgreementMonth:
    case InvoiceAgreementYear:
    {
        const QList<InvoiceReportRow> report = (currentReport() == InvoiceAgreementMonth)
            ? invoice_report_agreement_month(b->m_invoices, b->m_customers, month_year, month)
            : invoice_report_agreement_year(b->m_invoices, b->m_customers, year, startMonth());
        headers = { "Kund", "Uppdrag", "Fakturor", "Belopp exkl. moms", "Moms", "Belopp inkl. moms" };
        first_figure = 2;
        for (const InvoiceReportRow& r : report)
        {
            rows.append({ r.customer_name, r.agreement, QString::number(r.invoices), amount_format_ore(r.sum),
                          amount_format_ore(r.vat), amount_format_ore(r.sum_including_vat) });
        }
        break;
    }
    }

    m_heading->setText(QString("%1, %2").arg(m_report_type->currentText(), period));
    setEmptyMessage(rows.isEmpty() ? QString("There is nothing to report for that period.") : QString());
    m_reportModel->setReport(headers, rows, first_figure);
    m_report->resizeColumnsToContents();
}

//-----------------------------------------------------------------------------
// The whole year as a document: the chart, the totals, and the year per
// customer and per employee.
static QByteArray companyReportPdf(void)
{
    Business* b = DataBase::instance()->db();
    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly);
    // Every year up to this one, so the years can be compared.
    CompanyReportPrinter::create_pdf(company_report(*b, QDate::currentDate()), b->m_companyinfo, &buffer);
    return buffer.data();
}

//-----------------------------------------------------------------------------
void ReportsTab::onViewReport(void)
{
    pdf_show(companyReportPdf(), "Rapport");
}

//-----------------------------------------------------------------------------
void ReportsTab::onGenerateReport(void)
{
    const QString suggestion = QString("%1/%2-rapport.pdf")
        .arg(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation))
        .arg(QDate::currentDate().toString("yyyy-MM-dd"));

    const QString filename = QFileDialog::getSaveFileName(this, "Save report", suggestion, "PDF Files (*.pdf)");
    if (filename.isEmpty())
    {
        return;
    }

    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly))
    {
        QMessageBox::warning(this, "Save report", QString("Could not write %1.").arg(filename));
        return;
    }
    file.write(companyReportPdf());
}
