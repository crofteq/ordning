#include "util/amount.h"

#include "mileageallowance.h"

//-----------------------------------------------------------------------------
bool mileage_parse_rates(const QList<QStringList>& rows, QMap<int, long long>* amounts, QString* error)
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
            *error = "Året måste vara ett år, till exempel 2026.";
            return false;
        }
        if (amounts->contains(year))
        {
            *error = QString("%1 finns med mer än en gång.").arg(year);
            return false;
        }
        // The schablon for an own car is a couple of tens of kronor per mil.
        // The bound is wide enough not to argue with a future year and narrow
        // enough to catch öre typed as kronor.
        if (!ok_amount || amount <= 0 || amount > 50000)
        {
            *error = QString("Milersättningen för %1 måste vara ett belopp i kronor per mil, till exempel 25.").arg(year);
            return false;
        }

        amounts->insert(year, amount);
    }
    return true;
}

//-----------------------------------------------------------------------------
void MileageAllowance::clear(void)
{
    m_years.clear();
}

//-----------------------------------------------------------------------------
void MileageAllowance::setYear(int year, long long per_mil_ore)
{
    m_years.insert(year, per_mil_ore);
}

//-----------------------------------------------------------------------------
void MileageAllowance::removeYear(int year)
{
    m_years.remove(year);
}

//-----------------------------------------------------------------------------
bool MileageAllowance::hasYear(int year) const
{
    return m_years.contains(year);
}

//-----------------------------------------------------------------------------
QList<int> MileageAllowance::years(void) const
{
    return m_years.keys();
}

//-----------------------------------------------------------------------------
long long MileageAllowance::perMilOre(int year) const
{
    return m_years.value(year, 0);
}
