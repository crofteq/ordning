#include "invoice.h"

#define CONSULTANT_WORKING_HOURS_VAT    25

//-----------------------------------------------------------------------------
Invoice::Invoice()
{
    m_company_f_skatt = "Innehar F-skattsedel";
    m_agreement_standard_vat = CONSULTANT_WORKING_HOURS_VAT;
    m_agreement_overtime_vat = CONSULTANT_WORKING_HOURS_VAT;
    m_agreement_qualified_vat = CONSULTANT_WORKING_HOURS_VAT;
    m_agreement_traveltime_vat = CONSULTANT_WORKING_HOURS_VAT;
}

//-----------------------------------------------------------------------------
void Invoice::setAgreement(const Agreement& agreement)
{
    m_agreement_key = agreement.id();
    m_agreement_name = agreement.m_agreement_name;
}

//-----------------------------------------------------------------------------
QString Invoice::toString(void)
{
    QString s;
    s.append(QString::number(m_year) + '\n');
    s.append(QString::number(m_count) + '\n');
    s.append(m_number + '\n');
    // TODO: Fill up with more data...
    return s + "\n";
}
