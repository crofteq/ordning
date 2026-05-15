#ifndef INVOICE_H
#define INVOICE_H

#include <QString>

class Invoice
{
public:
    Invoice();

    int m_year;                             // 25
    int m_count;                            // 11
    QString m_number;                       // 2511

    QString m_date;
    QString m_due_date;

    QString m_company_name;
    QString m_company_adrsline1;
    QString m_company_adrsline2;
    QString m_company_reference_name;
    QString m_company_org_number;           // 554433-2211
    QString m_company_bankgiro;             // 321-7654
    QString m_company_phone;                // +46 (0)732 12 34 56
    QString m_company_vat_number;           // SE554433221101
    QString m_company_swift_bic;            // HANDBANK
    QString m_company_email;                // info@company.com
    QString m_company_f_skatt;              // "Innehar F-skattsedel"
    QString m_company_iban;                 // SE01 1234 1234 1234 1234 1234

    int m_customer_id;
    QString m_customer_name;
    QString m_customer_adrsline1;
    QString m_customer_adrsline2;
    QString m_customer_reference_name;
    QString m_customer_email_invoice;

    QString m_agreement_id;
    int m_agreement_payment_terms_days;     // The number of days to pay the invoice
    int m_agreement_late_payment_interest;  // e.g. 8 %

    QString m_description_consultant_period; // "Konsult Namn Namnsson Period Januari"

    int m_hours_standard;                   // 10 h
    int m_agreement_standard_hourly_rate;   // 1000 SEK
    int m_agreement_standard_vat;           // 25 %
    int m_sum_standard;                     // 10000 SEK
    QString m_description_standard;         // "Normal arbetstid"

    int m_hours_overtime;                   // 2 h
    int m_agreement_overtime_hourly_rate;   // 1500 SEK
    int m_agreement_overtime_vat;           // 25 %
    int m_sum_overtime;                     // 3000 SEK
    QString m_description_overtime;         // "Övertidsersättning"

    int m_hours_qualified;                  // 1 h
    int m_agreement_qualified_hourly_rate;  // 2000 SEK
    int m_agreement_qualified_vat;          // 25 %
    int m_sum_qualified;                    // 2000 SEK
    QString m_description_qualified;        // "Övertidsersättning (kvalificerad)"

    int m_hours_traveltime;                 // 4 h
    int m_agreement_traveltime_hourly_rate; // 500 SEK
    int m_agreement_traveltime_vat;         // 25 %
    int m_sum_traveltime;                   // 2000 SEK
    QString m_description_traveltime;       // "Restidsersättning"

    QString m_comment;                      // Comments

    int m_sum;
    int m_vat;
    int m_sum_including_vat;
    QString toString(void);

    int id(void) { return m_id; }
    void setId(int id) { m_id = id; }

private:
    int m_id = -1;
};

#endif // INVOICE_H
