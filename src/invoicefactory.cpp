#include "data/database.h"

#include "invoicefactory.h"

#define DATE_FORMAT_STRING          "yyyy-MM-dd"

InvoiceFactory::InvoiceFactory() {}

Invoice* InvoiceFactory::create(InvoiceDraft* p_draft)
{
    QStringList months = { "Januari", "Februari", "Mars", "April", "Maj", "Juni", "Juli", "Augusti", "September", "Oktober", "November", "December" };

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

    Invoice* p_in = new Invoice();
    p_in->m_year                                = p_draft->m_invoice_date.year() % 100;
    p_in->m_count                               = DataBase::instance()->db()->m_invoices.nof_invoices(p_in->m_year) + 1;
    p_in->m_number                              = QStringLiteral("%1%2").arg(p_in->m_year, 2, 10, QLatin1Char('0')).arg(p_in->m_count, 2, 10, QLatin1Char('0'));

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

    p_in->m_customer_id                         = p_cu->id();
    p_in->m_customer_name                       = p_cu->m_name;
    p_in->m_customer_adrsline1                  = p_cu->m_adrsline1;
    p_in->m_customer_adrsline2                  = p_cu->m_adrsline2;
    p_in->m_customer_reference_name             = p_ag->m_reference_name;
    p_in->m_customer_email_invoice              = p_cu->m_email_invoice;

    p_in->m_agreement_id                        = p_ag->m_agreement_id;
    p_in->m_agreement_payment_terms_days        = p_ag->m_payment_terms_days;
    p_in->m_agreement_late_payment_interest     = p_ag->m_late_payment_interest;

    p_in->m_description_consultant_period       = QString("Konsult %1").arg(p_ag->m_reference_name_ours);
    if (p_draft->m_month < months.size())
    {
        p_in->m_description_consultant_period.append(QString(" Period %1").arg(months.at(p_draft->m_month)));
    }

    p_in->m_description_standard                = "Ordinarie arbetstid";
    p_in->m_hours_standard                      = p_draft->m_hours_standard;
    p_in->m_agreement_standard_hourly_rate      = p_ag->m_standard_hourly_rate;
    p_in->m_sum_standard                        = p_in->m_hours_standard * p_in->m_agreement_standard_hourly_rate;

    p_in->m_description_overtime                = "Övertid";
    p_in->m_hours_overtime                      = p_draft->m_hours_overtime;
    p_in->m_agreement_overtime_hourly_rate      = p_ag->m_overtime_hourly_rate;
    p_in->m_sum_overtime                        = p_in->m_hours_overtime * p_in->m_agreement_overtime_hourly_rate;

    p_in->m_description_qualified               = "Övertid (kvalificerad)";
    p_in->m_hours_qualified                     = p_draft->m_hours_qualified;
    p_in->m_agreement_qualified_hourly_rate     = p_ag->m_qualified_hourly_rate;
    p_in->m_sum_qualified                       = p_in->m_hours_qualified * p_in->m_agreement_qualified_hourly_rate;

    p_in->m_description_traveltime              = "Restid";
    p_in->m_hours_traveltime                    = p_draft->m_hours_traveltime;
    p_in->m_agreement_traveltime_hourly_rate    = p_ag->m_traveltime_hourly_rate;
    p_in->m_sum_traveltime                      = p_in->m_hours_traveltime * p_in->m_agreement_traveltime_hourly_rate;

    p_in->m_comment                             = p_draft->m_comment;

    p_in->m_sum                                 = p_in->m_sum_standard + p_in->m_sum_overtime + p_in->m_sum_qualified + p_in->m_sum_traveltime;
    p_in->m_vat                                 = (int)(
        (p_in->m_sum_standard * ((double)p_in->m_agreement_standard_vat/100)) +
        (p_in->m_sum_overtime * ((double)p_in->m_agreement_overtime_vat/100)) +
        (p_in->m_sum_qualified * ((double)p_in->m_agreement_qualified_vat/100)) +
        (p_in->m_sum_traveltime * ((double)p_in->m_agreement_traveltime_vat/100)));
    p_in->m_sum_including_vat                   = p_in->m_sum + p_in->m_vat;

    return p_in;
}
