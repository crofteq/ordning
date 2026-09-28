#include <QDate>
#include <QDebug>

#include "invoices.h"

//-----------------------------------------------------------------------------
Invoices::Invoices()
{
}

//-----------------------------------------------------------------------------
Invoices::~Invoices()
{
    clear();
}

//-----------------------------------------------------------------------------
int Invoices::nof_invoices(int year)
{
    return m_numbers.value(year);
}

//-----------------------------------------------------------------------------
void Invoices::clear(void)
{
    qDeleteAll(m_list);
    m_list.clear();
    m_numbers.clear();
}

//-----------------------------------------------------------------------------
Invoice* Invoices::get(int id)
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
Invoice* Invoices::append(std::unique_ptr<Invoice> owned)
{
    Invoice* p = owned.release();

    int max = 0;
    for (int i=0; i<m_list.size(); i++)
    {
        if (max < m_list.at(i)->id())
        {
            max = m_list.at(i)->id();
        }
    }
    p->setId(max + 1); // assign the next id
    m_list.append(p);

    // lets update the invoice counters
    if (m_numbers.contains(p->m_year))
    {
        if (p->m_count > m_numbers.value(p->m_year))
        {
            m_numbers.insert(p->m_year, p->m_count);
        }
    }
    else
    {
        m_numbers.insert(p->m_year, p->m_count);
    }

    return p;
}

//-----------------------------------------------------------------------------
void Invoices::remove(Invoice *p)
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
int Invoices::size(void) const
{
    return m_list.size();
}

//-----------------------------------------------------------------------------
Invoice* Invoices::at(int i) const
{
    return m_list.at(i);
}

//-----------------------------------------------------------------------------
QString Invoices::toString(void)
{
    QString s;
    s.append("--------------------------------------------------------------\n");
    for (int i=0; i<m_list.size(); i++)
    {
        s.append(m_list.at(i)->toString() + '\n');
    }
    return s + "\n";
}
