#include <QStringList>

#include "util/amount.h"

#include "employerfees.h"

//-----------------------------------------------------------------------------
void EmployerFees::clear(void)
{
    m_years.clear();
}

//-----------------------------------------------------------------------------
void EmployerFees::setYear(int year, const EmployerFeeRate& rate)
{
    m_years.insert(year, rate);
}

//-----------------------------------------------------------------------------
void EmployerFees::removeYear(int year)
{
    m_years.remove(year);
}

//-----------------------------------------------------------------------------
bool EmployerFees::hasYear(int year) const
{
    return m_years.contains(year);
}

//-----------------------------------------------------------------------------
QList<int> EmployerFees::years(void) const
{
    return m_years.keys();
}

//-----------------------------------------------------------------------------
EmployerFeeRate EmployerFees::rate(int year) const
{
    return m_years.value(year);
}

//-----------------------------------------------------------------------------
int EmployerFees::rateFor(int year, int birth_year, bool* ok) const
{
    *ok = m_years.contains(year);
    if (!*ok)
    {
        return 0;
    }

    const EmployerFeeRate rate = m_years.value(year);
    // The lower rate is for those who had turned 66 when the year began, which
    // is a whole birth cohort: someone born in `year - 66` turns 66 during the
    // year and was 65 at its start, so the cohort is `year - 67` and earlier.
    // Skatteverket publishes the boundary that way too, as a birth year.
    const bool senior = birth_year > 0 && (year - birth_year) >= 67;
    return senior ? rate.senior : rate.standard;
}

//-----------------------------------------------------------------------------
long long employer_fee_ore(long long gross_ore, int rate)
{
    const long long scaled = gross_ore * (long long)rate;
    const long long half = scaled < 0 ? -5000 : 5000;
    return (scaled + half) / 10000;
}

//-----------------------------------------------------------------------------
bool employer_fee_parse_rates(const QList<QStringList>& rows, QMap<int, EmployerFeeRate>* rates, QString* error)
{
    rates->clear();
    for (const QStringList& row : rows)
    {
        auto cell = [&](int column) -> QString {
            return column < row.size() ? row.at(column).trimmed() : QString();
        };

        bool ok_year, ok_standard, ok_senior;
        const int year = cell(0).toInt(&ok_year);
        // A percentage has two decimals, the same as an amount of kronor has
        // öre, so it parses the same way: 31,42 becomes 3142.
        const int standard = (int)amount_parse_ore(cell(1), &ok_standard);
        const int senior = (int)amount_parse_ore(cell(2), &ok_senior);

        if (!ok_year || year < 2000 || year > 2100)
        {
            *error = "Året måste vara ett år, till exempel 2025.";
            return false;
        }
        if (rates->contains(year))
        {
            *error = QString("%1 finns med mer än en gång.").arg(year);
            return false;
        }
        if (!ok_standard || standard < 0 || standard > 10000)
        {
            *error = QString("Avgiften för %1 måste vara ett procenttal, till exempel 31,42.").arg(year);
            return false;
        }
        if (!ok_senior || senior < 0 || senior > 10000)
        {
            *error = QString("Avgiften för den som fyllt 66 år vid ingången av %1 måste vara ett procenttal, till exempel 10,21.").arg(year);
            return false;
        }

        rates->insert(year, EmployerFeeRate{ standard, senior });
    }
    return true;
}
