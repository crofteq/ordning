#include <QDate>

#include "personnummer.h"

//-----------------------------------------------------------------------------
// Luhn over the first nine digits of `digits`, weights 2, 1, 2, 1, ...
static int luhn_check_digit(const QString& digits)
{
    int sum = 0;
    for (int i = 0; i < 9; i++)
    {
        int d = digits.at(i).digitValue() * ((i % 2 == 0) ? 2 : 1);
        sum += (d > 9) ? d - 9 : d;
    }
    return (10 - (sum % 10)) % 10;
}

//-----------------------------------------------------------------------------
QString personnummer_normalize(const QString& text)
{
    QString s = text.trimmed();

    QChar separator = '-';
    if (s.contains('+'))
    {
        separator = '+';
    }
    s.remove('-');
    s.remove('+');

    if (s.size() == 12)
    {
        s = s.mid(2);
    }
    if (s.size() != 10)
    {
        return QString();
    }
    for (const QChar& c : s)
    {
        if (!c.isDigit())
        {
            return QString();
        }
    }

    // The century is not part of the stored form, so any leap year will do for
    // checking that the month and day exist. A samordningsnummer adds 60 to
    // the day.
    const int month = s.mid(2, 2).toInt();
    int day = s.mid(4, 2).toInt();
    if (day > 60)
    {
        day -= 60;
    }
    if (!QDate::isValid(2000, month, day))
    {
        return QString();
    }

    if (luhn_check_digit(s) != s.at(9).digitValue())
    {
        return QString();
    }

    return s.left(6) + separator + s.mid(6);
}

//-----------------------------------------------------------------------------
bool personnummer_is_valid(const QString& text)
{
    return !personnummer_normalize(text).isEmpty();
}

//-----------------------------------------------------------------------------
int personnummer_birth_year(const QString& pnr, int reference_year)
{
    const QString normalized = personnummer_normalize(pnr);
    if (normalized.isEmpty())
    {
        return 0;
    }

    const int two_digits = normalized.left(2).toInt();
    // The latest year ending in those digits that is not in the future.
    int year = (reference_year / 100) * 100 + two_digits;
    if (year > reference_year)
    {
        year -= 100;
    }
    // A '+' means the person had already turned 100 when the number was
    // written, so it is the century before that.
    if (normalized.contains('+'))
    {
        year -= 100;
    }
    return year;
}
