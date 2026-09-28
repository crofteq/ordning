#include <QCollator>
#include <QDate>
#include <QMap>

#include "fiscalyear.h"

#include "invoicereport.h"

//-----------------------------------------------------------------------------
// An invoice keeps its date as the text it is printed with.
static QDate dateOf(const Invoice* p_invoice)
{
    return QDate::fromString(p_invoice->m_date, Qt::ISODate);
}

//-----------------------------------------------------------------------------
// The invoices of `year`, and of `month` as well when that is 1-12, summed per
// customer with a total last. `start_month` decides what a year means: with 1
// the calendar year, otherwise the fiscal year that began in `year`. Customers
// come in the order they were first invoiced.
// What to call the uppdrag an invoice was written under, and what tells it
// apart from the customer's other uppdrag.
struct InvoiceAgreement
{
    QString key;    // what makes it a line of its own
    QString name;   // what the line is called
};

static InvoiceAgreement agreementOf(const Invoice* p_invoice, const Customers& customers)
{
    // What the line is called: the name the invoice was written under.
    QString name = p_invoice->m_agreement_name;

    // What makes it a line of its own: the agreement it was written under.
    // Two agreements may carry the same avtalsid and still be different
    // projects, so the id they are stored under is what tells them apart.
    if (p_invoice->m_agreement_key > 0)
    {
        return { QString::number(p_invoice->m_agreement_key), name.isEmpty() ? p_invoice->m_agreement_id : name };
    }

    // An invoice from before that was kept with it knows only the avtalsid.
    // The customer's agreements can say which one it was, as long as only one
    // of them carries that avtalsid: with several there is no telling, and
    // naming the wrong project would be worse than naming none.
    const Agreement* p_found = nullptr;
    int matches = 0;
    for (int i = 0; i < customers.size(); i++)
    {
        const Customer* p_customer = customers.at(i);
        if (p_customer->id() != p_invoice->m_customer_id)
        {
            continue;
        }
        for (int j = 0; j < p_customer->m_agreements.size(); j++)
        {
            const Agreement* p_agreement = p_customer->m_agreements.at(j);
            if (p_agreement->m_agreement_id == p_invoice->m_agreement_id && !p_agreement->m_agreement_name.isEmpty())
            {
                p_found = p_agreement;
                matches++;
            }
        }
    }
    if (matches == 1)
    {
        return { QString::number(p_found->id()), name.isEmpty() ? p_found->m_agreement_name : name };
    }

    // Nothing to go on but what the invoice carries.
    if (name.isEmpty())
    {
        name = p_invoice->m_agreement_id.isEmpty() ? QString("(utan avtal)") : p_invoice->m_agreement_id;
    }
    return { name, name };
}

//-----------------------------------------------------------------------------
static QList<InvoiceReportRow> report(const Invoices& invoices, const Customers& customers,
                                      int year, int month, int start_month, bool by_agreement)
{
    QList<QString> order;
    QMap<QString, InvoiceReportRow> rows;

    for (int i = 0; i < invoices.size(); i++)
    {
        const Invoice* p = invoices.at(i);
        const QDate date = dateOf(p);
        if (!date.isValid() || fiscal_year_of(date, start_month) != year)
        {
            continue;
        }
        if (month >= 1 && month <= 12 && date.month() != month)
        {
            continue;
        }

        // A customer is the same customer whatever they are called; an
        // uppdrag is the agreement id printed on the invoice.
        const InvoiceAgreement agreement = by_agreement ? agreementOf(p, customers) : InvoiceAgreement();
        const QString key = by_agreement
            ? QString("%1\n%2").arg(p->m_customer_id).arg(agreement.key)
            : QString::number(p->m_customer_id);
        if (!rows.contains(key))
        {
            order.append(key);
            rows.insert(key, InvoiceReportRow());
        }

        InvoiceReportRow& row = rows[key];
        // The name is the one on the latest invoice, since that is what the
        // customer was called when it was sent.
        row.customer_name = p->m_customer_name;
        if (by_agreement)
        {
            row.agreement = agreement.name;
        }
        row.invoices++;
        row.sum += p->m_sum;
        row.vat += p->m_vat;
        row.sum_including_vat += p->m_sum_including_vat;
    }

    // In order of customer, and within a customer in order of uppdrag, so
    // that a customer's lines stand together however the invoices came.
    QCollator collator;
    collator.setCaseSensitivity(Qt::CaseInsensitive);
    std::sort(order.begin(), order.end(), [&](const QString& left, const QString& right) {
        const InvoiceReportRow& a = rows.value(left);
        const InvoiceReportRow& b = rows.value(right);
        if (a.customer_name != b.customer_name)
        {
            return collator.compare(a.customer_name, b.customer_name) < 0;
        }
        return collator.compare(a.agreement, b.agreement) < 0;
    });

    QList<InvoiceReportRow> result;
    InvoiceReportRow sum;
    sum.customer_name = "Totalt";
    sum.total = true;
    for (const QString& key : order)
    {
        const InvoiceReportRow& row = rows.value(key);
        result.append(row);
        sum.invoices += row.invoices;
        sum.sum += row.sum;
        sum.vat += row.vat;
        sum.sum_including_vat += row.sum_including_vat;
    }
    if (!result.isEmpty())
    {
        result.append(sum);
    }
    return result;
}

