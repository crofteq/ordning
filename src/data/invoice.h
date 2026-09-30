#ifndef INVOICE_H
#define INVOICE_H

#include <QString>

#include "agreement.h"

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
    QString m_company_web;                  // www.company.com

    int m_customer_id;
    QString m_customer_name;
    QString m_customer_adrsline1;
    QString m_customer_adrsline2;
    QString m_customer_reference_name;
    QString m_customer_email_invoice;

    QString m_agreement_id;                 // AG-1, the avtalsid as it is printed
    QString m_agreement_name;               // "Utveckling", the uppdrag it was for
    // Which agreement of the customer it was: two agreements may carry the
    // same avtalsid and still be different uppdrag, so the id it is stored
    // under is what tells them apart. 0 for an invoice from before this was
    // kept with it.
    int m_agreement_key = 0;
    int m_agreement_payment_terms_days;     // The number of days to pay the invoice
    int m_agreement_late_payment_interest;  // e.g. 8 %

    // Which employee was the consultant on the invoice, and the name that was
    // printed for them. The name is kept with the invoice like every other
    // detail it was sent with, so renaming or removing the employee later
    // leaves what the customer got alone. -1 for an invoice from before a
    // consultant was picked, and for one whose consultant could not be worked
    // out from the name it carries.
    int m_employee_id = -1;
    QString m_consultant_name;              // "Namn Namnsson"

    QString m_description_consultant_period; // "Konsult Namn Namnsson Period Januari"

    // Hours are hundredths of an hour and every amount is in öre, so 10 h at
    // 1000 kr/h is 1000 and 100000, summing to 1000000. See util/amount.h.
    int m_hours_standard = 0;                    // 10 h
    long long m_agreement_standard_hourly_rate;  // 1000 SEK
    int m_agreement_standard_vat;                // 25 %
    long long m_sum_standard;                    // 10000 SEK
    QString m_description_standard;              // "Normal arbetstid"

    int m_hours_overtime = 0;                    // 2 h
    long long m_agreement_overtime_hourly_rate;  // 1500 SEK
    int m_agreement_overtime_vat;                // 25 %
    long long m_sum_overtime;                    // 3000 SEK
    QString m_description_overtime;              // "Övertidsersättning"

    int m_hours_qualified = 0;                   // 1 h
    long long m_agreement_qualified_hourly_rate; // 2000 SEK
    int m_agreement_qualified_vat;               // 25 %
    long long m_sum_qualified;                   // 2000 SEK
    QString m_description_qualified;             // "Övertidsersättning (kvalificerad)"

    int m_hours_traveltime = 0;                  // 4 h
    long long m_agreement_traveltime_hourly_rate;// 500 SEK
    int m_agreement_traveltime_vat;              // 25 %
    long long m_sum_traveltime;                  // 2000 SEK
    QString m_description_traveltime;            // "Restidsersättning"

    QString m_comment;                      // Comments

    bool m_is_credit = false;
    QString m_credited_invoice_number;

    long long m_sum;
    long long m_vat;
    long long m_sum_including_vat;
    QString toString(void);

    // Says which of the customer's agreements the invoice was written under.
    // An invoice from before that was kept with it has none, and the reports
    // then cannot tell one uppdrag from another; this is how such an invoice
    // is put right. The amounts and the avtalsid it was printed with are left
    // alone: they are what the customer was sent.
    void setAgreement(const Agreement& agreement);

    int id(void) const { return m_id; }
    void setId(int id) { m_id = id; }

private:
    int m_id = -1;
};

#endif // INVOICE_H
