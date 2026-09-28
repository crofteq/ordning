#ifndef EMPLOYERFEES_H
#define EMPLOYERFEES_H

#include <QList>
#include <QMap>
#include <QString>

// The arbetsgivaravgifter of one year, as percentages in hundredths of a
// percent: 31,42 % is 3142. Skatteverket publishes them on their website
// rather than as open data, so they are typed in, one year at a time.
struct EmployerFeeRate
{
    int standard = 0;
    int senior = 0;     // for someone who had turned 66 at the start of the year

    bool operator==(const EmployerFeeRate& o) const
    {
        return standard == o.standard && senior == o.senior;
    }
};

class EmployerFees
{
public:
    void clear(void);

    void setYear(int year, const EmployerFeeRate& rate);
    void removeYear(int year);
    bool hasYear(int year) const;
    QList<int> years(void) const;
    EmployerFeeRate rate(int year) const;

    // The percentage that applies to someone born in `birth_year` when the
    // salary is paid out in `year`, in hundredths of a percent. Sets `ok` to
    // false when that year has not been entered.
    int rateFor(int year, int birth_year, bool* ok) const;

private:
    QMap<int, EmployerFeeRate> m_years;
};

// The arbetsgivaravgift on `gross_ore` at `rate` hundredths of a percent, in
// öre, rounded half away from zero.
long long employer_fee_ore(long long gross_ore, int rate);

// Reads the rate list of the Company tab: per row the year and the two
// percentages as typed. A percentage takes a decimal comma. On the first row
// that does not make sense, returns false and says why in `error`.
bool employer_fee_parse_rates(const QList<QStringList>& rows, QMap<int, EmployerFeeRate>* rates, QString* error);

#endif // EMPLOYERFEES_H
