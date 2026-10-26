#include "yearlist.h"

//-----------------------------------------------------------------------------
int year_to_add(const QList<int>& listed, const QList<int>& needed, int current_year)
{
    // The earliest year something has been paid out in that has no figures
    // yet. Working backwards is what entering a history comes down to.
    int missing = 0;
    for (int year : needed)
    {
        if (!listed.contains(year) && (missing == 0 || year < missing))
        {
            missing = year;
        }
    }
    if (missing > 0)
    {
        return missing;
    }

    // Otherwise the year after the latest one listed: the next year's figures
    // are what is added once the years that are in use are covered.
    int latest = 0;
    for (int year : listed)
    {
        if (year > latest)
        {
            latest = year;
        }
    }
    return latest > 0 ? latest + 1 : current_year;
}
