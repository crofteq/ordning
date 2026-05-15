#include <QDebug>

#include "agreement.h"

#define XML_KEY_AGREEMENT_NAME          "agreement_name"
#define XML_KEY_AGREEMENT_ID            "agreement_id"
#define XML_KEY_REFERENCE_NAME          "reference_name"
#define XML_KEY_REFERENCE_EMAIL         "reference_email"
#define XML_KEY_REFERENCE_PHONE         "reference_phone"
#define XML_KEY_REFERENCE_NAME_OURS     "reference_name_ours"
#define XML_KEY_HOURLY_RATE_STANDARD    "hourly_rate_standard"
#define XML_KEY_HOURLY_RATE_OVERTIME    "hourly_rate_overtime"
#define XML_KEY_HOURLY_RATE_QUALIFIED   "hourly_rate_qualified"
#define XML_KEY_HOURLY_RATE_TRAVELTIME  "hourly_rate_traveltime"
#define XML_KEY_PAYMENT_TERMS_DAYS      "payment_terms_days"
#define XML_KEY_LATE_PAYMENT_INTEREST   "late_payment_interest"
#define XML_KEY_ACTIVE                  "active"

//-----------------------------------------------------------------------------
Agreement::Agreement() {}

//-----------------------------------------------------------------------------
void Agreement::update(Agreement* p_a)
{
    m_active = p_a->m_active;
    m_agreement_name = p_a->m_agreement_name;
    m_agreement_id = p_a->m_agreement_id;
    m_reference_name = p_a->m_reference_name;
    m_reference_email = p_a->m_reference_email;
    m_reference_phone = p_a->m_reference_phone;
    m_reference_name_ours = p_a->m_reference_name_ours;
    m_standard_hourly_rate = p_a->m_standard_hourly_rate;
    m_overtime_hourly_rate = p_a->m_overtime_hourly_rate;
    m_qualified_hourly_rate = p_a->m_qualified_hourly_rate;
    m_traveltime_hourly_rate = p_a->m_traveltime_hourly_rate;
    m_payment_terms_days = p_a->m_payment_terms_days;
    m_late_payment_interest = p_a->m_late_payment_interest;
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

