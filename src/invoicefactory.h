#ifndef INVOICEFACTORY_H
#define INVOICEFACTORY_H

#include "data/invoice.h"
#include "data/invoicedraft.h"

class InvoiceFactory
{
public:
    InvoiceFactory();

    static Invoice* create(InvoiceDraft* p_draft);
};

#endif // INVOICEFACTORY_H
