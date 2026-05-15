#ifndef SQLITEDATASTORE_H
#define SQLITEDATASTORE_H

#include "idatastore.h"
#include <QString>

class SqliteDataStore : public IDataStore
{
public:
    SqliteDataStore() = default;
    ~SqliteDataStore() override = default;

    bool save(Business* b, const QString& filename) override;
    bool load(Business* b, const QString& filename) override;
};

#endif // SQLITEDATASTORE_H
