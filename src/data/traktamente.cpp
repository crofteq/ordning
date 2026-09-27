#include <QStringDecoder>

#include "util/amount.h"

#include "traktamente.h"

// A country name is matched without regard to case or to surrounding and
// repeated spaces: the name on a salary line is typed by hand, and the same
// country is spelled with an extra space often enough.
static QString normalized(const QString& name)
{
    return name.simplified().toLower();
}

//-----------------------------------------------------------------------------
// One line split into fields, honouring double quotes so that a country name
// holding the separator survives: "Korea, Republiken";656. A doubled quote
// inside a quoted field is one quote.
static QStringList csv_split(const QString& line, QChar separator)
{
    QStringList fields;
    QString field;
    bool quoted = false;

    for (int i = 0; i < line.size(); i++)
    {
        const QChar c = line.at(i);
        if (quoted)
        {
            if (c != '"')
            {
                field.append(c);
            }
            else if (i + 1 < line.size() && line.at(i + 1) == '"')
            {
                field.append('"');
                i++;
            }
            else
            {
                quoted = false;
            }
        }
        else if (c == '"')
        {
            quoted = true;
        }
        else if (c == separator)
        {
            fields.append(field.trimmed());
            field.clear();
        }
        else
        {
            field.append(c);
        }
    }
    fields.append(field.trimmed());
    return fields;
}

//-----------------------------------------------------------------------------
// Where `name` is in `countries`, or -1. The name is matched the way a country
// typed by hand is.
static int index_of_country(const QList<TraktamenteCountry>& countries, const QString& name)
{
    const QString wanted = normalized(name);
    for (int i = 0; i < countries.size(); i++)
    {
        if (normalized(countries.at(i).country) == wanted)
        {
            return i;
        }
    }
    return -1;
}

//-----------------------------------------------------------------------------
// Some countries are given no normalbelopp of their own but are pointed at
// another: Skatteverket writes "Se Danmark" for Grönland, and "Se Myanmar" for
// Burma, which is that country's older name. Returns the name pointed at, or an
// empty string when the field is an amount like any other. Only the amount
// column is ever read this way, so a country whose own name begins with "Se"
// is no trouble.
static QString country_reference(const QString& field)
{
    const QString text = field.simplified();
    if (!text.startsWith("se ", Qt::CaseInsensitive))
    {
        return QString();
    }
    return text.mid(3).trimmed();
}

//-----------------------------------------------------------------------------
// The column whose header is one of `exact`, or failing that the first one
// holding any of `loose`. Exact before loose, so that a file carrying both
// "Land" and "Landskod" picks the country and not its code. -1 when there is
// none.
static int column_of(const QStringList& header, const QStringList& exact, const QStringList& loose)
{
    for (int i = 0; i < header.size(); i++)
    {
        if (exact.contains(normalized(header.at(i))))
        {
            return i;
        }
    }
    for (int i = 0; i < header.size(); i++)
    {
        const QString cell = normalized(header.at(i));
        for (const QString& part : loose)
        {
            if (cell.contains(part))
            {
                return i;
            }
        }
    }
    return -1;
}

