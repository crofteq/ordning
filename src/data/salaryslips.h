#ifndef SALARYSLIPS_H
#define SALARYSLIPS_H

#include <QList>

#include <memory>

#include "salaryslip.h"

// The "Totalt under året" figures of a salary slip, in öre.
struct SalaryYearToDate
{
    long long gross = 0;
    long long tax = 0;
    long long tax_free = 0;
};

// Owns the slips it holds: they are deleted by clear() and by the destructor.
// There is no remove(): a slip is a record of a salary that has been paid.
// Pointers handed out by get()/at() are observers.
class SalarySlips
{
public:
    SalarySlips();
    ~SalarySlips();

    // Owning collection, so copying one would mean deciding who deletes what.
    SalarySlips(const SalarySlips&) = delete;
    SalarySlips& operator=(const SalarySlips&) = delete;

    void clear(void);

    SalarySlip* get(int id);

    // Takes over `p` and assigns it the next free id. Returns the stored slip.
    SalarySlip* append(std::unique_ptr<SalarySlip> p);

    int size(void) const;
    SalarySlip* at(int i) const;

    // "Totalt under året": the figures of `slip` and of every earlier slip of
    // the same employee paid out in the same year. Slips paid on the same day
    // count in the order they were approved. A slip that is not stored yet,
    // such as a preview of a draft, counts as the latest.
    SalaryYearToDate yearToDate(const SalarySlip& slip) const;

    // Whether any slip was paid to `employee_id`.
    bool hasEmployee(int employee_id) const;

private:
    QList<SalarySlip*> m_list;
};

#endif // SALARYSLIPS_H
