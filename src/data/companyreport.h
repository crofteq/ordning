#ifndef COMPANYREPORT_H
#define COMPANYREPORT_H

#include <QList>
#include <QMap>
#include <QString>

#include <QDate>

#include "business.h"

// What a stretch of time came to: a month, a year, or the first months of a
// year. Money in öre, hours in hundredths.
struct CompanyReportFigures
{
    long long invoiced = 0;         // excluding VAT, by invoice date
    long long salary_gross = 0;     // bruttolön, by payment date
    long long employer_fee = 0;
    long long tax = 0;              // avdragen skatt
    long long tax_free = 0;         // skattefria ersättningar paid out
    int hours = 0;                  // debiterade, all four kinds, a credit invoice taking its own back
    int invoices = 0;               // not counting the credit invoices
    int credit_invoices = 0;

    // What the salaries cost the company: the gross, the fee on top, and the
    // skattefria ersättningar, which are paid out of the same pocket even
    // though they are no part of the bruttolön.
    long long salary_cost(void) const { return salary_gross + employer_fee + tax_free; }

    // What is left of the invoiced amount once the salaries are paid. It is
    // not a profit: the company has other costs Ordning knows nothing about.
    long long after_salaries(void) const { return invoiced - salary_cost(); }

    void add(const CompanyReportFigures& other);
};

// One month of a year.
struct CompanyReportMonth : CompanyReportFigures
{
    int month = 0;                  // 1-12, the calendar month
    int year = 0;                   // the calendar year it falls in
};

// One year of the company.
struct CompanyReportYear : CompanyReportFigures
{
    int year = 0;                       // the year the fiscal year began in
    QString name;                       // "2025" or "2025/26"
    QList<CompanyReportMonth> months;   // always twelve, in fiscal order

    // The first `count` months of the year added up, which is what a year
    // still running is compared with.
    CompanyReportFigures firstMonths(int count) const;
};

// A customer, an uppdrag, an employee or a consultant, and what they came to
// each year: öre, or hundredths of an hour for a consultant.
struct CompanyReportSeries
{
    QString name;
    // Who the line belongs under, when the lines are grouped: an uppdrag is
    // one of its customer's, and the customer heads the rows of its own. Empty
    // when the lines stand on their own, as customers and employees do.
    QString group;
    QMap<int, long long> per_year;
    // The same, counting only as many months of each year as the last year of
    // the report has begun: what a year still running is compared with.
    QMap<int, long long> to_date;

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
    QList<CompanyReportSeries> consultants; // debiterade timmar

    int start_month = 1;    // the räkenskapsår, 1 for the calendar year
    // Whether the last year is still running, and so shows only part of what
    // it will come to.
    bool running = false;
    // How many months of the last year are over, 1-12. A running year holds
    // only those, and the year before it is compared over the same months: a
    // month still under way would be set against the whole of it.
    int months_complete = 12;

    QList<int> yearNumbers(void) const;
    bool isEmpty(void) const { return years.isEmpty(); }

    // The months of the last year that are over, as a reader says them:
    // "jan–sep", or "maj–sep" for a year that begins in May.
    QString elapsedMonths(void) const;

    // The largest customer's share of what was invoiced in `year`, in
    // hundredths of a percent, over the whole year or the months to date.
    // 0 when nothing was invoiced.
    int largestCustomerShare(int year, bool to_date) const;
};

// `today` says which fiscal year the report ends with, and how far into it
// the report goes: the months that are over, a month being over on its last
// day. A year none of whose months is over yet is left out, and the report
// ends with the year before. The business decides where it starts, and the
// company's räkenskapsår decides what a year is.
CompanyReport company_report(const Business& business, const QDate& today);

#endif // COMPANYREPORT_H
