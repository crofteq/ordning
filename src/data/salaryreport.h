#ifndef SALARYREPORT_H
#define SALARYREPORT_H

#include <QList>
#include <QString>

#include "salaryslips.h"

// One line of a salary report: an employee, or the total of the report.
struct SalaryReportRow
{
    QString employee_name;
    QString personnummer;
    int slips = 0;          // how many salary slips the line sums up
    long long gross = 0;    // öre
    long long tax = 0;      // öre
    long long tax_free = 0; // öre, the skattefria ersättningar paid out
    long long net = 0;      // öre
    long long employer_fee = 0; // öre, what the company pays on top
    bool total = false;     // the last line, the sum of the ones above

    bool operator==(const SalaryReportRow& o) const
    {
        return employee_name == o.employee_name && personnummer == o.personnummer
            && slips == o.slips && gross == o.gross && tax == o.tax
            && tax_free == o.tax_free && net == o.net
            && employer_fee == o.employer_fee && total == o.total;
    }
};

// The salaries paid out in `year` and `month` (1-12), one line per employee
// and a total last. This is what goes into Skatteverket's arbetsgivar-
// deklaration: it follows the payment date, not the month the salary is for.
// The skattefria ersättningar are kept apart from the bruttolön, since that is
// how they are reported: an amount of traktamente is a cross in the
// individuppgift and no part of the kontant bruttolön.
// Empty when nothing was paid out that month.
QList<SalaryReportRow> salary_report_month(const SalarySlips& slips, int year, int month);

// The same for a whole fiscal year, the one that began in `year`. With
// `start_month` 1 that is the calendar year; see data/fiscalyear.h.
QList<SalaryReportRow> salary_report_year(const SalarySlips& slips, int year, int start_month);

// The fiscal years salaries have been paid out in, oldest first.
QList<int> salary_report_years(const SalarySlips& slips, int start_month);

#endif // SALARYREPORT_H
