#ifndef INVOICEREPORT_H
#define INVOICEREPORT_H

#include <QList>
#include <QString>

#include "customers.h"
#include "invoices.h"

// One line of an invoice report: a customer, an uppdrag of a customer, or the
// total of the report. The lines come in order of customer, and within a
// customer in order of uppdrag.
struct InvoiceReportRow
{
    QString customer_name;
    QString agreement;          // the uppdrag, on the rows that are per uppdrag
    int invoices = 0;           // credit invoices counted too
    long long sum = 0;          // öre, excluding VAT
    long long vat = 0;          // öre
    long long sum_including_vat = 0;
    bool total = false;

    bool operator==(const InvoiceReportRow& o) const
    {
        return customer_name == o.customer_name && agreement == o.agreement
            && invoices == o.invoices && sum == o.sum
            && vat == o.vat && sum_including_vat == o.sum_including_vat && total == o.total;
    }
};

// What was invoiced in `year` and `month` (1-12), one line per customer and a
// total last. It goes by the invoice date. A credit invoice carries negative
// amounts, so it subtracts itself from the customer it belongs to.
QList<InvoiceReportRow> invoice_report_month(const Invoices& invoices, int year, int month);

// The same for a whole fiscal year, the one that began in `year`. With
// `start_month` 1 that is the calendar year; see data/fiscalyear.h.
QList<InvoiceReportRow> invoice_report_year(const Invoices& invoices, int year, int start_month);

// The same again, but a line per uppdrag rather than per customer: a customer
// with two agreements gets a line for each, named after the agreement.
//
// An invoice keeps the name of the agreement it was written under, but the
// ones written before it did keep only its avtalsid; for those the name is
// looked up among `customers`, and an agreement that is no longer there
// leaves the avtalsid to stand for it. An invoice written without an
// agreement is gathered under a line of its own.
QList<InvoiceReportRow> invoice_report_agreement_month(const Invoices& invoices, const Customers& customers, int year, int month);
QList<InvoiceReportRow> invoice_report_agreement_year(const Invoices& invoices, const Customers& customers, int year, int start_month);

// The fiscal years there are invoices for, oldest first.
QList<int> invoice_report_years(const Invoices& invoices, int start_month);

#endif // INVOICEREPORT_H
