#ifndef INVOICEFACTORY_H
#define INVOICEFACTORY_H

#include <memory>

#include "data/invoice.h"
#include "data/invoicedraft.h"

class InvoiceFactory
{
public:
    InvoiceFactory();

    // Builds the invoice a draft describes, or returns nullptr when the
    // customer, the agreement or the credited invoice cannot be found. The
    // caller owns the result until it is handed to Invoices::append().
    static std::unique_ptr<Invoice> create(InvoiceDraft* p_draft);
};

#endif // INVOICEFACTORY_H
