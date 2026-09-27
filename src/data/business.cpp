#include <QDebug>

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
