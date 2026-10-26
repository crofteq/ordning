#include "util/amount.h"

#include "invoicedraft.h"

//-----------------------------------------------------------------------------
InvoiceDraft::InvoiceDraft()
{
}

//-----------------------------------------------------------------------------
bool InvoiceDraft::update(const InvoiceDraft& p)
{
    if (id() == p.id())
    {
        m_invoice_year = p.m_invoice_year;
        m_invoice_number= p.m_invoice_number;
        m_customer_id = p.m_customer_id;
        m_agreement_id = p.m_agreement_id;
        m_employee_id = p.m_employee_id;
        m_invoice_date = p.m_invoice_date;
        m_month = p.m_month;
        m_hours_standard = p.m_hours_standard;
        m_hours_overtime = p.m_hours_overtime;
        m_hours_qualified = p.m_hours_qualified;
        m_hours_traveltime = p.m_hours_traveltime;
        m_comment = p.m_comment;
        m_is_credit = p.m_is_credit;
        m_credited_invoice_number = p.m_credited_invoice_number;
        m_expenses = p.m_expenses;
        return true;
    }
    return false;
}

//-----------------------------------------------------------------------------
bool invoice_parse_expenses(const QList<QStringList>& rows, QList<InvoiceExpense>* expenses, QString* error)
{
    expenses->clear();
    if (rows.size() > INVOICE_MAX_EXPENSES)
    {
        *error = QString("En faktura rymmer högst %1 kostnader.").arg(INVOICE_MAX_EXPENSES);
        return false;
    }

    for (int i = 0; i < rows.size(); i++)
    {
        const QStringList& row = rows.at(i);
        auto cell = [&](int column) -> QString {
            return column < row.size() ? row.at(column).trimmed() : QString();
        };

        InvoiceExpense expense;
        expense.text = cell(0);
        if (expense.text.isEmpty())
        {
            *error = QString("Kostnad %1 behöver en benämning: den är vad kunden ser på fakturan.").arg(i + 1);
            return false;
        }

        bool ok = false;
        expense.amount = amount_parse_ore(cell(1), &ok);
        if (!ok)
        {
            *error = QString("Beloppet för kostnad %1 är inte ett belopp.").arg(i + 1);
            return false;
        }
        // A cost of nothing is a row left half filled in, and one of less than
        // nothing is a credit, which is what a kreditfaktura is for.
        if (expense.amount <= 0)
        {
            *error = QString("Beloppet för kostnad %1 måste vara större än noll.").arg(i + 1);
            return false;
        }
        expenses->append(expense);
    }
    return true;
}

//-----------------------------------------------------------------------------
QString InvoiceDraft::toString(void)
{
    QString s;
    s.append(QString::number(m_id) + '\n');
    s.append(QString::number(m_invoice_year) + '\n');
    s.append(QString::number(m_invoice_number) + '\n');
    // s.append(m_invoice_date.toString(DATE_FORMAT_STRING) + '\n');
    s.append(QString::number(m_customer_id) + '\n');
    s.append(QString::number(m_agreement_id) + '\n');
    s.append(QString::number(m_employee_id) + '\n');
    s.append(QString::number(m_month) + '\n');
    s.append(QString::number(m_hours_standard) + '\n');
    s.append(QString::number(m_hours_overtime) + '\n');
    s.append(QString::number(m_hours_qualified) + '\n');
    s.append(QString::number(m_hours_traveltime) + '\n');
    s.append(m_comment + '\n');
    return s + "\n";
}
