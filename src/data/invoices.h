#ifndef INVOICES_H
#define INVOICES_H

#include <QDate>
#include <QMap>

#include <memory>

#include "invoice.h"

// Owns the invoices it holds: they are deleted by clear(), by remove() and by
// the destructor. Pointers handed out by get()/at() are observers and stay
// valid until the invoice is removed or the collection goes away.
class Invoices
{
public:
    Invoices();
    ~Invoices();

    // Owning collection, so copying one would mean deciding who deletes what.
    Invoices(const Invoices&) = delete;
    Invoices& operator=(const Invoices&) = delete;

    int nof_invoices(int year); // year range: 00-99

    void clear(void);
    Invoice* get(int id);

    // Takes over `p`, assigns it the next free id and updates the per-year
    // counter. Returns the stored invoice.
    Invoice* append(std::unique_ptr<Invoice> p);

    void remove(Invoice *p);
    int size(void) const;
    Invoice* at(int i) const;
    QString toString(void);

    int lid(void) const { return m_id; }
    void setId(int id) { m_id = id; }

private:
    int m_id = -1;

private:
    QMap<int, int> m_numbers; // year (yy), counter
    QList<Invoice*> m_list;
};

#endif // INVOICES_H
