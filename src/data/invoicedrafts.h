#ifndef INVOICEDRAFTS_H
#define INVOICEDRAFTS_H

#include "invoicedraft.h"

// Owns the drafts it holds: they are deleted by clear(), by remove() and by the
// destructor. Pointers handed out by at() are observers and stay valid until
// the draft is removed or the collection goes away.
class InvoiceDrafts
{
public:
    InvoiceDrafts();
    ~InvoiceDrafts();

    // Owning collection, so copying one would mean deciding who deletes what.
    InvoiceDrafts(const InvoiceDrafts&) = delete;
    InvoiceDrafts& operator=(const InvoiceDrafts&) = delete;

    void clear(void);

    // Copies `p` onto the draft with the same id, or stores a copy of it under
    // the next free id. `p` itself is never kept, so the caller stays the owner
    // of whatever it passed in. Returns the stored draft.
    InvoiceDraft* update(const InvoiceDraft& p);

    void remove(InvoiceDraft *p);

    int size() const {return m_list.size();}
    InvoiceDraft* at(int i) const {return m_list.at(i); }
    QString toString(void);

    int id(void) const { return m_id; }
    void setId(int id) { m_id = id; }

private:
    int m_id = -1;

private:
    QList<InvoiceDraft*> m_list;
};

#endif // INVOICEDRAFTS_H
