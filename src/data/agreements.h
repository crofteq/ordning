#ifndef AGREEMENTS_H
#define AGREEMENTS_H

#include "agreement.h"

// Owns the agreements it holds: they are deleted by clear(), by remove() and by
// the destructor. Pointers handed out by get()/at() are observers and stay
// valid until the agreement is removed or the collection goes away.
class Agreements
{
public:
    Agreements();
    ~Agreements();

    // Owning collection, so copying one would mean deciding who deletes what.
    Agreements(const Agreements&) = delete;
    Agreements& operator=(const Agreements&) = delete;

    void clear(void);

    Agreement* get(int id);

    // Copies `a` onto the agreement with the same id, or stores a copy of it
    // under the next free id. `a` itself is never kept, so the caller stays the
    // owner of whatever it passed in. Returns the stored agreement.
    Agreement* update(const Agreement& a);

    void remove(Agreement *a);
    int size(void) const;
    Agreement* at(int i) const;
    QString toString(void);

    int id(void) const { return m_id; }
    void setId(int id) { m_id = id; }

private:
    int m_id = -1;

private:
    QList<Agreement*> m_list;
};

#endif // AGREEMENTS_H
