#include <QDebug>

#include "customer.h"

//-----------------------------------------------------------------------------
Customer::Customer()
{
    m_id = -1;
    m_name = "";
    m_org_number = "";
    m_adrsline1 = "";
    m_adrsline2 = "";
    m_email = "";
    m_email_invoice = "";
}

//-----------------------------------------------------------------------------
void Customer::update(Customer* c)
{
    m_name = c->m_name;
    m_org_number = c->m_org_number;
    m_adrsline1 = c->m_adrsline1;
    m_adrsline2 = c->m_adrsline2;
    m_email = c->m_email;
    m_email_invoice = c->m_email_invoice;
}

//-----------------------------------------------------------------------------
void Customer::updateAgreement(Agreement *a)
{
    m_agreements.update(a);
}

//-----------------------------------------------------------------------------
void Customer::deleteAgreement(Agreement *a)
{
    m_agreements.remove(a);
}

//-----------------------------------------------------------------------------
int Customer::nofAgreements(void)
{
    return m_agreements.size();
}

//-----------------------------------------------------------------------------
QString Customer::toString(void)
{
    QString s;
    s.append(QString::number(m_id) + '\n');
    s.append(m_name + '\n');
    s.append(m_org_number + '\n');
    s.append(m_adrsline1 + '\n');
    s.append(m_adrsline2 + '\n');
    s.append(m_email + '\n');
    s.append(m_email_invoice + '\n');
    s.append(m_agreements.toString() + '\n');
    return s;
}
