#include "data/database.h"
#include "util/amount.h"

#include "invoicefactory.h"

#define DATE_FORMAT_STRING          "yyyy-MM-dd"

InvoiceFactory::InvoiceFactory() {}

//-----------------------------------------------------------------------------
// `percent` of `amount_ore`, rounded half away from zero.
static long long vat_of(long long amount_ore, int percent)
{
    const long long scaled = amount_ore * (long long)percent;
    const long long half = scaled < 0 ? -50 : 50;
    return (scaled + half) / 100;
}

std::unique_ptr<Invoice> InvoiceFactory::create(InvoiceDraft* p_draft)
{
    QStringList months = { "Januari", "Februari", "Mars", "April", "Maj", "Juni", "Juli", "Augusti", "September", "Oktober", "November", "December" };

    auto p_in = std::make_unique<Invoice>();
    p_in->m_year  = p_draft->m_invoice_date.year() % 100;
    p_in->m_count = DataBase::instance()->db()->m_invoices.nof_invoices(p_in->m_year) + 1;
    p_in->m_number = QStringLiteral("%1%2").arg(p_in->m_year, 2, 10, QLatin1Char('0')).arg(p_in->m_count, 2, 10, QLatin1Char('0'));

    if (p_draft->m_is_credit) {
        // Find the original invoice being credited
        Invoice* orig = nullptr;
        Invoices& invs = DataBase::instance()->db()->m_invoices;
        for (int i = 0; i < invs.size(); i++) {
            if (invs.at(i)->m_number == p_draft->m_credited_invoice_number) {
                orig = invs.at(i);
                break;
            }
        }
        if (orig == nullptr) {
            qDebug() << "Original invoice not found for credit:" << p_draft->m_credited_invoice_number;
            return nullptr;
        }

        // Copy all fields from original (company, customer, agreement, line items, descriptions)
        int newYear   = p_in->m_year;
        int newCount  = p_in->m_count;
        QString newNumber = p_in->m_number;
        *p_in = *orig;
        p_in->setId(-1);

        // Restore new invoice identity
        p_in->m_year   = newYear;
        p_in->m_count  = newCount;
        p_in->m_number = newNumber;
        p_in->m_date     = p_draft->m_invoice_date.toString(DATE_FORMAT_STRING);
        p_in->m_due_date = p_draft->m_invoice_date.addDays(orig->m_agreement_payment_terms_days).toString(DATE_FORMAT_STRING);

        // Negate all monetary amounts
        p_in->m_sum_standard      = -orig->m_sum_standard;
        p_in->m_sum_overtime      = -orig->m_sum_overtime;
        p_in->m_sum_qualified     = -orig->m_sum_qualified;
        p_in->m_sum_traveltime    = -orig->m_sum_traveltime;
        p_in->m_sum               = -orig->m_sum;
        p_in->m_vat               = -orig->m_vat;
        p_in->m_sum_including_vat = -orig->m_sum_including_vat;

        p_in->m_is_credit = true;
        p_in->m_credited_invoice_number = p_draft->m_credited_invoice_number;
        p_in->m_comment = QString("Denna kreditfaktura avser makulering av faktura %1 (%2).")
                              .arg(orig->m_number, orig->m_date);

        return p_in;
    }

    Customer* p_cu = DataBase::instance()->db()->m_customers.get(p_draft->m_customer_id);
    if (p_cu == nullptr)
    {
        qDebug() << "Customer not found!!";
        return nullptr;
    }

    Agreement* p_ag = p_cu->m_agreements.get(p_draft->m_agreement_id);
    if (p_ag == nullptr)
    {
        qDebug() << "Agreement not found!!";
        return nullptr;
    }

    Employee* p_em = DataBase::instance()->db()->m_employees.get(p_draft->m_employee_id);
    if (p_em == nullptr)
    {
        qDebug() << "Consultant not found!!";
        return nullptr;
    }

    p_in->m_date                                = p_draft->m_invoice_date.toString(DATE_FORMAT_STRING);
    p_in->m_due_date                            = p_draft->m_invoice_date.addDays(p_ag->m_payment_terms_days).toString(DATE_FORMAT_STRING);

    p_in->m_company_name                        = DataBase::instance()->db()->m_companyinfo.m_name;
    p_in->m_company_adrsline1                   = DataBase::instance()->db()->m_companyinfo.m_adrsline1;
    p_in->m_company_adrsline2                   = DataBase::instance()->db()->m_companyinfo.m_adrsline2;
    p_in->m_company_reference_name              = p_ag->m_reference_name_ours;
    p_in->m_company_org_number                  = DataBase::instance()->db()->m_companyinfo.m_org_number;
    p_in->m_company_bankgiro                    = DataBase::instance()->db()->m_companyinfo.m_bankgiro;
    p_in->m_company_phone                       = DataBase::instance()->db()->m_companyinfo.m_phone;
    p_in->m_company_vat_number                  = DataBase::instance()->db()->m_companyinfo.m_vat_number;
    p_in->m_company_swift_bic                   = DataBase::instance()->db()->m_companyinfo.m_swift_bic;
    p_in->m_company_email                       = DataBase::instance()->db()->m_companyinfo.m_email;
    p_in->m_company_iban                        = DataBase::instance()->db()->m_companyinfo.m_iban;
    p_in->m_company_web                         = DataBase::instance()->db()->m_companyinfo.m_web;

    p_in->m_customer_id                         = p_cu->id();
    p_in->m_customer_name                       = p_cu->m_name;
    p_in->m_customer_adrsline1                  = p_cu->m_adrsline1;
    p_in->m_customer_adrsline2                  = p_cu->m_adrsline2;
    p_in->m_customer_reference_name             = p_ag->m_reference_name;
    p_in->m_customer_email_invoice              = p_cu->m_email_invoice;

    p_in->m_agreement_id                        = p_ag->m_agreement_id;
    p_in->m_agreement_name                      = p_ag->m_agreement_name;
    p_in->m_agreement_key                       = p_ag->id();
    p_in->m_agreement_payment_terms_days        = p_ag->m_payment_terms_days;
    p_in->m_agreement_late_payment_interest     = p_ag->m_late_payment_interest;

    // The consultant is an employee picked on the draft; the agreement's "vår
    // referens" is who the customer turns to, which is not necessarily the
    // same person.
    p_in->m_employee_id                         = p_em->id();
    p_in->m_consultant_name                     = p_em->name();

    p_in->m_description_consultant_period       = QString("Konsult %1").arg(p_in->m_consultant_name);
    if (p_draft->m_month < months.size())
    {
        p_in->m_description_consultant_period.append(QString(" Period %1").arg(months.at(p_draft->m_month)));
    }

    // Each line is hours * rate, both scaled, so the sums stay exact in öre.
    p_in->m_description_standard                = "Ordinarie arbetstid";
    p_in->m_hours_standard                      = p_draft->m_hours_standard;
    p_in->m_agreement_standard_hourly_rate      = p_ag->m_standard_hourly_rate;
    p_in->m_sum_standard                        = amount_line_total_ore(p_in->m_agreement_standard_hourly_rate, p_in->m_hours_standard);

    p_in->m_description_overtime                = "Övertid";
    p_in->m_hours_overtime                      = p_draft->m_hours_overtime;
    p_in->m_agreement_overtime_hourly_rate      = p_ag->m_overtime_hourly_rate;
    p_in->m_sum_overtime                        = amount_line_total_ore(p_in->m_agreement_overtime_hourly_rate, p_in->m_hours_overtime);

    p_in->m_description_qualified               = "Övertid (kvalificerad)";
    p_in->m_hours_qualified                     = p_draft->m_hours_qualified;
    p_in->m_agreement_qualified_hourly_rate     = p_ag->m_qualified_hourly_rate;
    p_in->m_sum_qualified                       = amount_line_total_ore(p_in->m_agreement_qualified_hourly_rate, p_in->m_hours_qualified);

    p_in->m_description_traveltime              = "Restid";
    p_in->m_hours_traveltime                    = p_draft->m_hours_traveltime;
    p_in->m_agreement_traveltime_hourly_rate    = p_ag->m_traveltime_hourly_rate;
    p_in->m_sum_traveltime                      = amount_line_total_ore(p_in->m_agreement_traveltime_hourly_rate, p_in->m_hours_traveltime);

    p_in->m_comment                             = p_draft->m_comment;

    p_in->m_sum                                 = p_in->m_sum_standard + p_in->m_sum_overtime + p_in->m_sum_qualified + p_in->m_sum_traveltime;

    // VAT is charged per line, each rounded to a whole öre, so the printed
    // lines add up to the printed total.
    p_in->m_vat                                 =
        vat_of(p_in->m_sum_standard, p_in->m_agreement_standard_vat) +
        vat_of(p_in->m_sum_overtime, p_in->m_agreement_overtime_vat) +
        vat_of(p_in->m_sum_qualified, p_in->m_agreement_qualified_vat) +
        vat_of(p_in->m_sum_traveltime, p_in->m_agreement_traveltime_vat);
    p_in->m_sum_including_vat                   = p_in->m_sum + p_in->m_vat;

    return p_in;
}
