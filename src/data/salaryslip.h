#ifndef SALARYSLIP_H
#define SALARYSLIP_H

#include <QDate>
#include <QList>
#include <QString>

#include "salarydraft.h"

// An approved salary statement. Everything it prints is copied in when it is
// created, so later changes to the company, the employee or the tax tables do
// not alter a statement that has been paid out.
class SalarySlip
{
public:
    SalarySlip();

    QDate m_payment_date;

    QString m_company_name;
    QString m_company_adrsline1;
    QString m_company_adrsline2;
    QString m_company_org_number;

    int m_employee_id = -1;     // kept for the year-to-date totals
    QString m_employee_name;
    QString m_employee_adrsline1;
    QString m_employee_adrsline2;
    QString m_employment_number;
    QString m_personnummer;
    QString m_bank_account;

    int m_tax_table = 0;
    // The arbetsgivaravgift the company pays on this salary, and the rate it
    // was worked out at (hundredths of a percent). Kept with the slip so that
    // a later change of the rates does not alter a salary already paid.
    int m_employer_fee_rate = 0;
    long long m_employer_fee = 0;
    QList<SalaryLine> m_lines;

    // All in öre. The skattefria ersättningar are paid out without being taxed,
    // so utbetalt is bruttolön less skatt plus them.
    long long m_gross = 0;
    long long m_tax = 0;
    long long m_tax_free = 0;
    long long m_net = 0;

    int id(void) const { return m_id; }
    void setId(int id) { m_id = id; }

private:
    int m_id = -1;
};

#endif // SALARYSLIP_H
