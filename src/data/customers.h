#ifndef CUSTOMERS_H
#define CUSTOMERS_H

#include <QList>

#include "customer.h"

// Owns the customers it holds: they are deleted by clear(), by remove() and by
// the destructor. Pointers handed out by get()/at() are observers and stay
// valid until the customer is removed or the collection goes away.
class Customers
{
public:
    Customers();
    ~Customers();

    // Owning collection, so copying one would mean deciding who deletes what.
    Customers(const Customers&) = delete;
    Customers& operator=(const Customers&) = delete;

    void clear(void);

    Customer* get(int id);

    // Copies `c` onto the customer with the same id, or stores a copy of it
    // under the next free id. `c` itself is never kept, so the caller stays the
    // owner of whatever it passed in, and the agreements of an existing
    // customer are left alone. Returns the stored customer.
    Customer* update(const Customer& c);

    void remove(Customer* c);
    int size(void) const;
    Customer* at(int i) const;
    QString toString(void);

    int lid(void) const { return m_id; }
    void setId(int id) { m_id = id; }

private:
    int m_id = -1;
    QList<Customer*> m_list;
};

#endif // CUSTOMERS_H
