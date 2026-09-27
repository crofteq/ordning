#include "util/amount.h"

#include "salarydraft.h"

//-----------------------------------------------------------------------------
long long SalaryLine::total_ore(void) const
{
    return amount_line_total_ore(amount, quantity);
}

//-----------------------------------------------------------------------------
SalaryDraft::SalaryDraft()
{
}

//-----------------------------------------------------------------------------
void SalaryDraft::update(const SalaryDraft& d)
{
    m_employee_id = d.m_employee_id;
    m_payment_date = d.m_payment_date;
    m_lines = d.m_lines;
}

//-----------------------------------------------------------------------------
long long SalaryDraft::gross_ore(void) const
{
    long long sum = 0;
    for (const SalaryLine& line : m_lines)
    {
        if (line.kind == SALARY_LINE_TAXABLE)
        {
            sum += line.total_ore();
        }
    }
    return sum;
}

//-----------------------------------------------------------------------------
long long SalaryDraft::taxfree_ore(void) const
{
    long long sum = 0;
    for (const SalaryLine& line : m_lines)
    {
        if (line.kind == SALARY_LINE_TAX_FREE)
        {
            sum += line.total_ore();
        }
    }
    return sum;
}

//-----------------------------------------------------------------------------
bool salary_parse_lines(const QList<QStringList>& rows, QList<SalaryLine>* lines, QString* error)
{
    lines->clear();
    if (rows.isEmpty())
    {
        *error = "The salary draft needs at least one line.";
        return false;
    }
    if (rows.size() > SALARY_MAX_LINES)
    {
        *error = QString("A salary statement holds at most %1 lines.").arg(SALARY_MAX_LINES);
        return false;
    }

    for (int i = 0; i < rows.size(); i++)
    {
        const QStringList& row = rows.at(i);
        auto cell = [&](int column) -> QString {
            return column < row.size() ? row.at(column).trimmed() : QString();
        };

        SalaryLine line;
        bool ok_quantity, ok_amount;
        // Antal is kept in hundredths, the same scale as öre, so it parses
        // the same way.
        line.quantity = (int)amount_parse_ore(cell(1), &ok_quantity);
        line.amount = amount_parse_ore(cell(2), &ok_amount);
        // Free text, and none of it required: the length it may run to is
        // decided by the page, which SalarySlipPrinter::fits() answers for.
        line.comment = cell(3);
        // Not typed: it is the value the line was looked up by, and the page
        // hands back what it was given.
        line.applies_to = cell(4);

        // The benämning and the kind are the löneart's, so a line that has
        // none of the fixed lönearter is a line that cannot be worked out:
        // what it is called, whether it is taxed and whether it is paid out
        // all hang on it.
        const SalaryType* p_type = salary_type(cell(0).toInt());
        if (p_type == nullptr)
        {
            *error = QString("Line %1 needs a löneart.").arg(i + 1);
            return false;
        }
        line.code = p_type->number;
        line.text = p_type->name;
        line.kind = p_type->kind;

        if (!ok_quantity)
        {
            *error = QString("Antal on line %1 is not a number.").arg(i + 1);
            return false;
        }
        if (!ok_amount)
        {
            *error = QString("Kronor on line %1 is not an amount.").arg(i + 1);
            return false;
        }
        lines->append(line);
    }
    return true;
}
