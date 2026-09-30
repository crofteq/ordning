#ifndef BUSINESS_H
#define BUSINESS_H

#include "companyinfo.h"
#include "customers.h"
#include "employees.h"
#include "invoicedrafts.h"
#include "invoices.h"
#include "salarydrafts.h"
#include "salaryslips.h"
#include "employerfees.h"
#include "taxtables.h"
#include "traktamente.h"

class Business
{
public:
    Business();

    CompanyInfo m_companyinfo;
    Customers m_customers;
    Employees m_employees;
    InvoiceDrafts m_invoicedrafts;
    Invoices m_invoices;
    TaxTables m_taxtables;
    EmployerFees m_employerfees;
    Traktamente m_traktamente;
    SalaryDrafts m_salarydrafts;
    SalarySlips m_salaryslips;

    void clear(void);

    // Whether a salary draft or slip belongs to the employee. Such an
    // employee must not be deleted: the slips' year-to-date totals would
    // otherwise join up with whoever is given the id next.
    bool employeeInUse(int employee_id) const;

    // The years a salary has been paid out in, earliest first: the payment
    // year of every löneunderlag and every lönebesked, of `employee_id` or of
    // everybody when it is 0, which is no employee's id. These are the years
    // the skattetabeller, the arbetsgivaravgifter and traktamentena have to
    // cover, and so the years the tabs offer to add figures for.
    QList<int> salaryYears(int employee_id = 0) const;
    QString toString(void);

private:
    int m_version = 1;

};

#endif // BUSINESS_H
