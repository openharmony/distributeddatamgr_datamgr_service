/*
* Copyright (c) 2023 Huawei Device Co., Ltd.
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include "grd_document/grd_document_api.h"

#include "grd_base/grd_error.h"
#include "grd_resultset_inner.h"
#include "grd_type_inner.h"
#include "log_print.h"
using namespace DocumentDB;

int32_t GRD_CreateCollection(GRD_DB *db, const char *collectionName, const char *optionStr, uint32_t flags)
{
    if (db == nullptr || db->store_ == nullptr) {
        return GRD_INVALID_ARGS;
    }

    std::string name = (collectionName == nullptr ? "" : collectionName);
    std::string option = (optionStr == nullptr ? "" : optionStr);
    int ret = db->store_->CreateCollection(name, option, flags);
    return TransferDocErr(ret);
}

int32_t GRD_DropCollection(GRD_DB *db, const char *collectionName, uint32_t flags)
{
    if (db == nullptr || db->store_ == nullptr) {
        return GRD_INVALID_ARGS;
    }

    std::string name = (collectionName == nullptr ? "" : collectionName);
    int ret = db->store_->DropCollection(name, flags);
    return TransferDocErr(ret);
}

int32_t GRD_UpdateDoc(GRD_DB *db, const char *collectionName, const char *filter, const char *update, uint32_t flags)
{
    if (db == nullptr || db->store_ == nullptr || collectionName == nullptr || filter == nullptr || update == nullptr) {
        return GRD_INVALID_ARGS;
    }
    int ret = db->store_->UpdateDocument(collectionName, filter, update, flags);
    if (ret == 1) {
        return 1; // The amount of text updated
    } else if (ret == 0) {
        return 0;
    }
    return TransferDocErr(ret);
}

int32_t GRD_UpsertDoc(GRD_DB *db, const char *collectionName, const char *filter, const char *document, uint32_t flags)
{
    if (db == nullptr || db->store_ == nullptr || collectionName == nullptr || filter == nullptr ||
        document == nullptr) {
        return GRD_INVALID_ARGS;
    }
    int ret = db->store_->UpsertDocument(collectionName, filter, document, flags);
    if (ret == 1) {
        return 1; // The amount of text updated
    } else if (ret == 0) {
        return 0;
    }
    return TransferDocErr(ret);
}

int32_t GRD_InsertDoc(GRD_DB *db, const char *collectionName, const char *document, uint32_t flags)
{
    if (db == nullptr || db->store_ == nullptr || collectionName == nullptr || document == nullptr) {
        return GRD_INVALID_ARGS;
    }
    int ret = db->store_->InsertDocument(collectionName, document, flags);
    return TransferDocErr(ret);
}

int32_t GRD_DeleteDoc(GRD_DB *db, const char *collectionName, const char *filter, uint32_t flags)
{
    if (db == nullptr || db->store_ == nullptr || filter == nullptr || collectionName == nullptr) {
        return GRD_INVALID_ARGS;
    }
    int ret = db->store_->DeleteDocument(collectionName, filter, flags);
    int errCode = TransferDocErr(ret);
    int deleteCount = 0;
    switch (errCode) {
        case GRD_OK:
            deleteCount = 1;
            return deleteCount;
            break;
        case GRD_NO_DATA:
            deleteCount = 0;
            return deleteCount;
            break;
        default:
            break;
    }
    return errCode;
}

int32_t GRD_FindDoc(GRD_DB *db, const char *collectionName, Query query, uint32_t flags, GRD_ResultSet **resultSet)
{
    if (db == nullptr || db->store_ == nullptr || collectionName == nullptr || resultSet == nullptr ||
        query.filter == nullptr || query.projection == nullptr) {
        return GRD_INVALID_ARGS;
    }
    if (db->store_->IsCollectionOpening(collectionName)) {
        return GRD_RESOURCE_BUSY;
    }
    GRD_ResultSet *grdResultSet = new (std::nothrow) GRD_ResultSet();
    if (grdResultSet == nullptr) {
        GLOGE("Memory allocation failed!");
        return -E_FAILED_MEMORY_ALLOCATE;
    }
    int ret = db->store_->FindDocument(collectionName, query.filter, query.projection, flags, grdResultSet);
    if (ret != E_OK) {
        delete grdResultSet;
        *resultSet = nullptr;
        return TransferDocErr(ret);
    }
    *resultSet = grdResultSet;
    return TransferDocErr(ret);
}
