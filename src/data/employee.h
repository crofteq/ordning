#ifndef EMPLOYEE_H
#define EMPLOYEE_H

#include <QList>
#include <QMap>
#include <QPair>
#include <QString>

// Holds exactly what the salary statement prints about the employee.
class Employee
{
public:
    Employee();

    QString m_first_name;
    QString m_last_name;
    QString m_adrsline1;
    QString m_adrsline2;
    QString m_personnummer; // YYMMDD-XXXX, see util/personnummer.h
    QString m_bank_account;
    bool m_active;
    // The skattetabell from the employee's skattebesked, keyed by year.
    // Column 1 of the table is always used.
    QMap<int, int> m_tax_tables;

    // Copies every field of `e`, including the tax tables. The id is left alone.
    void update(const Employee& e);
    QString toString(void);

    // "Förnamn Efternamn", as printed. Kept as two fields so that employees
    // can be sorted by surname; a name split later could only be guessed at.
    QString name(void) const;

    // The anställningsnummer, "0001" for id 1. Empty until the employee has
    // been stored and given an id.
    QString employmentNumber(void) const;

    int id(void) const { return m_id; }
    void setId(int id) { m_id = id; }

private:
    int m_id = -1;
};

// Whether `e` has what the salary statement cannot do without: a first name,
// a last name and a valid personnummer.
bool employee_is_complete(const Employee& e);

// Reads the skattetabell list of the edit page: one (year, table) pair of
// texts per row. The year must be 2000-2100 and listed once, the table a
// positive number. On the first row that does not make sense, returns false
// and says why in `error`.
bool employee_parse_tax_tables(const QList<QPair<QString, QString>>& rows, QMap<int, int>* tax_tables, QString* error);

#endif // EMPLOYEE_H
