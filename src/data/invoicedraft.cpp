#include "invoicedraft.h"

//-----------------------------------------------------------------------------
InvoiceDraft::InvoiceDraft()
{
}

//-----------------------------------------------------------------------------
bool InvoiceDraft::update(const InvoiceDraft& p)
{
    if (id() == p.id())
    {
        m_invoice_year = p.m_invoice_year;
        m_invoice_number= p.m_invoice_number;
        m_customer_id = p.m_customer_id;
        m_agreement_id = p.m_agreement_id;
        m_employee_id = p.m_employee_id;
        m_invoice_date = p.m_invoice_date;
        m_month = p.m_month;
        m_hours_standard = p.m_hours_standard;
        m_hours_overtime = p.m_hours_overtime;
        m_hours_qualified = p.m_hours_qualified;
        m_hours_traveltime = p.m_hours_traveltime;
        m_comment = p.m_comment;
        m_is_credit = p.m_is_credit;
        m_credited_invoice_number = p.m_credited_invoice_number;
        return true;
    }
    return false;
}

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
QString InvoiceDraft::toString(void)
{
    QString s;
    s.append(QString::number(m_id) + '\n');
    s.append(QString::number(m_invoice_year) + '\n');
    s.append(QString::number(m_invoice_number) + '\n');
    // s.append(m_invoice_date.toString(DATE_FORMAT_STRING) + '\n');
    s.append(QString::number(m_customer_id) + '\n');
    s.append(QString::number(m_agreement_id) + '\n');
    s.append(QString::number(m_employee_id) + '\n');
    s.append(QString::number(m_month) + '\n');
    s.append(QString::number(m_hours_standard) + '\n');
    s.append(QString::number(m_hours_overtime) + '\n');
    s.append(QString::number(m_hours_qualified) + '\n');
    s.append(QString::number(m_hours_traveltime) + '\n');
    s.append(m_comment + '\n');
    return s + "\n";
}
