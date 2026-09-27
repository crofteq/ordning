#include <QDebug>

#include "customers.h"

Customers::Customers() {}

//-----------------------------------------------------------------------------
Customers::~Customers()
{
    clear();
}

//-----------------------------------------------------------------------------
void Customers::clear(void)
{
    qDeleteAll(m_list);
    m_list.clear();
}

//-----------------------------------------------------------------------------
Customer* Customers::update(const Customer& c)
{
    if (c.id() > 0)
    {
        for(int i=0; i<m_list.size(); i++)
        {
            if (m_list.at(i)->id() == c.id())
            {
                m_list.at(i)->update(c);
                return m_list.at(i);
            }
        }
        return nullptr;
    }

    int nextid = 1;
    for(int i=0; i<m_list.size(); i++)
    {
        if (m_list.at(i)->id() >= nextid)
        {
            nextid = m_list.at(i)->id() + 1;
        }
    }

    Customer* stored = new Customer();
    stored->update(c);
    stored->setId(nextid);
    m_list.append(stored);
    return stored;
}

//-----------------------------------------------------------------------------
void Customers::remove(Customer* c)
{
    for (int i=0; i<m_list.size(); i++)
    {
        if (m_list.at(i)->id() == c->id())
        {
            delete m_list.takeAt(i);
            break;
        }
    }
}

//-----------------------------------------------------------------------------
Customer* Customers::get(int id)
{
    for(int i=0; i<m_list.size(); i++)
    {
        if (m_list.at(i)->id() == id)
        {
            return m_list.at(i);
        }
    }
    return nullptr;
}

//-----------------------------------------------------------------------------
int Customers::size(void) const
{
    return m_list.size();
}

//-----------------------------------------------------------------------------
Customer* Customers::at(int i) const
{
    return m_list.at(i);
}

//-----------------------------------------------------------------------------
QString Customers::toString(void)
{
    QString s;
    s.append("--------------------------------------------------------------\n");
    s.append("--------------------------------------------------------------\n");
    for(int i=0; i<m_list.size(); i++)
    {
        s.append(m_list.at(i)->toString() + '\n');
    }
    return s;
}
