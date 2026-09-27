#include <QCollator>
#include <QLocale>

#include <algorithm>

#include "fiscalyear.h"
#include "invoicereport.h"
#include "salaryreport.h"

#include "companyreport.h"

//-----------------------------------------------------------------------------
QList<int> CompanyReport::yearNumbers(void) const
{
    QList<int> numbers;
    for (const CompanyReportYear& year : years)
    {
        numbers.append(year.year);
    }
    return numbers;
}

//-----------------------------------------------------------------------------
// Adds `amount` to `name` for `year`, keeping the order the names first
// appeared in. `group` is what the line belongs under, and two uppdrag of the
// same name under different customers are two lines.
static void addTo(QList<CompanyReportSeries>* series, const QString& name, int year, long long amount,
                  const QString& group = QString())
{
    for (CompanyReportSeries& entry : *series)
    {
        if (entry.name == name && entry.group == group)
        {
            entry.per_year[year] += amount;
            return;
        }
    }

    CompanyReportSeries entry;
    entry.name = name;
    entry.group = group;
    entry.per_year[year] = amount;
    series->append(entry);
}

//-----------------------------------------------------------------------------
CompanyReport company_report(const Business& business, const QDate& today)
{
    CompanyReport report;

    // What a year is comes from the company: the calendar year, or a broken
    // one beginning in some other month.
    const int start_month = qBound(1, business.m_companyinfo.m_fiscal_year_start_month, 12);
    const int this_year = fiscal_year_of(today, start_month);

    // From the first year there is anything to show up to this one. A year in
    // between with nothing in it is kept: an empty year is worth seeing.
    int first = this_year;
    for (int year : invoice_report_years(business.m_invoices, start_month))
    {
        first = qMin(first, year);
    }
    for (int year : salary_report_years(business.m_salaryslips, start_month))
    {
        first = qMin(first, year);
    }

    for (int year = first; year <= this_year; year++)
    {
        CompanyReportYear entry;
        entry.year = year;
        entry.name = fiscal_year_name(year, start_month);

        // The months of the fiscal year, in the order they come.
        for (int month : fiscal_year_months(start_month))
        {
            CompanyReportMonth m;
            m.month = month;
            m.year = fiscal_year_calendar_year(year, start_month, month);

            for (const InvoiceReportRow& row : invoice_report_month(business.m_invoices, m.year, month))
            {
                if (row.total)
                {
                    m.invoiced = row.sum;
                }
            }
            for (const SalaryReportRow& row : salary_report_month(business.m_salaryslips, m.year, month))
            {
                if (row.total)
                {
                    m.salary_gross = row.gross;
                    m.employer_fee = row.employer_fee;
                    m.tax = row.tax;
                    m.tax_free = row.tax_free;
                }
            }

            entry.invoiced += m.invoiced;
            entry.salary_gross += m.salary_gross;
            entry.employer_fee += m.employer_fee;
            entry.tax += m.tax;
            entry.tax_free += m.tax_free;
            entry.months.append(m);
        }

        for (const InvoiceReportRow& row : invoice_report_year(business.m_invoices, year, start_month))
        {
            if (!row.total)
            {
                addTo(&report.customers, row.customer_name, year, row.sum);
            }
        }
        for (const InvoiceReportRow& row : invoice_report_agreement_year(business.m_invoices, business.m_customers, year, start_month))
        {
            if (!row.total)
            {
                addTo(&report.assignments, row.agreement, year, row.sum, row.customer_name);
            }
        }
        for (const SalaryReportRow& row : salary_report_year(business.m_salaryslips, year, start_month))
        {
            if (!row.total)
            {
                addTo(&report.employees, row.employee_name, year, row.gross);
            }
        }

        report.years.append(entry);
    }

    // The uppdrag of a customer have to stand together, whatever year each of
    // them was first invoiced in.
    QCollator collator(QLocale(QLocale::Swedish, QLocale::Sweden));
    collator.setCaseSensitivity(Qt::CaseInsensitive);
    std::stable_sort(report.assignments.begin(), report.assignments.end(),
                     [&collator](const CompanyReportSeries& a, const CompanyReportSeries& b) {
                         if (a.group != b.group)
                         {
                             return collator.compare(a.group, b.group) < 0;
                         }
                         return collator.compare(a.name, b.name) < 0;
                     });

    return report;
}
