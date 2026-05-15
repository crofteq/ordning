#include <QDebug>

#include "business.h"

Business::Business() {}

//-----------------------------------------------------------------------------
void Business::clear(void)
{
    m_companyinfo.clear();
    m_customers.clear();
    m_invoicedrafts.clear();
    m_invoices.clear();
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
