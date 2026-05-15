#ifndef DATABASE_H
#define DATABASE_H

#include <QObject>
#include <QMutex>
#include <memory>

#include "business.h"
#include "idatastore.h"

class DataBase : public QObject
{
    Q_OBJECT
public:
    // Public static method to access the singleton instance
    static DataBase* instance() {
        static QMutex mutex;
        if (!m_instance) {
            QMutexLocker locker(&mutex);
            if (!m_instance) {
                m_instance = new DataBase();
            }
        }
        return m_instance;
    }

    // Deleting copy constructor and assignment operator to prevent copying
    DataBase(const DataBase&) = delete;
    DataBase& operator=(const DataBase&) = delete;

    Business* db();
    void clear(void);

    // SQLite persistence
    bool loadSqlite(const QString& filename);
    bool saveSqlite(const QString& filename);

    // Allow swapping the datastore implementation (for tests / future extensions)
    void setDataStore(std::unique_ptr<IDataStore> store);

signals:
    void updated(void);

protected:
    // Protected constructor to prevent instantiation from outside the class
    DataBase(QObject *parent = nullptr);

private:
    // Static pointer to hold the instance
    static DataBase* m_instance;
    Business* m_b;
    std::unique_ptr<IDataStore> m_store;
};

#endif // DATABASE_H
