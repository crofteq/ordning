#include "salarydrafts.h"

SalaryDrafts::SalaryDrafts() {}

//-----------------------------------------------------------------------------
SalaryDrafts::~SalaryDrafts()
{
    clear();
}

//-----------------------------------------------------------------------------
void SalaryDrafts::clear(void)
{
    qDeleteAll(m_list);
    m_list.clear();
}

//-----------------------------------------------------------------------------
SalaryDraft* SalaryDrafts::get(int id)
{
    for (int i=0; i<m_list.size(); i++)
    {
        if (m_list.at(i)->id() == id)
        {
            return m_list.at(i);
        }
    }
    return nullptr;
}

//-----------------------------------------------------------------------------
SalaryDraft* SalaryDrafts::update(const SalaryDraft& d)
{
    if (d.id() > 0)
    {
        SalaryDraft* stored = get(d.id());
        if (stored != nullptr)
        {
            stored->update(d);
        }
        return stored;
    }

    int nextid = 1;
    for (int i=0; i<m_list.size(); i++)
    {
        if (m_list.at(i)->id() >= nextid)
        {
            nextid = m_list.at(i)->id() + 1;
        }
    }

    SalaryDraft* stored = new SalaryDraft();
    stored->update(d);
    stored->setId(nextid);
    m_list.append(stored);
    return stored;
}

//-----------------------------------------------------------------------------
void SalaryDrafts::remove(SalaryDraft* d)
{
    for (int i=0; i<m_list.size(); i++)
    {
        if (m_list.at(i)->id() == d->id())
        {
            delete m_list.takeAt(i);
            break;
        }
    }
}

//-----------------------------------------------------------------------------
int SalaryDrafts::size(void) const
{
    return m_list.size();
}

//-----------------------------------------------------------------------------
SalaryDraft* SalaryDrafts::at(int i) const
{
    return m_list.at(i);
}
