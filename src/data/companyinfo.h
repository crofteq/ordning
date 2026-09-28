#ifndef COMPANYINFO_H
#define COMPANYINFO_H

#include <QString>

class CompanyInfo
{
public:
    CompanyInfo();


    QString m_name;
    QString m_adrsline1;
    QString m_adrsline2;
    QString m_org_number;
    QString m_vat_number;
    QString m_bankgiro;
    QString m_swift_bic;
    QString m_iban;
    QString m_phone;
    QString m_email;
    QString m_web;
    QString m_logo;
    // The month the räkenskapsår begins in, 1 for the calendar year.
    int m_fiscal_year_start_month;

    void clear(void);
    QString toString(void);
};

#endif // COMPANYINFO_H
