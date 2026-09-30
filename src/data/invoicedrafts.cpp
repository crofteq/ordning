#include "invoicedrafts.h"

//-----------------------------------------------------------------------------
InvoiceDrafts::InvoiceDrafts()
{
}

//-----------------------------------------------------------------------------
InvoiceDrafts::~InvoiceDrafts()
{
    clear();
}

//-----------------------------------------------------------------------------
void InvoiceDrafts::clear(void)
{
    qDeleteAll(m_list);
    m_list.clear();
}

//-----------------------------------------------------------------------------
InvoiceDraft* InvoiceDrafts::update(const InvoiceDraft& p)
{
    int max = 0;
    for (int i=0; i<m_list.size(); i++)
    {
        if (m_list.at(i)->id() > max)
        {
            max = m_list.at(i)->id();
        }
        if (m_list.at(i)->update(p))
        {
            return m_list.at(i);
        }
    }

    InvoiceDraft* stored = new InvoiceDraft();
    stored->setId(p.id());
    stored->update(p);
    stored->setId(max + 1); // assign the next id
    m_list.append(stored);
    return stored;
}

//-----------------------------------------------------------------------------
void InvoiceDrafts::remove(InvoiceDraft *p)
{
    for (int i=0; i<m_list.size(); i++)
    {
        if (m_list.at(i)->id() == p->id())
        {
            delete m_list.takeAt(i);
            break;
        }
    }
}

//-----------------------------------------------------------------------------
QString InvoiceDrafts::toString(void)
{
    QString s;
    s.append("--------------------------------------------------------------\n");
    for (int i=0; i<m_list.size(); i++)
    {
        s.append(m_list.at(i)->toString() + '\n');
    }
    return s + "\n";
}
