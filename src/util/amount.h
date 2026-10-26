#ifndef AMOUNT_H
#define AMOUNT_H

#include <QString>

// Monetary amounts are stored as öre and hours as hundredths of an hour, both
// as integers. Storing them scaled keeps the arithmetic exact: an invoice line
// is rate * hours, and a value that never becomes a double never drifts.
//
// The helpers here convert between those integers and the text the user types
// and reads. Everything is formatted Swedish style: comma for the decimal
// separator, dot for thousands.

// 12345 -> "123,45". Grouped in threes at and above 1000 kronor.
QString amount_format_ore(long long ore);

// Parses "1 234,56", "1234.56" or "1234" into öre. Accepts both separators and
// ignores spaces. Sets *ok to false when the text is not a number.
long long amount_parse_ore(const QString& text, bool* ok = nullptr);

// 750 -> "7,5". Rounded to a single decimal, which is the precision shown on
// the invoice.
QString hours_format_hundredths(int hundredths);

// Parses "7,5", "7.5" or "7" into hundredths of an hour.
int hours_parse_hundredths(const QString& text, bool* ok = nullptr);

// The value of `hundredths` hours at `rate_ore` per hour, in öre, rounded to
// the nearest öre.
long long amount_line_total_ore(long long rate_ore, int hundredths);

#endif // AMOUNT_H
