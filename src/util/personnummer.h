#ifndef PERSONNUMMER_H
#define PERSONNUMMER_H

#include <QString>

// Swedish personal identity numbers. The salary statement prints the ten-digit
// form, YYMMDD-XXXX (with '+' instead of '-' for someone who has turned 100),
// so that is the form they are stored in.

// Returns `text` as YYMMDD-XXXX, or an empty string if it is not a valid
// personnummer. Accepts ten or twelve digits, with or without the separator;
// a twelve-digit number loses its century. The last digit must be the Luhn
// check digit of the nine before it, and the date part must be a real date
// (a samordningsnummer, day + 60, is accepted too).
QString personnummer_normalize(const QString& text);

// True when personnummer_normalize() would accept `text`.
bool personnummer_is_valid(const QString& text);

// The year `pnr` was born, worked out from `reference_year`: the stored form
// has no century, so the year is the one that makes the person younger than
// 100 at that point, or older when the number carries a '+'. Returns 0 for a
// personnummer that is not valid.
int personnummer_birth_year(const QString& pnr, int reference_year);

#endif // PERSONNUMMER_H
