#ifndef FISCALYEAR_H
#define FISCALYEAR_H

#include <QDate>
#include <QString>

// A company's räkenskapsår is either the calendar year or a broken one. A
// broken year begins on the first day of a month and runs twelve months, so
// the month it starts in is all that has to be kept. 1 is January, and that
// is the calendar year.
//
// A fiscal year is named after the year it begins in: 2025 starting in May is
// "2025/26", and the calendar year 2025 is just "2025".

// The fiscal year `date` belongs to, that is, the year it began in.
int fiscal_year_of(const QDate& date, int start_month);

// The first and the last day of the fiscal year that began in `year`.
QDate fiscal_year_first(int year, int start_month);
QDate fiscal_year_last(int year, int start_month);

// What to call it: "2025" or "2025/26".
QString fiscal_year_name(int year, int start_month);

// The calendar months of the fiscal year, in the order they come: 1..12 for
// the calendar year, 5..12 then 1..4 for one starting in May.
QList<int> fiscal_year_months(int start_month);

// The calendar year that `month` (1-12) falls in within the fiscal year that
// began in `year`.
int fiscal_year_calendar_year(int year, int start_month, int month);

#endif // FISCALYEAR_H
