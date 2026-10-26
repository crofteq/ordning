#ifndef YEARLIST_H
#define YEARLIST_H

#include <QList>

// The year a "Lägg till år" button offers for a list that already holds
// `listed` and whose figures are needed for `needed`. A needed year that is
// not listed comes first, the earliest one: a lönebesked for a year gone by
// needs that year's skattetabell and arbetsgivaravgifter, and a button that
// only ever counts forwards from the latest year listed has to be typed over
// every time such a year is entered. Failing that it is the year after the
// latest one listed, and `current_year` when nothing is listed at all.
int year_to_add(const QList<int>& listed, const QList<int>& needed, int current_year);

#endif // YEARLIST_H
