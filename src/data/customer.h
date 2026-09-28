#ifndef CUSTOMER_H
#define CUSTOMER_H

#include <QList>
#include <QString>

#include "agreements.h"

class Customer
{
public:
    Customer();

    // Owns its agreements, so copying one would mean deciding who deletes them.
    Customer(const Customer&) = delete;
    Customer& operator=(const Customer&) = delete;

    QString m_name;
    QString m_org_number;
    QString m_adrsline1;
    QString m_adrsline2;
    QString m_email;
    QString m_email_invoice;
    Agreements m_agreements;

    // Copies the customer fields of `c`. The id and the agreements of this
    // customer are left alone.
    void update(const Customer& c);

    // Stores a copy of `p_ag`; the caller keeps ownership of what it passed in.
    Agreement* updateAgreement(const Agreement& ag);

    // Deletes the agreement, which must be one this customer owns.
    void deleteAgreement(Agreement* p_ag);
    int nofAgreements(void);
    QString toString(void);

    int id(void) const { return m_id; }
    void setId(int id) { m_id = id; }

private:
    int m_id = -1;
};

#endif // CUSTOMER_H
