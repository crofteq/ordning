#include <QDebug>

#include "agreements.h"

//-----------------------------------------------------------------------------
Agreements::Agreements()
{
}

//-----------------------------------------------------------------------------
Agreement* Agreements::get(int id)
{
    for (int i=0; i<m_list.size(); i++)
    {
        if (id == m_list.at(i)->id())
        {
            return m_list.at(i);
        }
    }
    return nullptr;
}

//-----------------------------------------------------------------------------
void Agreements::update(Agreement *a)
{
    int max = 0;
    for (int i=0; i<m_list.size(); i++)
    {
        if (max < m_list.at(i)->id())
        {
            max = m_list.at(i)->id();
        }
        if (a->id() == m_list.at(i)->id())
        {
            m_list.at(i)->update(a);
            return;
        }
    }
    a->setId(max + 1); // assign the next id
    m_list.append(a);
}

//-----------------------------------------------------------------------------
void Agreements::remove(Agreement *a)
{
    for (int i=0; i<m_list.size(); i++)
    {
        if (m_list.at(i)->id() == a->id())
        {
            m_list.removeAt(i);
            break;
        }
    }
}

//-----------------------------------------------------------------------------
int Agreements::size(void)
{
    return m_list.size();
}

//-----------------------------------------------------------------------------
Agreement* Agreements::at(int i)
{
    return m_list.at(i);
}

//-----------------------------------------------------------------------------
QString Agreements::toString(void)
{
    QString s;
    s.append("--------------------------------------------------------------\n");
    for (int i=0; i<m_list.size(); i++)
    {
        s.append(m_list.at(i)->toString() + '\n');
    }
    return s + "\n";
}
