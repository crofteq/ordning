#include "amount.h"

#include <QLocale>

#include <cmath>
#include <cstdlib>

//-----------------------------------------------------------------------------
// Splits a scaled integer into its whole and fractional part, both positive,
// and reports whether the value was negative.
static void split(long long scaled, int scale, long long* whole, long long* fraction, bool* negative)
{
    *negative = scaled < 0;
    const long long magnitude = *negative ? -scaled : scaled;
    *whole = magnitude / scale;
    *fraction = magnitude % scale;
}

//-----------------------------------------------------------------------------
// Groups the digits of `value` in threes from the right, using a dot as
// thousand separator: 1234567 -> 1.234.567. Inserting from the right keeps the
// remaining positions valid as the string grows.
static QString grouped(long long value)
{
    QString s = QString::number(value);
    for (int i = s.size() - 3; i > 0; i -= 3)
    {
        s.insert(i, QChar('.'));
    }
    return s;
}

//-----------------------------------------------------------------------------
QString amount_format_ore(long long ore)
{
    long long kronor = 0;
    long long fraction = 0;
    bool negative = false;
    split(ore, 100, &kronor, &fraction, &negative);

    QString formatted = QString("%1,%2").arg(grouped(kronor)).arg(fraction, 2, 10, QChar('0'));
    if (negative)
    {
        formatted.prepend('-');
    }
    return formatted;
}

//-----------------------------------------------------------------------------
// Reads `text` as a decimal number and returns it multiplied by `scale`,
// rounded to the nearest whole unit.
static long long parse_scaled(const QString& text, int scale, bool* ok)
{
    QString cleaned = text.simplified();
    cleaned.remove(' ');

    // Swedish notation uses a comma for the decimals and a dot for thousands,
    // so a text with a comma in it has dots that are grouping and nothing else.
    // Without a comma the dot is taken as the decimal separator, which is what
    // people type on a numeric keypad.
    if (cleaned.contains(','))
    {
        cleaned.remove('.');
        cleaned.replace(',', '.');
    }

    if (cleaned.isEmpty())
    {
        if (ok != nullptr) *ok = false;
        return 0;
    }

    bool converted = false;
    const double value = cleaned.toDouble(&converted);
    if (ok != nullptr) *ok = converted;
    if (!converted)
    {
        return 0;
    }

    return std::llround(value * scale);
}

//-----------------------------------------------------------------------------
long long amount_parse_ore(const QString& text, bool* ok)
{
    return parse_scaled(text, 100, ok);
}

//-----------------------------------------------------------------------------
QString hours_format_hundredths(int hundredths)
{
    // Hours are shown with a single decimal, so round to whole tenths first.
    const bool negative = hundredths < 0;
    const long long magnitude = negative ? -(long long)hundredths : hundredths;
    const long long tenths = (magnitude + 5) / 10;

    QString formatted = QString("%1,%2").arg(tenths / 10).arg(tenths % 10);
    if (negative)
    {
        formatted.prepend('-');
    }
    return formatted;
}

//-----------------------------------------------------------------------------
int hours_parse_hundredths(const QString& text, bool* ok)
{
    return (int)parse_scaled(text, 100, ok);
}

//-----------------------------------------------------------------------------
long long amount_line_total_ore(long long rate_ore, int hundredths)
{
    // rate is per whole hour and hours are hundredths, so the product is a
    // hundred times too large. Round half away from zero when dividing back.
    const long long product = rate_ore * (long long)hundredths;
    const long long half = product < 0 ? -50 : 50;
    return (product + half) / 100;
}
