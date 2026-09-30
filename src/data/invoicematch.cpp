#include <QList>

#include "invoicematch.h"

//-----------------------------------------------------------------------------
// A name as it can be compared: no case, no double spaces, nothing around it.
static QString normalized(const QString& name)
{
    return name.simplified().toCaseFolded();
}

//-----------------------------------------------------------------------------
// How many single-character edits it takes to turn one name into the other,
// counted no further than `limit`: anything above that is the same answer, so
// the work is not worth doing.
static int distance(const QString& a, const QString& b, int limit)
{
    if (qAbs(a.length() - b.length()) > limit)
    {
        return limit + 1;
    }

    QList<int> previous;
    for (int j = 0; j <= b.length(); j++)
    {
        previous.append(j);
    }

    for (int i = 1; i <= a.length(); i++)
    {
        QList<int> current;
        current.append(i);
        int best = i;
        for (int j = 1; j <= b.length(); j++)
        {
            const int cost = (a.at(i - 1) == b.at(j - 1)) ? 0 : 1;
            const int value = qMin(qMin(current.at(j - 1) + 1, previous.at(j) + 1), previous.at(j - 1) + cost);
            current.append(value);
            best = qMin(best, value);
        }
        if (best > limit)
        {
            return limit + 1;
        }
        previous = current;
    }

    return previous.last();
}

//-----------------------------------------------------------------------------
// The same person, written by two different hands. A single typo is still the
// same name, but only in a name long enough for one character not to be what
// tells two people apart.
static const int NAME_TYPO_TOLERANCE = 1;
static const int NAME_TOLERANT_FROM = 5;

static int nameDistance(const QString& a, const QString& b)
{
    if (a.isEmpty() || b.isEmpty())
    {
        return NAME_TYPO_TOLERANCE + 1;
    }

    // A short name is taken as it is written, and anything beyond what that
    // name allows is no match at all rather than a near one.
    const int limit = (qMin(a.length(), b.length()) >= NAME_TOLERANT_FROM) ? NAME_TYPO_TOLERANCE : 0;
    const int d = distance(a, b, limit);
    return (d > limit) ? NAME_TYPO_TOLERANCE + 1 : d;
}

//-----------------------------------------------------------------------------
const Agreement* invoice_agreement_by_reference(const Invoice& invoice, const Customers& customers)
{
    const QString reference = normalized(invoice.m_customer_reference_name);
    if (reference.isEmpty())
    {
        return nullptr;
    }

    // Only among the uppdrag of the customer the invoice was sent to: the same
    // contact person may well work for two companies.
    const Customer* p_customer = nullptr;
    for (int i = 0; i < customers.size() && p_customer == nullptr; i++)
    {
        if (customers.at(i)->id() == invoice.m_customer_id)
        {
            p_customer = customers.at(i);
        }
    }
    if (p_customer == nullptr)
    {
        return nullptr;
    }

    // The closest name wins, but only if it is close enough and only if it is
    // alone: two uppdrag equally close leave no way of telling which it was.
    const Agreement* p_found = nullptr;
    int best = NAME_TYPO_TOLERANCE + 1;
    int matches = 0;
    for (int i = 0; i < p_customer->m_agreements.size(); i++)
    {
        const Agreement* p_agreement = p_customer->m_agreements.at(i);
        const int d = nameDistance(reference, normalized(p_agreement->m_reference_name));
        if (d > NAME_TYPO_TOLERANCE)
        {
            continue;
        }
        if (d < best)
        {
            best = d;
            matches = 1;
            p_found = p_agreement;
        }
        else if (d == best)
        {
            matches++;
        }
    }

    return (matches == 1) ? p_found : nullptr;
}

//-----------------------------------------------------------------------------
QList<InvoiceAgreementMatch> invoice_agreement_matches(const Invoices& invoices, const Customers& customers)
{
    QList<InvoiceAgreementMatch> matches;
    for (int i = 0; i < invoices.size(); i++)
    {
        Invoice* p_invoice = invoices.at(i);
        if (p_invoice->m_agreement_key > 0)
        {
            continue;
        }

        const Agreement* p_agreement = invoice_agreement_by_reference(*p_invoice, customers);
        if (p_agreement != nullptr)
        {
            matches.append({ p_invoice, p_agreement });
        }
    }

    return matches;
}

//-----------------------------------------------------------------------------
int invoice_link_agreements(const QList<InvoiceAgreementMatch>& matches)
{
    int linked = 0;
    for (const InvoiceAgreementMatch& match : matches)
    {
        if (match.p_invoice != nullptr && match.p_agreement != nullptr)
        {
            match.p_invoice->setAgreement(*match.p_agreement);
            linked++;
        }
    }

    return linked;
}