//-----------------------------------------------------------------------------
QList<InvoiceReportRow> invoice_report_month(const Invoices& invoices, int year, int month)
{
    // A month is a month whatever the fiscal year looks like.
    return report(invoices, Customers(), year, month, 1, false);
}

//-----------------------------------------------------------------------------
QList<InvoiceReportRow> invoice_report_year(const Invoices& invoices, int year, int start_month)
{
    return report(invoices, Customers(), year, 0, start_month, false);
}

//-----------------------------------------------------------------------------
QList<InvoiceReportRow> invoice_report_agreement_month(const Invoices& invoices, const Customers& customers, int year, int month)
{
    return report(invoices, customers, year, month, 1, true);
}

//-----------------------------------------------------------------------------
QList<InvoiceReportRow> invoice_report_agreement_year(const Invoices& invoices, const Customers& customers, int year, int start_month)
{
    return report(invoices, customers, year, 0, start_month, true);
}

//-----------------------------------------------------------------------------
static QList<ConsultantHoursRow> hoursReport(const Invoices& invoices, int year, int month, int start_month)
{
    QList<QString> order;
    QMap<QString, ConsultantHoursRow> rows;

    for (int i = 0; i < invoices.size(); i++)
    {
        const Invoice* p = invoices.at(i);
        const QDate date = dateOf(p);
        if (!date.isValid() || fiscal_year_of(date, start_month) != year)
        {
            continue;
        }
        if (month >= 1 && month <= 12 && date.month() != month)
        {
            continue;
        }

        // A consultant is the same employee whatever they are called; an
        // invoice from before one was picked has only the name to go by.
        const QString name = p->m_consultant_name.trimmed().isEmpty()
            ? QString("(utan konsult)") : p->m_consultant_name.trimmed();
        const QString key = (p->m_employee_id >= 0)
            ? QString::number(p->m_employee_id)
            : QString("\n%1").arg(name);
        if (!rows.contains(key))
        {
            order.append(key);
            rows.insert(key, ConsultantHoursRow());
        }

        ConsultantHoursRow& row = rows[key];
        // The name is the one on the latest invoice.
        row.consultant_name = name;
        row.invoices++;
        const int sign = p->m_is_credit ? -1 : 1;
        row.standard += sign * p->m_hours_standard;
        row.overtime += sign * p->m_hours_overtime;
        row.qualified += sign * p->m_hours_qualified;
        row.traveltime += sign * p->m_hours_traveltime;
        row.hours = row.standard + row.overtime + row.qualified + row.traveltime;
    }

    QCollator collator;
    collator.setCaseSensitivity(Qt::CaseInsensitive);
    std::sort(order.begin(), order.end(), [&](const QString& left, const QString& right) {
        return collator.compare(rows.value(left).consultant_name, rows.value(right).consultant_name) < 0;
    });

    QList<ConsultantHoursRow> result;
    ConsultantHoursRow sum;
    sum.consultant_name = "Totalt";
    sum.total = true;
    for (const QString& key : order)
    {
        const ConsultantHoursRow& row = rows.value(key);
        result.append(row);
        sum.invoices += row.invoices;
        sum.standard += row.standard;
        sum.overtime += row.overtime;
        sum.qualified += row.qualified;
        sum.traveltime += row.traveltime;
        sum.hours += row.hours;
    }
    if (!result.isEmpty())
    {
        result.append(sum);
    }
    return result;
}

//-----------------------------------------------------------------------------
QList<ConsultantHoursRow> consultant_hours_report_year(const Invoices& invoices, int year, int start_month)
{
    return hoursReport(invoices, year, 0, start_month);
}

//-----------------------------------------------------------------------------
QList<ConsultantHoursRow> consultant_hours_report_month(const Invoices& invoices, int year, int month)
{
    return hoursReport(invoices, year, month, 1);
}

//-----------------------------------------------------------------------------
QList<int> invoice_report_years(const Invoices& invoices, int start_month)
{
    QList<int> years;
    for (int i = 0; i < invoices.size(); i++)
    {
        const QDate date = dateOf(invoices.at(i));
        const int year = fiscal_year_of(date, start_month);
        if (date.isValid() && !years.contains(year))
        {
            years.append(year);
        }
    }
    std::sort(years.begin(), years.end());
    return years;
}