//-----------------------------------------------------------------------------
bool traktamente_parse_csv(const QByteArray& data, int* year, QList<TraktamenteCountry>* countries, QString* error)
{
    countries->clear();
    *year = 0;

    // Skatteverket publishes in ISO-8859-1. Anything that is valid UTF-8 is
    // taken as UTF-8, which also covers a file that has been converted or
    // saved out of a spreadsheet.
    QString text;
    {
        QStringDecoder utf8(QStringDecoder::Utf8);
        text = utf8(data);
        if (utf8.hasError())
        {
            text = QString::fromLatin1(data);
        }
    }

    int column_country = -1;
    int column_amount = -1;
    int column_year = -1;
    QChar separator = ';';

    // Countries pointed at another one, by their place in `countries`: the name
    // they point at and the line they stand on. They are resolved once the
    // whole file has been read, since the country pointed at may come further
    // down — Burma points at Myanmar.
    QMap<int, QPair<QString, int>> references;

    const QStringList lines = text.split('\n');
    for (int i = 0; i < lines.size(); i++)
    {
        const QString line = lines.at(i).trimmed();
        const int lineno = i + 1;
        if (line.isEmpty())
        {
            continue;
        }

        // The separator is whichever of the two the header holds more of, and
        // the rest of the file is read with that one: a note under the table
        // then falls out as a line with a single field rather than being read
        // as a country with an amount.
        const QChar candidate = line.count(';') >= line.count(',') ? QChar(';') : QChar(',');
        const QStringList fields = csv_split(line, column_country < 0 ? candidate : separator);

        if (column_country < 0)
        {
            // Still looking for the header. A file may open with a title line,
            // so every line is tried until one names the columns.
            const int country = column_of(fields, { "land", "land/område", "country" }, { "land", "område", "omrade", "country" });
            const int amount = column_of(fields, { "normalbelopp", "belopp", "traktamente" }, { "normalbelopp", "belopp", "traktamente", "amount" });
            if (country >= 0 && amount >= 0 && country != amount)
            {
                column_country = country;
                column_amount = amount;
                column_year = column_of(fields, { "år", "ar", "year" }, {});
                separator = candidate;
            }
            continue;
        }

        // A note or a source line under the table carries no separator.
        if (fields.size() < 2)
        {
            continue;
        }

        const int needed = qMax(column_year, qMax(column_country, column_amount)) + 1;
        if (fields.size() < needed)
        {
            *error = QString("Line %1: expected at least %2 fields, found %3.").arg(lineno).arg(needed).arg(fields.size());
            return false;
        }

        TraktamenteCountry c;
        c.country = fields.at(column_country);
        if (c.country.isEmpty())
        {
            continue;
        }

        const QString reference = country_reference(fields.at(column_amount));
        if (!reference.isEmpty())
        {
            references.insert(countries->size(), qMakePair(reference, lineno));
        }
        else
        {
            bool ok_amount = false;
            c.amount = amount_parse_ore(fields.at(column_amount), &ok_amount);
            if (!ok_amount || c.amount <= 0)
            {
                *error = QString("Line %1: \"%2\" is not a normalbelopp for %3.").arg(lineno).arg(fields.at(column_amount), c.country);
                return false;
            }
        }

        if (column_year >= 0)
        {
            bool ok_year = false;
            const int row_year = fields.at(column_year).toInt(&ok_year);
            if (!ok_year || row_year < 2000 || row_year > 2100)
            {
                *error = QString("Line %1: \"%2\" is not a year such as 2026.").arg(lineno).arg(fields.at(column_year));
                return false;
            }
            if (*year == 0)
            {
                *year = row_year;
            }
            else if (row_year != *year)
            {
                *error = QString("Line %1: the year %2 differs from %3 earlier in the file. Import one year at a time.").arg(lineno).arg(row_year).arg(*year);
                return false;
            }
        }

        for (const TraktamenteCountry& seen : *countries)
        {
            if (normalized(seen.country) == normalized(c.country))
            {
                *error = QString("Line %1: %2 is listed more than once.").arg(lineno).arg(c.country);
                return false;
            }
        }

        countries->append(c);
    }

    if (column_country < 0)
    {
        *error = "The file has no header naming a country column (\"Land\") and an amount column (\"Normalbelopp\").";
        return false;
    }
    if (countries->isEmpty())
    {
        *error = "The file holds no countries.";
        return false;
    }

    // Every country is known now, so the ones pointing at another can be given
    // its amount. A name pointed at may itself point on, so each is followed
    // until it lands on an amount; one that leads nowhere, or round in a
    // circle, fails the import rather than leaving a country at nothing.
    for (auto it = references.constBegin(); it != references.constEnd(); ++it)
    {
        int at = it.key();
        int steps = 0;
        while (references.contains(at))
        {
            const QPair<QString, int>& pointer = references.value(at);
            const int next = index_of_country(*countries, pointer.first);
            if (next < 0)
            {
                *error = QString("Line %1: %2 points at %3, which the file does not hold.")
                             .arg(pointer.second).arg(countries->at(at).country, pointer.first);
                return false;
            }
            if (next == at || ++steps > references.size())
            {
                *error = QString("Line %1: %2 points at %3, and round it comes back again.")
                             .arg(pointer.second).arg(countries->at(at).country, pointer.first);
                return false;
            }
            at = next;
        }
        (*countries)[it.key()].amount = countries->at(at).amount;
    }

    return true;
}

