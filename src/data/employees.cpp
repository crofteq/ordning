#include <QDebug>

#include "employees.h"

Employees::Employees() {}

//-----------------------------------------------------------------------------
Employees::~Employees()
{
    clear();
}

//-----------------------------------------------------------------------------
void Employees::clear(void)
{
    qDeleteAll(m_list);
    m_list.clear();
}

//-----------------------------------------------------------------------------
Employee* Employees::update(const Employee& e)
{
    if (e.id() > 0)
    {
        for(int i=0; i<m_list.size(); i++)
        {
            if (m_list.at(i)->id() == e.id())
            {
                m_list.at(i)->update(e);
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

    Employee* stored = new Employee();
    stored->update(e);
    stored->setId(nextid);
    m_list.append(stored);
    return stored;
}

//-----------------------------------------------------------------------------
void Employees::remove(Employee* e)
{
    for (int i=0; i<m_list.size(); i++)
    {
        if (m_list.at(i)->id() == e->id())
        {
            delete m_list.takeAt(i);
            break;
        }
    }
}

//-----------------------------------------------------------------------------
Employee* Employees::get(int id)
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
int Employees::size(void) const
{
    return m_list.size();
}

//-----------------------------------------------------------------------------
Employee* Employees::at(int i) const
{
    return m_list.at(i);
}

//-----------------------------------------------------------------------------
QString Employees::toString(void)
{
    QString s;
    s.append("--------------------------------------------------------------\n");
    for(int i=0; i<m_list.size(); i++)
    {
        s.append(m_list.at(i)->toString() + '\n');
    }
    return s;
}
