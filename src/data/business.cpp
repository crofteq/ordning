#include <QDebug>

#include <algorithm>

#include "business.h"

Business::Business() {}

//-----------------------------------------------------------------------------
void Business::clear(void)
{
    m_companyinfo.clear();
    m_customers.clear();
    m_employees.clear();
    m_invoicedrafts.clear();
    m_invoices.clear();
    m_taxtables.clear();
    m_employerfees.clear();
    m_traktamente.clear();
    m_salarydrafts.clear();
    m_salaryslips.clear();
}

//-----------------------------------------------------------------------------
bool Business::employeeInUse(int employee_id) const
{
    if (m_salaryslips.hasEmployee(employee_id))
    {
        return true;
    }
    for (int i = 0; i < m_salarydrafts.size(); i++)
    {
        if (m_salarydrafts.at(i)->m_employee_id == employee_id)
        {
            return true;
        }
    }
    // An invoice names the employee it was written for as its consultant, and
    // a draft is waiting to be written for one.
    for (int i = 0; i < m_invoicedrafts.size(); i++)
    {
        if (m_invoicedrafts.at(i)->m_employee_id == employee_id)
        {
            return true;
        }
    }
    for (int i = 0; i < m_invoices.size(); i++)
    {
        if (m_invoices.at(i)->m_employee_id == employee_id)
        {
            return true;
        }
    }
    return false;
}

//-----------------------------------------------------------------------------
QList<int> Business::salaryYears(int employee_id) const
{
    QList<int> years;
    auto add = [&](const QDate& date, int id) {
        if (!date.isValid() || (employee_id != 0 && id != employee_id))
        {
            return;
        }
        if (!years.contains(date.year()))
        {
            years.append(date.year());
        }
    };

    for (int i = 0; i < m_salarydrafts.size(); i++)
    {
        add(m_salarydrafts.at(i)->m_payment_date, m_salarydrafts.at(i)->m_employee_id);
    }
    for (int i = 0; i < m_salaryslips.size(); i++)
    {
        add(m_salaryslips.at(i)->m_payment_date, m_salaryslips.at(i)->m_employee_id);
    }

    std::sort(years.begin(), years.end());
    return years;
}

//-----------------------------------------------------------------------------
QString Business::toString(void)
{
    QString s;
    s.append("version: " + QString::number(m_version) + '\n');
    // s.append("==============================================================\n");
    // s.append(m_companyinfo.toString());
    // s.append("==============================================================\n");
    // s.append(m_customers.toString());
    s.append("==============================================================\n");
    s.append(m_invoicedrafts.toString());
    return s;
}
