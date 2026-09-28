#include <QDebug>

#include "employee.h"
#include "util/personnummer.h"

//-----------------------------------------------------------------------------
Employee::Employee()
{
    m_id = -1;
    m_active = true;
}

//-----------------------------------------------------------------------------
void Employee::update(const Employee& e)
{
    m_first_name = e.m_first_name;
    m_last_name = e.m_last_name;
    m_adrsline1 = e.m_adrsline1;
    m_adrsline2 = e.m_adrsline2;
    m_personnummer = e.m_personnummer;
    m_bank_account = e.m_bank_account;
    m_active = e.m_active;
    m_tax_tables = e.m_tax_tables;
}

//-----------------------------------------------------------------------------
QString Employee::name(void) const
{
    return QString("%1 %2").arg(m_first_name, m_last_name).trimmed();
}

//-----------------------------------------------------------------------------
QString Employee::employmentNumber(void) const
{
    if (m_id <= 0)
    {
        return QString();
    }
    return QString("%1").arg(m_id, 4, 10, QLatin1Char('0'));
}

//-----------------------------------------------------------------------------
QString Employee::toString(void)
{
    QString s;
    s.append(employmentNumber() + '\n');
    s.append(name() + '\n');
    s.append(m_adrsline1 + '\n');
    s.append(m_adrsline2 + '\n');
    s.append(m_personnummer + '\n');
    s.append(m_bank_account + '\n');
    s.append(QString(m_active ? "active" : "inactive") + '\n');
    for (auto it = m_tax_tables.constBegin(); it != m_tax_tables.constEnd(); ++it)
    {
        s.append(QString("%1: table %2\n").arg(it.key()).arg(it.value()));
    }
    return s;
}

//-----------------------------------------------------------------------------
bool employee_parse_tax_tables(const QList<QPair<QString, QString>>& rows, QMap<int, int>* tax_tables, QString* error)
{
    tax_tables->clear();
    for (const QPair<QString, QString>& row : rows)
    {
        bool ok_year, ok_table;
        const int year = row.first.trimmed().toInt(&ok_year);
        const int table = row.second.trimmed().toInt(&ok_table);

        if (!ok_year || year < 2000 || year > 2100)
        {
            *error = "Året måste vara ett år, till exempel 2025.";
            return false;
        }
        if (tax_tables->contains(year))
        {
            *error = QString("%1 finns med mer än en gång.").arg(year);
            return false;
        }
        if (!ok_table || table <= 0)
        {
            *error = QString("%1 behöver en skattetabell, till exempel 34.").arg(year);
            return false;
        }
        tax_tables->insert(year, table);
    }
    return true;
}

//-----------------------------------------------------------------------------
bool employee_is_complete(const Employee& e)
{
    return !e.m_first_name.trimmed().isEmpty()
        && !e.m_last_name.trimmed().isEmpty()
        && personnummer_is_valid(e.m_personnummer);
}
