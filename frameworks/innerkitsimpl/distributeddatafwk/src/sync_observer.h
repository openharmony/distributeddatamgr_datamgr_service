//
// Created by Sincer on 2022/2/12.
//

#ifndef DISTRIBUTEDDATAMGR_DATAMGR_SYNC_OBSERVER_H
#define DISTRIBUTEDDATAMGR_DATAMGR_SYNC_OBSERVER_H

#include <vector>
#include <memory>
#include "kvstore_sync_callback.h"

namespace OHOS::DistributedKv {
class SyncObserver : public KvStoreSyncCallback {
public:
    SyncObserver(const std::vector<std::shared_ptr<KvStoreSyncCallback>> &callbacks);

    SyncObserver() = default;

    virtual ~SyncObserver() = default;

    bool Add(const std::shared_ptr<KvStoreSyncCallback> callback);

    bool Clean();

    void SyncCompleted(const std::map<std::string, DistributedKv::Status> &results) override;

private:
    std::vector<std::shared_ptr<KvStoreSyncCallback>> callbacks_;
};
}
#endif //DISTRIBUTEDDATAMGR_DATAMGR_SYNC_OBSERVER_H
