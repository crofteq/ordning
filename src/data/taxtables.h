#ifndef TAXTABLES_H
#define TAXTABLES_H

#include <QByteArray>
#include <QList>
#include <QMap>
#include <QString>

// One income bracket of one skattetabell, from Skatteverket's monthly table
// ("Antal dgr" 30B or 30%). Only Kolumn 1 is kept: salary for someone under 66.
struct TaxTableRow
{
    int table_number = 0;
    int income_from = 0;    // kronor, inclusive
    int income_to = 0;      // kronor, inclusive; 0 for the open-ended top bracket
    bool percent = false;   // 30%: `tax` is a percentage of the income
    int tax = 0;            // 30B: kronor

    bool operator==(const TaxTableRow& o) const
    {
        return table_number == o.table_number && income_from == o.income_from
            && income_to == o.income_to && percent == o.percent && tax == o.tax;
    }
};

// Parses Skatteverket's "Skattetabeller för månadslön" CSV, either as they
// publish it (ISO-8859-1) or re-saved as UTF-8. The file must hold exactly one
// year. On failure returns false and says why in `error`, naming the line.
bool taxtable_parse_csv(const QByteArray& data, int* year, QList<TaxTableRow>* rows, QString* error);

// The imported tax tables, per year.
class TaxTables
{
public:
    void clear(void);

    // Replaces whatever was stored for `year`.
    void setYear(int year, const QList<TaxTableRow>& rows);
    void removeYear(int year);
    bool hasYear(int year) const;
    QList<int> years(void) const;
    QList<TaxTableRow> rows(int year) const;

    // The preliminary tax, in öre, on a monthly salary of `taxable_ore` for
    // skattetabell `table` in `year`. The öre of the salary are dropped before
    // the bracket is looked up, and a percentage is rounded down to whole
    // kronor, so the result is always whole kronor. Sets `ok` to false when
    // the year or table has not been imported.
    long long taxOre(int year, int table, long long taxable_ore, bool* ok) const;

private:
    QMap<int, QList<TaxTableRow>> m_years;
};

#endif // TAXTABLES_H
