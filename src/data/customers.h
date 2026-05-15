#ifndef CUSTOMERS_H
#define CUSTOMERS_H

#include <QList>

#include "customer.h"

class Customers
{
public:
    Customers();

    void clear(void);

    Customer* get(int id);
    void update(Customer* c);
    void remove(Customer* c);
    QString toString(void);

    int lid(void) { return m_id; }
    void setId(int id) { m_id = id; }

    QList<Customer*> m_list;

private:
    int m_id = -1;
};

#endif // CUSTOMERS_H
