#ifndef AGREEMENTS_H
#define AGREEMENTS_H

#include "agreement.h"

class Agreements
{
public:
    Agreements();

    Agreement* get(int id);
    void update(Agreement *a);
    void remove(Agreement *a);
    int size(void);
    Agreement* at(int i);
    QString toString(void);

    int id(void) { return m_id; }
    void setId(int id) { m_id = id; }

private:
    int m_id = -1;

private:
    QList<Agreement*> m_list;
};

#endif // AGREEMENTS_H
