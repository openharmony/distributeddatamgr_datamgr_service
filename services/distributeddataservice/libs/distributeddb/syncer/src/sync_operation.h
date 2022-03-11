/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#ifndef SYNC_OPERATION_H
#define SYNC_OPERATION_H

#include <functional>
#include <string>
#include <vector>
#include <map>
#include <mutex>

#include "ikvdb_sync_interface.h"
#include "notification_chain.h"
#include "query_sync_object.h"
#include "ref_object.h"
#include "runtime_context.h"
#include "semaphore_utils.h"
#include "sync_types.h"

namespace DistributedDB {
class SyncOperation : public RefObject {
public:
    enum Status {
        OP_WAITING = 0,
        OP_SYNCING,
        OP_SEND_FINISHED,
        OP_RECV_FINISHED,
        OP_FINISHED_ALL, // status >= OP_FINISHED_ALL is final status.
        OP_FAILED,
        OP_TIMEOUT,
        OP_PERMISSION_CHECK_FAILED,
        OP_COMM_ABNORMAL,
        OP_SECURITY_OPTION_CHECK_FAILURE, // remote device's SecurityOption not equal to local
        OP_EKEYREVOKED_FAILURE, // EKEYREVOKED error
        OP_BUSY_FAILURE,
        OP_SCHEMA_INCOMPATIBLE,
        OP_QUERY_FORMAT_FAILURE,
        OP_QUERY_FIELD_FAILURE,
        OP_NOT_SUPPORT,
        OP_INTERCEPT_DATA_FAIL,
        OP_MAX_LIMITS,
        OP_SCHEMA_CHANGED,
        OP_INVALID_ARGS,
        OP_USER_CHANGED
    };

    using UserCallback = std::function<void(std::map<std::string, int>)>;
    using OnSyncFinished = std::function<void(int)>;
    using OnSyncFinalize = std::function<void(void)>;

    SyncOperation(uint32_t syncId, const std::vector<std::string> &devices, int mode,
        const UserCallback &userCallback, bool isBlockSync);

    DISABLE_COPY_ASSIGN_MOVE(SyncOperation);

    // Init the status for callback
    int Initialize();

    // Set the OnSyncFinalize callback
    void SetOnSyncFinalize(const OnSyncFinalize &callback);

    // Set the OnSyncFinished callback, it will be called either success or failed.
    void SetOnSyncFinished(const OnSyncFinished &callback);

    // Set the sync status, running or finished
    void SetStatus(const std::string &deviceId, int status);

    // Set the unfinished devices sync status, running or finished
    void SetUnfinishedDevStatus(int status);

    // Set the identifier, used in SyncOperation::Finished
    void SetIdentifier(const std::vector<uint8_t> &identifier);

    // Get the sync status, running or finished
    int GetStatus(const std::string &deviceId) const;

    // Get the sync id.
    uint32_t GetSyncId() const;

    // Get the sync mode
    int GetMode() const;

    // Used to call the onFinished and caller's on complete
    void Finished();

    // Get the deviceId of this sync status
    const std::vector<std::string> &GetDevices() const;

    // Wait if it's a block sync
    void WaitIfNeed();

    // Notify if it's a block sync
    void NotifyIfNeed();

    // Return if this sync is auto sync
    bool IsAutoSync() const;

    // Return if this sync is block sync
    bool IsBlockSync() const;

    // Return if this sync is AUTO_SUBSCRIBE_QUERY
    bool IsAutoControlCmd() const;

    // Check if All devices sync finished.
    bool CheckIsAllFinished() const;

    // For query sync
    void SetQuery(const QuerySyncObject &query);
    QuerySyncObject GetQuery() const;
    bool IsQuerySync() const;
    std::string GetQueryId() const;
    static SyncType GetSyncType(int mode);
    static int TransferSyncMode(int mode);

    static const std::map<int, DBStatus> &DBStatusTransMap();

protected:
    virtual ~SyncOperation();

private:
    DECLARE_OBJECT_TAG(SyncOperation);

    // called by destruction
    void Finalize();

    // Transfer sync mode from interface to inner
    void TransferQuerySyncMode();

    // The device list
    const std::vector<std::string> devices_;

    // The Syncid
    uint32_t syncId_;

    // The sync mode_ see SyncMode
    int mode_;

    // The callback caller registered
    UserCallback userCallback_;

    // The callback caller registered, when sync timeout, call
    OnSyncFinished onFinished_;

    // The callback caller registered, will be called when destruction.
    OnSyncFinalize onFinalize_;

    // The device id we sync with
    std::map<std::string, int> statuses_;

    // Is this operation is a block sync
    bool isBlockSync_;

    // Is this operation is a auto sync
    bool isAutoSync_;

    // Is this operation has finished
    bool isFinished_;

    // Used for block sync
    std::unique_ptr<SemaphoreUtils> semaphore_;

    QuerySyncObject query_;
    bool isQuerySync_;

    bool isAutoSubscribe_;

    // record identifier used to call ScheduleQueuedTask in SyncOperation::Finished
    std::string identifier_;
};
} // namespace DistributedDB

#endif  // SYNC_OPERATION_H
