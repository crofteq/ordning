#ifndef SALARYDRAFTS_H
#define SALARYDRAFTS_H

#include <QList>

#include "salarydraft.h"

// Owns the drafts it holds: they are deleted by clear(), by remove() and by the
// destructor. Pointers handed out by get()/at() are observers and stay valid
// until the draft is removed or the collection goes away.
class SalaryDrafts
{
public:
    SalaryDrafts();
    ~SalaryDrafts();

    // Owning collection, so copying one would mean deciding who deletes what.
    SalaryDrafts(const SalaryDrafts&) = delete;
    SalaryDrafts& operator=(const SalaryDrafts&) = delete;

    void clear(void);

    SalaryDraft* get(int id);

    // Copies `d` onto the draft with the same id, or stores a copy of it under
    // the next free id. `d` itself is never kept. Returns the stored draft, or
    // nullptr for an id that is not there.
    SalaryDraft* update(const SalaryDraft& d);

    void remove(SalaryDraft* d);
    int size(void) const;
    SalaryDraft* at(int i) const;

private:
    QList<SalaryDraft*> m_list;
};

#endif // SALARYDRAFTS_H
