#include <QDebug>

#include "agreement.h"

//-----------------------------------------------------------------------------
Agreement::Agreement() {}

//-----------------------------------------------------------------------------
void Agreement::update(const Agreement& a)
{
    m_active = a.m_active;
    m_agreement_name = a.m_agreement_name;
    m_agreement_id = a.m_agreement_id;
    m_reference_name = a.m_reference_name;
    m_reference_email = a.m_reference_email;
    m_reference_phone = a.m_reference_phone;
    m_reference_name_ours = a.m_reference_name_ours;
    m_standard_hourly_rate = a.m_standard_hourly_rate;
    m_overtime_hourly_rate = a.m_overtime_hourly_rate;
    m_qualified_hourly_rate = a.m_qualified_hourly_rate;
    m_traveltime_hourly_rate = a.m_traveltime_hourly_rate;
    m_payment_terms_days = a.m_payment_terms_days;
    m_late_payment_interest = a.m_late_payment_interest;
}

//-----------------------------------------------------------------------------
QString Agreement::toString(void)
{
    QString s;
    s.append(QString::number(m_id) + '\n');
    s.append(QString::number(m_active) + '\n');
    s.append(m_agreement_name + '\n');
    s.append(m_agreement_id + '\n');
    s.append(m_reference_name + '\n');
    s.append(m_reference_email + '\n');
    s.append(m_reference_phone + '\n');
    s.append(m_reference_name_ours + '\n');
    s.append(QString::number(m_standard_hourly_rate) + '\n');
    s.append(QString::number(m_overtime_hourly_rate) + '\n');
    s.append(QString::number(m_qualified_hourly_rate) + '\n');
    s.append(QString::number(m_traveltime_hourly_rate) + '\n');
    s.append(QString::number(m_payment_terms_days) + '\n');
    s.append(QString::number(m_late_payment_interest) + '\n');
    return s + "\n";
}

