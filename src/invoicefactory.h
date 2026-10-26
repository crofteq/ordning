#ifndef INVOICEFACTORY_H
#define INVOICEFACTORY_H

#include <memory>

#include "data/invoice.h"
#include "data/invoicedraft.h"

class InvoiceFactory
{
public:
    InvoiceFactory();

    // Builds the invoice a draft describes, or returns nullptr, saying why in
    // `error` when one is given: the customer, the agreement, the consultant or
    // the credited invoice cannot be found, the agreement or the consultant is
    // no longer active, or the credited invoice has been credited already. A
    // credit invoice is written whatever has become of them since, since it
    // only cancels what was invoiced. The caller owns the result until it is
    // handed to Invoices::append().
    static std::unique_ptr<Invoice> create(InvoiceDraft* p_draft, QString* error = nullptr);
};

#endif // INVOICEFACTORY_H
