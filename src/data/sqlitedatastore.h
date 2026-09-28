#ifndef SQLITEDATASTORE_H
#define SQLITEDATASTORE_H

#include "idatastore.h"
#include <QString>

// Where to put a copy of `filename` taken before something is done to it that
// cannot be undone. `label` says what it was taken before ("v3" for a schema
// migration), and the name carries the date as well so the copies can be told
// apart. The original extension is kept so the copy opens directly, and a
// counter is added if the name is taken.
QString database_backup_path(const QString& filename, const QString& label);

class SqliteDataStore : public IDataStore
{
public:
    SqliteDataStore() = default;
    ~SqliteDataStore() override = default;

    bool save(Business* b, const QString& filename) override;
    bool load(Business* b, const QString& filename) override;
};

#endif // SQLITEDATASTORE_H
