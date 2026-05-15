#include "invoicedrafts.h"

//-----------------------------------------------------------------------------
InvoiceDrafts::InvoiceDrafts()
{
}

//-----------------------------------------------------------------------------
void InvoiceDrafts::clear(void)
{
    m_list.clear();
}

//-----------------------------------------------------------------------------
void InvoiceDrafts::update(InvoiceDraft *p)
{
    for (int i=0; i<m_list.size(); i++)
    {
        if (m_list.at(i)->update(p))
        {
            return;
        }
    }

    {
        int max = 0;
        for (int i=0; i<m_list.size(); i++)
        {
            if (m_list.at(i)->id() > max)
            {
                max = m_list.at(i)->id();
            }
        }
        p->setId(max + 1); // assign the next id
        m_list.append(p);
    }
}

//-----------------------------------------------------------------------------
void InvoiceDrafts::remove(InvoiceDraft *p)
{
    for (int i=0; i<m_list.size(); i++)
    {
        if (m_list.at(i)->id() == p->id())
        {
            m_list.removeAt(i);
            break;
        }
    }
}

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------

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
