#ifndef INVOICEDRAFT_H
#define INVOICEDRAFT_H

#include <QDate>
#include <QList>
#include <QString>
#include <QStringList>

// A cost the company paid for in the course of an uppdrag and passes on to the
// customer: a train ticket, a hotel night, a licence. It is bought in the
// company's own name and its ingående moms is the company's to deduct, so it is
// invoiced as vidarefakturering: the amount without that moms, and moms on top
// at the rate of the consultancy it is part of, whatever rate the receipt
// carried. An utlägg proper, bought in the customer's name and passed on
// without moms, is not supported.
static const int INVOICE_EXPENSE_VAT = 25;

struct InvoiceExpense
{
    QString text;                       // "Tågresa Stockholm–Göteborg 3/10"
    long long amount = 0;               // öre, excluding moms
    int vat = INVOICE_EXPENSE_VAT;      // percent, kept with an invoice as it was printed

    bool operator==(const InvoiceExpense& o) const
    {
        return text == o.text && amount == o.amount && vat == o.vat;
    }
};

// The most costs one invoice carries. Seven fit beside all four kinds of hours
// and a comment of one line, so six leave the comment a line more. The page
// decides what really fits, which InvoicePrinter::fits() answers for: a longer
// comment or a benämning that wraps leaves room for fewer.
static const int INVOICE_MAX_EXPENSES = 6;

// Reads the cost table of the edit page: per row the benämning and the amount
// excluding moms, as typed, with a decimal comma. No rows is fine, since most
// invoices carry no costs; a row needs a benämning and an amount that is one
// and not nothing. On the first problem, returns false and says why in `error`.
bool invoice_parse_expenses(const QList<QStringList>& rows, QList<InvoiceExpense>* expenses, QString* error);

class InvoiceDraft
{
public:
    InvoiceDraft();

    // Copies the fields of `p` when the ids match. Returns whether it did.
    bool update(const InvoiceDraft& p);

    int m_invoice_year = 0;
    int m_invoice_number = 0;
    QDate m_invoice_date;
    int m_customer_id = -1;
    int m_agreement_id = -1;
    // The employee invoiced as the consultant. -1 until one is picked; a draft
    // without one cannot become an invoice.
    int m_employee_id = -1;
    int m_month = 0;
    // Hours are hundredths of an hour, so 7,5 h is 750. See util/amount.h.
    int m_hours_standard = 0;
    int m_hours_overtime = 0;
    int m_hours_qualified = 0;
    int m_hours_traveltime = 0;
    QString m_comment;
    bool m_is_credit = false;
    QString m_credited_invoice_number;
    QList<InvoiceExpense> m_expenses;
    QString toString(void);

    int id(void) const { return m_id; }
    void setId(int id) { m_id = id; }

private:
    int m_id = -1;

};

#endif // INVOICEDRAFT_H
