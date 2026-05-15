#ifndef BUSINESS_H
#define BUSINESS_H

#include "companyinfo.h"
#include "customers.h"
#include "invoicedrafts.h"
#include "invoices.h"

class Business
{
public:
    Business();

    CompanyInfo m_companyinfo;
    Customers m_customers;
    InvoiceDrafts m_invoicedrafts;
    Invoices m_invoices;

    void clear(void);
    QString toString(void);

private:
    int m_version = 1;

};

#endif // BUSINESS_H
