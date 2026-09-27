#include <QDebug>

#include "agreements.h"

//-----------------------------------------------------------------------------
Agreements::Agreements()
{
}

//-----------------------------------------------------------------------------
Agreements::~Agreements()
{
    clear();
}

//-----------------------------------------------------------------------------
void Agreements::clear(void)
{
    qDeleteAll(m_list);
    m_list.clear();
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
Agreement* Agreements::update(const Agreement& a)
{
    int max = 0;
    for (int i=0; i<m_list.size(); i++)
    {
        if (max < m_list.at(i)->id())
        {
            max = m_list.at(i)->id();
        }
        if (a.id() == m_list.at(i)->id())
        {
            m_list.at(i)->update(a);
            return m_list.at(i);
        }
    }

    Agreement* stored = new Agreement();
    stored->update(a);
    stored->setId(max + 1); // assign the next id
    m_list.append(stored);
    return stored;
}

//-----------------------------------------------------------------------------
void Agreements::remove(Agreement *a)
{
    for (int i=0; i<m_list.size(); i++)
    {
        if (m_list.at(i)->id() == a->id())
        {
            delete m_list.takeAt(i);
            break;
        }
    }
}

//-----------------------------------------------------------------------------
int Agreements::size(void) const
{
    return m_list.size();
}

//-----------------------------------------------------------------------------
Agreement* Agreements::at(int i) const
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
