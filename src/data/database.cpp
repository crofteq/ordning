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

    emit updated();
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
    return m_store->save(m_b, filename);
}

//-----------------------------------------------------------------------------
bool DataBase::loadSqlite(const QString& filename)
{
    if (!m_store) {
        qWarning() << "No IDataStore configured for loadSqlite";
        return false;
    }

    bool ok = m_store->load(m_b, filename);
    if (ok) emit updated();
    return ok;
}
