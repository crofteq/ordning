#include <QDebug>

#include "customers.h"

Customers::Customers() {}

//-----------------------------------------------------------------------------
void Customers::clear(void)
{
    m_list.clear();
}

//-----------------------------------------------------------------------------
void Customers::update(Customer* c)
{
    if (c->id() <= 0)
    {
        int nextid = 1;
        for(int i=0; i<m_list.size(); i++)
        {
            if (m_list.at(i)->id() >= nextid)
            {
                nextid = m_list.at(i)->id() + 1;
            }
        }
        c->setId(nextid);
        m_list.append(c);
    }
    else
    {
        for(int i=0; i<m_list.size(); i++)
        {
            if (m_list.at(i)->id() == c->id())
            {
                m_list.at(i)->update(c);
                break;
            }
        }
    }
}

//-----------------------------------------------------------------------------
void Customers::remove(Customer* c)
{
    for (int i=0; i<m_list.size(); i++)
    {
        if (m_list.at(i)->id() == c->id())
        {
            m_list.removeAt(i);
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
