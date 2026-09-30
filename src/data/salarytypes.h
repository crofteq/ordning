#ifndef SALARYTYPES_H
#define SALARYTYPES_H

#include <QList>
#include <QString>

// What a line is for tax. A skattefri ersättning — traktamente up to
// Skatteverket's schablon, and the like — is paid out but is not bruttolön: no
// preliminär skatt is withheld on it, no arbetsgivaravgift is paid on it, and
// it is not part of the kontant bruttolön of the arbetsgivardeklaration, where
// it is a kryssmarkering and not an amount. Anything paid above the schablon is
// ordinary lön and belongs on a line of its own.
enum SalaryLineKind
{
    SALARY_LINE_TAXABLE = 0,    // lön: bruttolön, taxed, avgiftsbelagd
    SALARY_LINE_TAX_FREE = 1,   // skattefri ersättning: paid out, and no more
};

// A löneart: the number a salary line is booked under, what it is called on the
// lönebesked, and what it is for tax.
struct SalaryType
{
    int number;
    QString name;   // benämning, printed as it stands here
    int kind;       // SalaryLineKind
};

// A line that has not been given a löneart, which is what a line of an older
// draft has: the löneart and the benämning used to be typed by hand.
static const int SALARY_TYPE_NONE = 0;
// The lönearter in use. The numbers follow the customary ones and are never
// reused: a line is stored as its number, so changing one would change what a
// line already written means.
static const int SALARY_TYPE_MONTHLY_SALARY = 10;
static const int SALARY_TYPE_TRAKTAMENTE = 51;

// Every löneart a salary line can be given, in the order they are offered. The
// table is fixed and lives in the code rather than in the database: what a line
// is called has to be the same thing from one year to the next for a report or
// a migration to be able to say anything about it, and a number the database
// only stores means a new löneart costs no migration.
const QList<SalaryType>& salary_types(void);

// The löneart `number`, or nullptr when the table has no such number: a line
// written before the lönearter were fixed carries a code that means nothing
// here, and it is left without one rather than given the wrong one.
const SalaryType* salary_type(int number);

#endif // SALARYTYPES_H
