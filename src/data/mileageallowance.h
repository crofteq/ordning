#ifndef MILEAGEALLOWANCE_H
#define MILEAGEALLOWANCE_H

#include <QList>
#include <QMap>
#include <QString>

// Milersättning: what an employee may be paid per mil, skattefritt, for a
// business trip in their own car. Skatteverket publishes the schablon on their
// website rather than as open data, and it is one figure a year, so it is typed
// in on the Company tab the way the inrikes traktamente is.
//
// Only the own car is held. A förmånsbil has schabloner of its own, for the
// fuel alone, and nothing here pays them out yet. Whatever is paid above the
// schablon is ordinary lön and belongs on a line of its own.
//
// Amounts are in öre per mil, like everything else in the domain.
class MileageAllowance
{
public:
    void clear(void);

    void setYear(int year, long long per_mil_ore);
    void removeYear(int year);
    bool hasYear(int year) const;
    QList<int> years(void) const;
    // 0 for a year that has not been entered; ask hasYear() to tell that from
    // an amount of nothing.
    long long perMilOre(int year) const;

private:
    QMap<int, long long> m_years;
};

// Reads the milersättning list of the Company tab: per row the year and the
// skattefria amount per mil in kronor, as typed, taking a decimal comma. On the
// first row that does not make sense, returns false and says why in `error`.
bool mileage_parse_rates(const QList<QStringList>& rows, QMap<int, long long>* amounts, QString* error);

#endif // MILEAGEALLOWANCE_H
