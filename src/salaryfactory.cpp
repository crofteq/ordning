#include "data/database.h"
#include "data/employerfees.h"
#include "util/amount.h"
#include "util/personnummer.h"

#include "salaryfactory.h"

//-----------------------------------------------------------------------------
std::unique_ptr<SalarySlip> SalaryFactory::create(const SalaryDraft& draft, QString* error)
{
    Business* b = DataBase::instance()->db();

    Employee* p_em = b->m_employees.get(draft.m_employee_id);
    if (p_em == nullptr)
    {
        *error = "Den anställde på löneunderlaget finns inte kvar.";
        return nullptr;
    }

    if (!draft.m_payment_date.isValid())
    {
        *error = "Löneunderlaget har ingen utbetalningsdag.";
        return nullptr;
    }

    if (draft.m_lines.isEmpty())
    {
        *error = "Löneunderlaget har inga rader.";
        return nullptr;
    }

    if (draft.m_lines.size() > SALARY_MAX_LINES)
    {
        *error = QString("Ett lönebesked rymmer högst %1 rader.").arg(SALARY_MAX_LINES);
        return nullptr;
    }

    // A salary of nothing, or a correction that leaves less than nothing, is
    // not paid out. A slip carrying nothing but a skattefri ersättning is: a
    // trip may be the only thing paid out in a month, and there is then no
    // bruttolön to tax and no arbetsgivaravgift to pay.
    const long long gross = draft.gross_ore();
    const long long tax_free = draft.taxfree_ore();
    if (gross < 0 || (gross == 0 && tax_free <= 0))
    {
        *error = QString("Bruttolönen är %1 kr. Bara en lön över 0 kr kan godkännas.").arg(amount_format_ore(gross));
        return nullptr;
    }

    // Tax is withheld when the salary is paid, so the payment date decides
    // the year: a December salary paid in January uses January's table.
    const int year = draft.m_payment_date.year();
    int table = 0;
    long long tax = 0;
    int fee_rate = 0;
    long long fee = 0;

    if (gross > 0)
    {
        if (!p_em->m_tax_tables.contains(year))
        {
            *error = QString("%1 har ingen skattetabell för %2. Lägg till den på fliken Anställda.").arg(p_em->name()).arg(year);
            return nullptr;
        }
        table = p_em->m_tax_tables.value(year);

        if (!b->m_taxtables.hasYear(year))
        {
            *error = QString("Skattetabellerna för %1 är inte importerade. Importera dem på fliken Företag.").arg(year);
            return nullptr;
        }

        bool ok = false;
        tax = b->m_taxtables.taxOre(year, table, gross, &ok);
        if (!ok)
        {
            *error = QString("Skattetabell %1 för %2 har ingen rad för en lön på %3 kr.").arg(table).arg(year).arg(amount_format_ore(gross));
            return nullptr;
        }

        // The company pays the arbetsgivaravgift on top of the salary, and on
        // the salary alone: a skattefri ersättning carries none. The lower rate
        // applies to someone who had turned 66 when the year began.
        if (!b->m_employerfees.hasYear(year))
        {
            *error = QString("Arbetsgivaravgifterna för %1 är inte angivna. Lägg till dem på fliken Företag.").arg(year);
            return nullptr;
        }
        bool rate_ok = false;
        fee_rate = b->m_employerfees.rateFor(year, personnummer_birth_year(p_em->m_personnummer, year), &rate_ok);
        fee = employer_fee_ore(gross, fee_rate);
    }

    auto p_slip = std::make_unique<SalarySlip>();
    p_slip->m_payment_date          = draft.m_payment_date;

    p_slip->m_company_name          = b->m_companyinfo.m_name;
    p_slip->m_company_adrsline1     = b->m_companyinfo.m_adrsline1;
    p_slip->m_company_adrsline2     = b->m_companyinfo.m_adrsline2;
    p_slip->m_company_org_number    = b->m_companyinfo.m_org_number;

    p_slip->m_employee_id           = p_em->id();
    p_slip->m_employee_name         = p_em->name();
    p_slip->m_employee_adrsline1    = p_em->m_adrsline1;
    p_slip->m_employee_adrsline2    = p_em->m_adrsline2;
    p_slip->m_employment_number     = p_em->employmentNumber();
    p_slip->m_personnummer          = p_em->m_personnummer;
    p_slip->m_bank_account          = p_em->m_bank_account;

    p_slip->m_tax_table             = table;
    p_slip->m_employer_fee_rate     = fee_rate;
    p_slip->m_employer_fee          = fee;
    p_slip->m_lines                 = draft.m_lines;
    p_slip->m_gross                 = gross;
    p_slip->m_tax                   = tax;
    p_slip->m_tax_free              = tax_free;
    p_slip->m_net                   = gross - tax + tax_free;

    return p_slip;
}
