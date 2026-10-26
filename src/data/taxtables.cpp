#include <QStringDecoder>

#include "taxtables.h"

// Number of fields up to and including Kolumn 1:
// År;Antal dgr;Tabellnr;Inkomst fr.o.m.;Inkomst t.o.m.;Kolumn 1
#define CSV_FIELDS_NEEDED   6

//-----------------------------------------------------------------------------
bool taxtable_parse_csv(const QByteArray& data, int* year, QList<TaxTableRow>* rows, QString* error)
{
    rows->clear();
    *year = 0;

    // Skatteverket publishes the file in ISO-8859-1. Anything that is valid
    // UTF-8 is taken as UTF-8, which also covers a file that was converted.
    QString text;
    {
        QStringDecoder utf8(QStringDecoder::Utf8);
        text = utf8(data);
        if (utf8.hasError())
        {
            text = QString::fromLatin1(data);
        }
    }

    const QStringList lines = text.split('\n');
    for (int i = 0; i < lines.size(); i++)
    {
        const QString line = lines.at(i).trimmed();
        const int lineno = i + 1;

        // The header, and any blank line.
        if (line.isEmpty() || !line.at(0).isDigit())
        {
            continue;
        }

        const QStringList f = line.split(';');
        if (f.size() < CSV_FIELDS_NEEDED)
        {
            *error = QString("Rad %1: förväntade minst %2 kolumner, hittade %3.").arg(lineno).arg(CSV_FIELDS_NEEDED).arg(f.size());
            return false;
        }

        // Only the monthly tables. Other periods would be 14B/14% and so on.
        const QString kind = f.at(1).trimmed();
        if (kind != "30B" && kind != "30%")
        {
            continue;
        }

        bool ok_year, ok_table, ok_from, ok_to, ok_tax;
        const int row_year = f.at(0).trimmed().toInt(&ok_year);
        TaxTableRow r;
        r.table_number = f.at(2).trimmed().toInt(&ok_table);
        r.income_from = f.at(3).trimmed().toInt(&ok_from);
        const QString to = f.at(4).trimmed();
        r.income_to = to.isEmpty() ? 0 : to.toInt(&ok_to);
        if (to.isEmpty())
        {
            ok_to = true;
        }
        r.percent = (kind == "30%");
        r.tax = f.at(5).trimmed().toInt(&ok_tax);

        if (!ok_year || !ok_table || !ok_from || !ok_to || !ok_tax)
        {
            *error = QString("Rad %1: här förväntades ett tal.").arg(lineno);
            return false;
        }
        if (*year == 0)
        {
            *year = row_year;
        }
        else if (row_year != *year)
        {
            *error = QString("Rad %1: året %2 skiljer sig från %3 tidigare i filen. Importera ett år i taget.").arg(lineno).arg(row_year).arg(*year);
            return false;
        }

        rows->append(r);
    }

    if (rows->isEmpty())
    {
        *error = "Filen innehåller inga rader för månadslön (30B eller 30 %).";
        return false;
    }
    return true;
}

//-----------------------------------------------------------------------------
void TaxTables::clear(void)
{
    m_years.clear();
}

//-----------------------------------------------------------------------------
void TaxTables::setYear(int year, const QList<TaxTableRow>& rows)
{
    m_years.insert(year, rows);
}

//-----------------------------------------------------------------------------
void TaxTables::removeYear(int year)
{
    m_years.remove(year);
}

//-----------------------------------------------------------------------------
bool TaxTables::hasYear(int year) const
{
    return m_years.contains(year);
}

//-----------------------------------------------------------------------------
QList<int> TaxTables::years(void) const
{
    return m_years.keys();
}

//-----------------------------------------------------------------------------
QList<TaxTableRow> TaxTables::rows(int year) const
{
    return m_years.value(year);
}

//-----------------------------------------------------------------------------
long long TaxTables::taxOre(int year, int table, long long taxable_ore, bool* ok) const
{
    *ok = false;
    auto it = m_years.constFind(year);
    if (it == m_years.constEnd())
    {
        return 0;
    }

    const long long income = taxable_ore / 100; // whole kronor, öre dropped
    bool table_found = false;
    for (const TaxTableRow& r : it.value())
    {
        if (r.table_number != table)
        {
            continue;
        }
        table_found = true;
        if (income >= r.income_from && (r.income_to == 0 || income <= r.income_to))
        {
            *ok = true;
            if (r.percent)
            {
                return (income * r.tax / 100) * 100;
            }
            return (long long)r.tax * 100;
        }
    }

    // The tables start at 1 kr, so a salary of nothing carries no tax.
    if (table_found && income < 1)
    {
        *ok = true;
    }
    return 0;
}
