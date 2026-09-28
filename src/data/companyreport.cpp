#include <QCollator>
#include <QLocale>

#include <algorithm>

#include "fiscalyear.h"
#include "invoicereport.h"
#include "salaryreport.h"

#include "companyreport.h"

//-----------------------------------------------------------------------------
void CompanyReportFigures::add(const CompanyReportFigures& other)
{
    invoiced += other.invoiced;
    salary_gross += other.salary_gross;
    employer_fee += other.employer_fee;
    tax += other.tax;
    tax_free += other.tax_free;
    hours += other.hours;
    invoices += other.invoices;
    credit_invoices += other.credit_invoices;
}

//-----------------------------------------------------------------------------
CompanyReportFigures CompanyReportYear::firstMonths(int count) const
{
    CompanyReportFigures sum;
    for (int i = 0; i < count && i < months.size(); i++)
    {
        sum.add(months.at(i));
    }
    return sum;
}

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
QString CompanyReport::elapsedMonths(void) const
{
    static const QStringList names = { "jan", "feb", "mar", "apr", "maj", "jun",
                                       "jul", "aug", "sep", "okt", "nov", "dec" };
    const QList<int> months = fiscal_year_months(start_month);
    const int first = months.first();
    const int last = months.at(qBound(1, months_complete, 12) - 1);
    if (first == last)
    {
        return names.at(first - 1);
    }
    return QString("%1–%2").arg(names.at(first - 1), names.at(last - 1));
}

//-----------------------------------------------------------------------------
int CompanyReport::largestCustomerShare(int year, bool to_date) const
{
    long long total = 0;
    long long largest = 0;
    for (const CompanyReportSeries& customer : customers)
    {
        const long long value = (to_date ? customer.to_date : customer.per_year).value(year);
        total += value;
        largest = qMax(largest, value);
    }
    if (total <= 0)
    {
        return 0;
    }
    return (int)qRound64(10000.0 * largest / total);
}

//-----------------------------------------------------------------------------
// Adds `amount` to `name` for `year`, and to what the year had come to by the
// same month when `to_date` says so. `group` is what the line belongs under,
// and two uppdrag of the same name under different customers are two lines.
static void addTo(QList<CompanyReportSeries>* series, const QString& name, int year, long long amount,
                  bool to_date, const QString& group = QString())
{
    for (CompanyReportSeries& entry : *series)
    {
        if (entry.name == name && entry.group == group)
        {
            entry.per_year[year] += amount;
            if (to_date)
            {
                entry.to_date[year] += amount;
            }
            return;
        }
    }

    CompanyReportSeries entry;
    entry.name = name;
    entry.group = group;
    entry.per_year[year] = amount;
    if (to_date)
    {
        entry.to_date[year] = amount;
    }
    series->append(entry);
}

//-----------------------------------------------------------------------------
CompanyReport company_report(const Business& business, const QDate& today)
{
    CompanyReport report;

    // What a year is comes from the company: the calendar year, or a broken
    // one beginning in some other month.
    const int start_month = qBound(1, business.m_companyinfo.m_fiscal_year_start_month, 12);
    report.start_month = start_month;

    // The months that are over, a month being over on its last day. A year
    // with none of them over yet has nothing to show, and the report ends
    // with the one before, whole.
    int this_year = fiscal_year_of(today, start_month);
    report.months_complete = fiscal_year_months(start_month).indexOf(today.month())
        + (today.day() == today.daysInMonth() ? 1 : 0);
    if (report.months_complete == 0)
    {
        this_year--;
        report.months_complete = 12;
    }
    report.running = report.months_complete < 12;

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
    // All there is may be in a year that is left out.
    first = qMin(first, this_year);

    // How many invoices and credit invoices each calendar month holds.
    QMap<QPair<int, int>, QPair<int, int>> counts;
    for (int i = 0; i < business.m_invoices.size(); i++)
    {
        const Invoice* p = business.m_invoices.at(i);
        const QDate date = QDate::fromString(p->m_date, Qt::ISODate);
        if (!date.isValid())
        {
            continue;
        }
        QPair<int, int>& count = counts[qMakePair(date.year(), date.month())];
        (p->m_is_credit ? count.second : count.first)++;
    }

    for (int year = first; year <= this_year; year++)
    {
        CompanyReportYear entry;
        entry.year = year;
        entry.name = fiscal_year_name(year, start_month);

        // Everything is gathered a month at a time, so that a year still
        // running can be set against the same months of the one before, the
        // customers and the consultants included.
        const QList<int> months = fiscal_year_months(start_month);
        for (int index = 0; index < months.size(); index++)
        {
            CompanyReportMonth m;
            m.month = months.at(index);
            m.year = fiscal_year_calendar_year(year, start_month, m.month);
            const bool to_date = index < report.months_complete;

            // A month of the last year that is not over is left empty: what
            // has been invoiced in it so far is no month to compare.
            if (year == this_year && !to_date)
            {
                entry.months.append(m);
                continue;
            }

            for (const InvoiceReportRow& row : invoice_report_month(business.m_invoices, m.year, m.month))
            {
                if (row.total)
                {
                    m.invoiced = row.sum;
                }
                else
                {
                    addTo(&report.customers, row.customer_name, year, row.sum, to_date);
                }
            }
            for (const InvoiceReportRow& row : invoice_report_agreement_month(business.m_invoices, business.m_customers, m.year, m.month))
            {
                if (!row.total)
                {
                    addTo(&report.assignments, row.agreement, year, row.sum, to_date, row.customer_name);
                }
            }
            for (const SalaryReportRow& row : salary_report_month(business.m_salaryslips, m.year, m.month))
            {
                if (row.total)
                {
                    m.salary_gross = row.gross;
                    m.employer_fee = row.employer_fee;
                    m.tax = row.tax;
                    m.tax_free = row.tax_free;
                }
                else
                {
                    addTo(&report.employees, row.employee_name, year, row.gross, to_date);
                }
            }
            for (const ConsultantHoursRow& row : consultant_hours_report_month(business.m_invoices, m.year, m.month))
            {
                if (row.total)
                {
                    m.hours = row.hours;
                }
                else if (row.hours != 0)
                {
                    // An invoice without hours, a fixed price, is nobody's
                    // hours: the consultants are what the hours were.
                    addTo(&report.consultants, row.consultant_name, year, row.hours, to_date);
                }
            }

            const QPair<int, int> count = counts.value(qMakePair(m.year, m.month));
            m.invoices = count.first;
            m.credit_invoices = count.second;

            entry.add(m);
            entry.months.append(m);
        }

        report.years.append(entry);
    }

    // Every list in order of name, and the uppdrag of a customer together,
    // whatever year each of them was first invoiced in.
    QCollator collator(QLocale(QLocale::Swedish, QLocale::Sweden));
    collator.setCaseSensitivity(Qt::CaseInsensitive);
    auto byName = [&collator](const CompanyReportSeries& a, const CompanyReportSeries& b) {
        if (a.group != b.group)
        {
            return collator.compare(a.group, b.group) < 0;
        }
        return collator.compare(a.name, b.name) < 0;
    };
    std::stable_sort(report.customers.begin(), report.customers.end(), byName);
    std::stable_sort(report.assignments.begin(), report.assignments.end(), byName);
    std::stable_sort(report.employees.begin(), report.employees.end(), byName);
    std::stable_sort(report.consultants.begin(), report.consultants.end(), byName);

    return report;
}
