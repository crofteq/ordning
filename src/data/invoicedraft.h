#ifndef INVOICEDRAFT_H
#define INVOICEDRAFT_H

#include <QDate>

class InvoiceDraft
{
public:
    InvoiceDraft();

    // Copies the fields of `p` when the ids match. Returns whether it did.
    bool update(const InvoiceDraft& p);

    int m_invoice_year;
    int m_invoice_number;
    QDate m_invoice_date;
    int m_customer_id;
    int m_agreement_id;
    // The employee invoiced as the consultant. -1 until one is picked; a draft
    // without one cannot become an invoice.
    int m_employee_id = -1;
    int m_month;
    // Hours are hundredths of an hour, so 7,5 h is 750. See util/amount.h.
    int m_hours_standard;
    int m_hours_overtime;
    int m_hours_qualified;
    int m_hours_traveltime;
    QString m_comment;
    bool m_is_credit = false;
    QString m_credited_invoice_number;
    QString toString(void);

    int id(void) const { return m_id; }
    void setId(int id) { m_id = id; }

private:
    int m_id = -1;

};

#endif // INVOICEDRAFT_H
