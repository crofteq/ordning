#ifndef INVOICEDRAFT_H
#define INVOICEDRAFT_H

#include <QDate>

class InvoiceDraft
{
public:
    InvoiceDraft();

    bool update(InvoiceDraft* p);

    int m_invoice_year;
    int m_invoice_number;
    QDate m_invoice_date;
    int m_customer_id;
    int m_agreement_id;
    int m_month;
    int m_hours_standard;
    int m_hours_overtime;
    int m_hours_qualified;
    int m_hours_traveltime;
    QString m_comment;
    QString toString(void);

    int id(void) { return m_id; }
    void setId(int id) { m_id = id; }

private:
    int m_id = -1;

};

#endif // INVOICEDRAFT_H
