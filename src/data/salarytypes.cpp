#include "salarytypes.h"

//-----------------------------------------------------------------------------
// Only the lönearter the application actually pays out are here: an option that
// nothing can fill in is an option to pick by mistake. Adding one — restids-
// ersättning, milersättning — is a line in this table, and nothing else, as
// long as it is lön or a skattefri ersättning. A löneart that is neither is
// more than a line: a skattepliktig förmån is taxed without being paid out and
// a nettolöneavdrag is taken after tax, and neither has anywhere to go in
// SalaryFactory's arithmetic yet.
const QList<SalaryType>& salary_types(void)
{
    static const QList<SalaryType> types = {
        { SALARY_TYPE_MONTHLY_SALARY, "Månadslön",               SALARY_LINE_TAXABLE },
        { SALARY_TYPE_TRAKTAMENTE,    "Skattefritt traktamente", SALARY_LINE_TAX_FREE },
    };
    return types;
}

//-----------------------------------------------------------------------------
const SalaryType* salary_type(int number)
{
    const QList<SalaryType>& types = salary_types();
    for (const SalaryType& type : types)
    {
        if (type.number == number)
        {
            return &type;
        }
    }
    return nullptr;
}
