#include <QDebug>

#include "companyinfo.h"

CompanyInfo::CompanyInfo() {}

//-----------------------------------------------------------------------------
void CompanyInfo::clear(void)
{
    m_name = "";
    m_adrsline1 = "";
    m_adrsline2 = "";
    m_org_number = "";
    m_vat_number = "";
    m_bankgiro = "";
    m_swift_bic = "";
    m_iban = "";
    m_phone = "";
    m_email = "";
    m_web = "";
    m_logo = "";
}

//-----------------------------------------------------------------------------
QString CompanyInfo::toString(void)
{
    QString s;
    s.append(m_name + '\n');
    s.append(m_adrsline1 + '\n');
    s.append(m_adrsline2 + '\n');
    s.append(m_org_number + '\n');
    s.append(m_vat_number + '\n');
    s.append(m_bankgiro + '\n');
    s.append(m_swift_bic + '\n');
    s.append(m_iban + '\n');
    s.append(m_phone + '\n');
    s.append(m_email + '\n');
    s.append(m_web + '\n');
    return s;
}
