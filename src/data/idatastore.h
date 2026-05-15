#ifndef IDATASTORE_H
#define IDATASTORE_H

#include <QString>

class Business;

class IDataStore
{
public:
    virtual ~IDataStore() = default;

    // Save the given Business to `filename`. Returns true on success.
    virtual bool save(Business* b, const QString& filename) = 0;

    // Load the given Business from `filename`. Returns true on success.
    virtual bool load(Business* b, const QString& filename) = 0;
};

#endif // IDATASTORE_H
