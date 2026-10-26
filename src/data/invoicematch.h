#ifndef INVOICEMATCH_H
#define INVOICEMATCH_H

#include "customers.h"
#include "invoices.h"

// Invoices written before the uppdrag was kept with them carry no agreement,
// and the reports cannot then tell one project from another. What they do
// carry is the contact person they were addressed to, and an uppdrag has a
// contact person of its own, so a name that belongs to exactly one of the
// customer's uppdrag says which one the invoice was for.

// A name is weaker evidence than the agreement id an invoice is written with
// today: a contact person can be replaced without the uppdrag changing, and
// then an old invoice carries a name the agreement no longer has — or, worse,
// a name that has since moved to the customer's other uppdrag. So the matching
// is a proposal to be looked at, not something that happens on its own.

// Which uppdrag of the customer the invoice was addressed to, judged by its
// contact person. Returns nullptr when no uppdrag matches, when the invoice
// names nobody, or when more than one uppdrag fits: naming the wrong project
// would be worse than naming none.
const Agreement* invoice_agreement_by_reference(const Invoice& invoice, const Customers& customers);

// An invoice and the uppdrag its contact person points at.
struct InvoiceAgreementMatch
{
    Invoice* p_invoice = nullptr;
    const Agreement* p_agreement = nullptr;
};

// What the matching would do: every invoice that has no uppdrag and whose
// contact person points at exactly one. Nothing is changed, so the proposal
// can be shown before it is accepted. In the order the invoices are held.
QList<InvoiceAgreementMatch> invoice_agreement_matches(const Invoices& invoices, const Customers& customers);

// Writes those matches. Returns how many invoices were given an uppdrag.
int invoice_link_agreements(const QList<InvoiceAgreementMatch>& matches);

#endif // INVOICEMATCH_H
