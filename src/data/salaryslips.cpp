#include "salaryslips.h"

SalarySlips::SalarySlips() {}

//-----------------------------------------------------------------------------
SalarySlips::~SalarySlips()
{
    clear();
}

//-----------------------------------------------------------------------------
void SalarySlips::clear(void)
{
    qDeleteAll(m_list);
    m_list.clear();
}

//-----------------------------------------------------------------------------
SalarySlip* SalarySlips::get(int id)
{
    for (int i=0; i<m_list.size(); i++)
    {
        if (m_list.at(i)->id() == id)
        {
            return m_list.at(i);
        }
    }
    return nullptr;
}

//-----------------------------------------------------------------------------
SalarySlip* SalarySlips::append(std::unique_ptr<SalarySlip> owned)
{
    SalarySlip* p = owned.release();

    int max = 0;
    for (int i=0; i<m_list.size(); i++)
    {
        if (max < m_list.at(i)->id())
        {
            max = m_list.at(i)->id();
        }
    }
    p->setId(max + 1);
    m_list.append(p);
    return p;
}

//-----------------------------------------------------------------------------
int SalarySlips::size(void) const
{
    return m_list.size();
}

//-----------------------------------------------------------------------------
SalarySlip* SalarySlips::at(int i) const
{
    return m_list.at(i);
}

//-----------------------------------------------------------------------------
SalaryYearToDate SalarySlips::yearToDate(const SalarySlip& slip) const
{
    SalaryYearToDate ytd;
    for (const SalarySlip* s : m_list)
    {
        if (s->m_employee_id != slip.m_employee_id
            || s->m_payment_date.year() != slip.m_payment_date.year())
        {
            continue;
        }
        // A slip that is not stored yet (a preview) would be approved after
        // every stored one, so those paid the same day count as earlier.
        const bool same_day_before = slip.id() <= 0 || s->id() <= slip.id();
        const bool earlier = s->m_payment_date < slip.m_payment_date
            || (s->m_payment_date == slip.m_payment_date && same_day_before);
        if (earlier)
        {
            ytd.gross += s->m_gross;
            ytd.tax += s->m_tax;
            ytd.tax_free += s->m_tax_free;
        }
    }

    // A stored slip has counted itself above; one that is not stored has not.
    if (slip.id() <= 0)
    {
        ytd.gross += slip.m_gross;
        ytd.tax += slip.m_tax;
        ytd.tax_free += slip.m_tax_free;
    }
    return ytd;
}

//-----------------------------------------------------------------------------
bool SalarySlips::hasEmployee(int employee_id) const
{
    for (const SalarySlip* s : m_list)
    {
        if (s->m_employee_id == employee_id)
        {
            return true;
        }
    }
    return false;
}
