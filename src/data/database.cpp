#include <QDebug>
#include <QFile>
#include <QIODevice>
#include <QString>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QUuid>
#include <QDateTime>

#include "database.h"
#include "idatastore.h"
#include "sqlitedatastore.h"

// Define the static member outside the class
DataBase* DataBase::m_instance = nullptr;

//-----------------------------------------------------------------------------
Business* DataBase::db()
{
    return m_b;
}

//-----------------------------------------------------------------------------
void DataBase::clear(void)
{
    m_b->clear();

    // An empty business matches nothing on disk, but there is also nothing in
    // it to lose, so it does not count as unsaved work.
    setModified(false);

    emit updated();
}

//-----------------------------------------------------------------------------
void DataBase::setModified(bool modified)
{
    if (m_modified == modified)
    {
        return;
    }
    m_modified = modified;
    emit modifiedChanged(m_modified);
}

//-----------------------------------------------------------------------------
DataBase::DataBase(QObject* parent) : QObject(parent)
{
    m_b = new Business();
    m_store = std::make_unique<SqliteDataStore>();
}

//-----------------------------------------------------------------------------
void DataBase::setDataStore(std::unique_ptr<IDataStore> store)
{
    m_store = std::move(store);
}

//-----------------------------------------------------------------------------
bool DataBase::saveSqlite(const QString& filename)
{
    if (!m_store) {
        qWarning() << "No IDataStore configured for saveSqlite";
        return false;
    }
    if (!m_store->save(m_b, filename))
    {
        return false;
    }
    setModified(false);
    return true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void DataBase::notifyUpdated(void)
{
    setModified(true);
    emit updated();
}

//-----------------------------------------------------------------------------
bool DataBase::loadSqlite(const QString& filename)
{
    if (!m_store) {
        qWarning() << "No IDataStore configured for loadSqlite";
        return false;
    }

    bool ok = m_store->load(m_b, filename);
    if (ok)
    {
        setModified(false);
        emit updated();
    }
    return ok;
}
