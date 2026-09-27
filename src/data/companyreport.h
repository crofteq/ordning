#ifndef COMPANYREPORT_H
#define COMPANYREPORT_H

#include <QList>
#include <QMap>
#include <QString>

#include <QDate>

#include "business.h"

// One month of a year. Everything in öre.
struct CompanyReportMonth
{
    int month = 0;                  // 1-12, the calendar month
    int year = 0;                   // the calendar year it falls in
    long long invoiced = 0;         // excluding VAT, by invoice date
    long long salary_gross = 0;     // bruttolön, by payment date
    long long employer_fee = 0;
    long long tax = 0;              // avdragen skatt
    long long tax_free = 0;         // skattefria ersättningar paid out

    // What the salaries cost the company: the gross, the fee on top, and the
    // skattefria ersättningar, which are paid out of the same pocket even
    // though they are no part of the bruttolön.
    long long salary_cost(void) const { return salary_gross + employer_fee + tax_free; }
};

// One year of the company.
struct CompanyReportYear
{
    int year = 0;                       // the year the fiscal year began in
    QString name;                       // "2025" or "2025/26"
    QList<CompanyReportMonth> months;   // always twelve, in fiscal order

    long long invoiced = 0;
    long long salary_gross = 0;
    long long employer_fee = 0;
    long long tax = 0;
    long long tax_free = 0;

    long long salary_cost(void) const { return salary_gross + employer_fee + tax_free; }

    // What is left of the invoiced amount once the salaries are paid. It is
    // not a profit: the company has other costs Ordning knows nothing about.
    long long after_salaries(void) const { return invoiced - salary_cost(); }
};

// A customer, an uppdrag or an employee, and what they came to each year.
struct CompanyReportSeries
{
    QString name;
    // Who the line belongs under, when the lines are grouped: an uppdrag is
    // one of its customer's, and the customer heads the rows of its own. Empty
    // when the lines stand on their own, as customers and employees do.
    QString group;
    QMap<int, long long> per_year;  // öre

    long long total(void) const
    {
        long long sum = 0;
        for (long long value : per_year)
        {
            sum += value;
        }
        return sum;
    }
};

// The company from its first year up to this one: every year, so that a year
// without a single invoice is visible rather than missing.
struct CompanyReport
{
    QList<CompanyReportYear> years;     // oldest first
    QList<CompanyReportSeries> customers;   // invoiced, excluding VAT
    QList<CompanyReportSeries> assignments; // the same per uppdrag
    QList<CompanyReportSeries> employees;   // bruttolön

    QList<int> yearNumbers(void) const;
    bool isEmpty(void) const { return years.isEmpty(); }
};

// `today` says which fiscal year the report ends with; the business decides
// where it starts. The company's räkenskapsår decides what a year is.
CompanyReport company_report(const Business& business, const QDate& today);

#endif // COMPANYREPORT_H
