#include <QMap>

#include "fiscalyear.h"

#include "salaryreport.h"

//-----------------------------------------------------------------------------
// The slips of `year`, and of `month` as well when that is 1-12, summed per
// employee with a total last. Employees come in the order they were first
// paid.
static QList<SalaryReportRow> report(const SalarySlips& slips, int year, int month, int start_month)
{
    QList<int> order;
    QMap<int, SalaryReportRow> rows;

    for (int i = 0; i < slips.size(); i++)
    {
        const SalarySlip* p = slips.at(i);
        if (fiscal_year_of(p->m_payment_date, start_month) != year)
        {
            continue;
        }
        if (month >= 1 && month <= 12 && p->m_payment_date.month() != month)
        {
            continue;
        }

        if (!rows.contains(p->m_employee_id))
        {
            order.append(p->m_employee_id);
            SalaryReportRow row;
            row.employee_name = p->m_employee_name;
            row.personnummer = p->m_personnummer;
            rows.insert(p->m_employee_id, row);
        }

        SalaryReportRow& row = rows[p->m_employee_id];
        // The name and the personnummer are the ones on the latest slip: an
        // employee who marries mid-year is reported under the name they were
        // last paid under.
        row.employee_name = p->m_employee_name;
        row.personnummer = p->m_personnummer;
        row.slips++;
        row.gross += p->m_gross;
        row.tax += p->m_tax;
        row.tax_free += p->m_tax_free;
        row.net += p->m_net;
        row.employer_fee += p->m_employer_fee;
    }

    QList<SalaryReportRow> result;
    SalaryReportRow sum;
    sum.employee_name = "Totalt";
    sum.total = true;
    for (int employee_id : order)
    {
        const SalaryReportRow& row = rows.value(employee_id);
        result.append(row);
        sum.slips += row.slips;
        sum.gross += row.gross;
        sum.tax += row.tax;
        sum.tax_free += row.tax_free;
        sum.net += row.net;
        sum.employer_fee += row.employer_fee;
    }
    if (!result.isEmpty())
    {
        result.append(sum);
    }
    return result;
}

//-----------------------------------------------------------------------------
QList<SalaryReportRow> salary_report_month(const SalarySlips& slips, int year, int month)
{
    // A month is a month whatever the fiscal year looks like, and it is the
    // month Skatteverket is told about.
    return report(slips, year, month, 1);
}

//-----------------------------------------------------------------------------
QList<SalaryReportRow> salary_report_year(const SalarySlips& slips, int year, int start_month)
{
    return report(slips, year, 0, start_month);
}

//-----------------------------------------------------------------------------
QList<int> salary_report_years(const SalarySlips& slips, int start_month)
{
    QList<int> years;
    for (int i = 0; i < slips.size(); i++)
    {
        const int year = fiscal_year_of(slips.at(i)->m_payment_date, start_month);
        if (!years.contains(year))
        {
            years.append(year);
        }
    }
    std::sort(years.begin(), years.end());
    return years;
}
