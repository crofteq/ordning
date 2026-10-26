#ifndef AGREEMENT_H
#define AGREEMENT_H

#include <QString>

class Agreement
{
public:
    Agreement();

    // A new agreement is active, as the Agreements tab starts one.
    bool m_active = true;
    QString m_agreement_name;
    QString m_agreement_id;
    QString m_reference_name;
    QString m_reference_email;
    QString m_reference_phone;
    QString m_reference_name_ours;
    // Hourly rates are in öre, so 1000 kr/h is 100000. See util/amount.h.
    long long m_standard_hourly_rate = 0;
    long long m_overtime_hourly_rate = 0;
    long long m_qualified_hourly_rate = 0;
    long long m_traveltime_hourly_rate = 0;
    int m_payment_terms_days = 0; // 30 dagar
    int m_late_payment_interest = 0; // 8 %

    void update(const Agreement& a);
    QString toString(void);

    int id(void) const { return m_id; }
    void setId(int id) { m_id = id; }

private:
    int m_id = -1;
};

#endif // AGREEMENT_H
