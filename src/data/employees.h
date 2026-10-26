#ifndef EMPLOYEES_H
#define EMPLOYEES_H

#include <QList>

#include "employee.h"

// Owns the employees it holds: they are deleted by clear(), by remove() and by
// the destructor. Pointers handed out by get()/at() are observers and stay
// valid until the employee is removed or the collection goes away.
class Employees
{
public:
    Employees();
    ~Employees();

    // Owning collection, so copying one would mean deciding who deletes what.
    Employees(const Employees&) = delete;
    Employees& operator=(const Employees&) = delete;

    void clear(void);

    Employee* get(int id);

    // Copies `e` onto the employee with the same id, or stores a copy of it
    // under the next free id. `e` itself is never kept. Returns the stored
    // employee, or nullptr for an id that is not there.
    Employee* update(const Employee& e);

    void remove(Employee* e);
    int size(void) const;
    Employee* at(int i) const;
    QString toString(void);

private:
    QList<Employee*> m_list;
};

#endif // EMPLOYEES_H
