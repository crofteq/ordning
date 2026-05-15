#ifndef AGREEMENT_H
#define AGREEMENT_H

#include <QString>

class Agreement
{
public:
    Agreement();

    bool m_active;
    QString m_agreement_name;
    QString m_agreement_id;
    QString m_reference_name;
    QString m_reference_email;
    QString m_reference_phone;
    QString m_reference_name_ours;
    int m_standard_hourly_rate;
    int m_overtime_hourly_rate;
    int m_qualified_hourly_rate;
    int m_traveltime_hourly_rate;
    int m_payment_terms_days; // 30 dagar
    int m_late_payment_interest; // 8 %

    void update(Agreement* p_a);
    QString toString(void);

    int id(void) { return m_id; }
    void setId(int id) { m_id = id; }

private:
    int m_id = -1;
};

#endif // AGREEMENT_H