//-----------------------------------------------------------------------------
bool traktamente_parse_domestic(const QList<QStringList>& rows, QMap<int, long long>* amounts, QString* error)
{
    amounts->clear();
    for (const QStringList& row : rows)
    {
        auto cell = [&](int column) -> QString {
            return column < row.size() ? row.at(column).trimmed() : QString();
        };

        bool ok_year = false;
        bool ok_amount = false;
        const int year = cell(0).toInt(&ok_year);
        const long long amount = amount_parse_ore(cell(1), &ok_amount);

        if (!ok_year || year < 2000 || year > 2100)
        {
            *error = "The year must be a year such as 2026.";
            return false;
        }
        if (amounts->contains(year))
        {
            *error = QString("%1 is listed more than once.").arg(year);
            return false;
        }
        // Helt dagtraktamente inrikes is 0,5 % of prisbasbeloppet rounded to
        // the nearest tio kronor, so a few hundred kronor. The bound is wide
        // enough not to argue with a future year and narrow enough to catch
        // öre typed as kronor.
        if (!ok_amount || amount <= 0 || amount > 1000000)
        {
            *error = QString("The traktamente for %1 must be an amount in kronor, such as 290.").arg(year);
            return false;
        }

        amounts->insert(year, amount);
    }
    return true;
}

//-----------------------------------------------------------------------------
void Traktamente::clear(void)
{
    m_domestic.clear();
    m_foreign.clear();
}

//-----------------------------------------------------------------------------
void Traktamente::setDomestic(int year, long long amount_ore)
{
    m_domestic.insert(year, amount_ore);
}

//-----------------------------------------------------------------------------
void Traktamente::removeDomestic(int year)
{
    m_domestic.remove(year);
}

//-----------------------------------------------------------------------------
bool Traktamente::hasDomestic(int year) const
{
    return m_domestic.contains(year);
}

//-----------------------------------------------------------------------------
QList<int> Traktamente::domesticYears(void) const
{
    return m_domestic.keys();
}

//-----------------------------------------------------------------------------
long long Traktamente::domesticOre(int year) const
{
    return m_domestic.value(year, 0);
}

//-----------------------------------------------------------------------------
void Traktamente::setForeign(int year, const QList<TraktamenteCountry>& countries)
{
    m_foreign.insert(year, countries);
}

//-----------------------------------------------------------------------------
void Traktamente::removeForeign(int year)
{
    m_foreign.remove(year);
}

//-----------------------------------------------------------------------------
bool Traktamente::hasForeign(int year) const
{
    return m_foreign.contains(year);
}

//-----------------------------------------------------------------------------
QList<int> Traktamente::foreignYears(void) const
{
    return m_foreign.keys();
}

//-----------------------------------------------------------------------------
QList<TraktamenteCountry> Traktamente::countries(int year) const
{
    return m_foreign.value(year);
}

//-----------------------------------------------------------------------------
long long Traktamente::foreignOre(int year, const QString& country, bool* ok) const
{
    *ok = false;
    auto it = m_foreign.constFind(year);
    if (it == m_foreign.constEnd())
    {
        return 0;
    }

    const int index = index_of_country(it.value(), country);
    if (index < 0)
    {
        return 0;
    }
    *ok = true;
    return it.value().at(index).amount;
}
