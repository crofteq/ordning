#ifndef INVOICEDRAFTS_H
#define INVOICEDRAFTS_H

#include "invoicedraft.h"

class InvoiceDrafts
{
public:
    InvoiceDrafts();

    void clear(void);
    void update(InvoiceDraft *p);
    void remove(InvoiceDraft *p);

    int size() {return m_list.size();}
    InvoiceDraft* at(int i) {return m_list.at(i); }
    QString toString(void);

    int id(void) { return m_id; }
    void setId(int id) { m_id = id; }

private:
    int m_id = -1;

private:
    int m_invoiceCounter;
    int m_invoiceYear;

    QList<InvoiceDraft*> m_list;
};

#endif // INVOICEDRAFTS_H
