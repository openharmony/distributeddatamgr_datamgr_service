
#include "grd_base/grd_db_api.h"
#include "grd_base/grd_error.h"
#include "document_store_manager.h"
#include "document_store.h"

using namespace DocumentDB;

typedef struct GRD_DB {
    DocumentStore *store_ = nullptr;
} GRD_DB;

int GRD_DBOpen(const char *dbPath, const char *configStr, unsigned int flags, GRD_DB **db)
{
    std::string path = dbPath;
    DocumentStore *store = nullptr;
    DocumentStoreManager::GetDocumentStore(path, store);
    *db = new (std::nothrow) GRD_DB();
    (*db)->store_ = store;
    return GRD_OK;
}

int GRD_DBClose(GRD_DB *db, unsigned int flags)
{
    return GRD_OK;
}
