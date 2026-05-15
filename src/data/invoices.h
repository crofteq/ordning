#ifndef INVOICES_H
#define INVOICES_H

#include <QDate>
#include <QMap>

#include "invoice.h"

class Invoices
{
public:
    Invoices();

    int nof_invoices(int year); // year range: 00-99

    void clear(void);
    Invoice* get(int id);
    void append(Invoice *p);
    void remove(Invoice *p);
    int size(void);
    Invoice* at(int i);
    QString toString(void);

    int lid(void) { return m_id; }
    void setId(int id) { m_id = id; }

private:
    int m_id = -1;

private:
    QMap<int, int> m_numbers; // year (yy), counter
    QList<Invoice*> m_list;
};

#endif // INVOICES_H
