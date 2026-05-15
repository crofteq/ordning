#ifndef CUSTOMER_H
#define CUSTOMER_H

#include <QList>
#include <QString>

#include "agreements.h"

class Customer
{
public:
    Customer();

    QString m_name;
    QString m_org_number;
    QString m_adrsline1;
    QString m_adrsline2;
    QString m_email;
    QString m_email_invoice;
    Agreements m_agreements;

    void update(Customer* c);
    void updateAgreement(Agreement* p_ag);
    void deleteAgreement(Agreement* p_ag);
    int nofAgreements(void);
    QString toString(void);

    int id(void) { return m_id; }
    void setId(int id) { m_id = id; }

private:
    int m_id = -1;
};

#endif // CUSTOMER_H
