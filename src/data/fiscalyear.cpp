#include "fiscalyear.h"

//-----------------------------------------------------------------------------
int fiscal_year_of(const QDate& date, int start_month)
{
    if (!date.isValid())
    {
        return 0;
    }
    // Before the starting month the date still belongs to the year that began
    // the calendar year before.
    return date.month() >= start_month ? date.year() : date.year() - 1;
}

//-----------------------------------------------------------------------------
QDate fiscal_year_first(int year, int start_month)
{
    return QDate(year, start_month, 1);
}

//-----------------------------------------------------------------------------
QDate fiscal_year_last(int year, int start_month)
{
    return fiscal_year_first(year, start_month).addYears(1).addDays(-1);
}

//-----------------------------------------------------------------------------
QString fiscal_year_name(int year, int start_month)
{
    if (start_month == 1)
    {
        return QString::number(year);
    }
    return QString("%1/%2").arg(year).arg((year + 1) % 100, 2, 10, QChar('0'));
}

//-----------------------------------------------------------------------------
QList<int> fiscal_year_months(int start_month)
{
    QList<int> months;
    for (int i = 0; i < 12; i++)
    {
        months.append((start_month - 1 + i) % 12 + 1);
    }
    return months;
}

//-----------------------------------------------------------------------------
int fiscal_year_calendar_year(int year, int start_month, int month)
{
    return month >= start_month ? year : year + 1;
}
