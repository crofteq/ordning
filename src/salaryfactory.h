#ifndef SALARYFACTORY_H
#define SALARYFACTORY_H

#include <memory>

#include "data/salarydraft.h"
#include "data/salaryslip.h"

class SalaryFactory
{
public:
    // Builds the salary slip a draft describes: a copy of the company and
    // employee details, the lines, and bruttolön, skatt, skattefri ersättning
    // and utbetalt. The tax comes from the skattetabell the employee has for
    // the year of the payment date, and is worked out on the bruttolön alone:
    // a skattefri line is paid out untaxed and carries no arbetsgivaravgift
    // either. A slip whose only lines are skattefria needs neither a
    // skattetabell nor the year's avgifter, since there is nothing to apply
    // them to. Returns nullptr and says why in `error` when the employee,
    // their skattetabell for that year or the imported tax table is missing,
    // when nothing at all would be paid out, when the arbetsgivaravgifter for
    // the year are missing, or when there are more lines than
    // SALARY_MAX_LINES. The
    // caller owns the result until it is handed to SalarySlips::append().
    static std::unique_ptr<SalarySlip> create(const SalaryDraft& draft, QString* error);
};

#endif // SALARYFACTORY_H
