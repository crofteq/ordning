#ifndef TRAKTAMENTE_H
#define TRAKTAMENTE_H

#include <QByteArray>
#include <QList>
#include <QMap>
#include <QString>

// Traktamente: what an employee may be paid for the increased living costs of
// a business trip, and the amount a payment has to be measured against to know
// how much of it is skattefritt.
//
// The two halves have different shapes, so they are held apart:
//
//  - Inrikes is one amount a year, helt dagtraktamente. Skatteverket publishes
//    it on their website rather than as open data, and a consultancy's
//    domestic trips are few, so it is typed in on the Company tab the way the
//    arbetsgivaravgifter are.
//  - Utrikes is Skatteverket's normalbelopp per country, some 150 rows a year.
//    They are an allmänt råd rather than law, but they are what a skattefritt
//    utrikes traktamente is measured against, so they are imported from a CSV.
//
// Amounts are in öre, like everything else in the domain. Halvdag and
// nattschablon are not stored: they are derived from the whole day, and belong
// with the salary lines that will use them.

// One country's normalbelopp for a whole day.
struct TraktamenteCountry
{
    QString country;        // as the file spells it
    long long amount = 0;   // öre

    bool operator==(const TraktamenteCountry& o) const
    {
        return country == o.country && amount == o.amount;
    }
};

// Parses a CSV of Skatteverket's normalbelopp, one row per country.
//
// A country given no amount of its own but pointed at another — the published
// table says "Se Danmark" for Grönland and "Se Myanmar" for the older name
// Burma — is given that country's normalbelopp. The country pointed at may
// stand anywhere in the file, so those are resolved once all of it has been
// read; one that points at a country the file does not hold fails the import.
//
// The columns are found from the header row rather than by position, so the
// file may carry more of them than these: a country column ("Land", "Land/
// Område" or "Country") and an amount column ("Normalbelopp", "Belopp" or
// "Traktamente") are needed, and a year column ("År" or "Year") is used when
// it is there, the file then having to hold exactly one year. Without a year
// column `year` is left 0 and the caller has to say which year the file is
// for. Fields are separated by whichever of ';' and ',' the header uses, and
// the file is read as UTF-8 or, failing that, as ISO-8859-1. On failure
// returns false and says why in `error`, naming the line.
bool traktamente_parse_csv(const QByteArray& data, int* year, QList<TraktamenteCountry>* countries, QString* error);

// Reads the inrikes list of the Company tab: per row the year and the helt
// dagtraktamente in kronor, as typed, taking a decimal comma. On the first row
// that does not make sense, returns false and says why in `error`.
bool traktamente_parse_domestic(const QList<QStringList>& rows, QMap<int, long long>* amounts, QString* error);

class Traktamente
{
public:
    void clear(void);

    // Inrikes: helt dagtraktamente in öre, per year.
    void setDomestic(int year, long long amount_ore);
    void removeDomestic(int year);
    bool hasDomestic(int year) const;
    QList<int> domesticYears(void) const;
    // 0 for a year that has not been entered; ask hasDomestic() to tell that
    // from an amount of nothing.
    long long domesticOre(int year) const;

    // Utrikes: the normalbelopp of one year. Replaces whatever was stored for
    // that year, since each import is one whole year.
    void setForeign(int year, const QList<TraktamenteCountry>& countries);
    void removeForeign(int year);
    bool hasForeign(int year) const;
    QList<int> foreignYears(void) const;
    // In the order they were imported, which is the order the file listed them.
    QList<TraktamenteCountry> countries(int year) const;

    // The normalbelopp of `country` in `year`, in öre. The name is matched
    // without regard to case or surrounding and repeated spaces, so a country
    // typed by hand still finds its row. Sets `ok` to false when the year has
    // not been imported or holds no such country.
    long long foreignOre(int year, const QString& country, bool* ok) const;

private:
    QMap<int, long long> m_domestic;
    QMap<int, QList<TraktamenteCountry>> m_foreign;
};

#endif // TRAKTAMENTE_H
