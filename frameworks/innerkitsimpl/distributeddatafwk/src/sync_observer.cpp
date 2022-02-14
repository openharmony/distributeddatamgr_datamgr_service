//
// Created by Sincer on 2022/2/12.
//

#include "sync_observer.h"

namespace OHOS::DistributedKv {
SyncObserver::SyncObserver(const std::vector<std::shared_ptr<KvStoreSyncCallback>> &callbacks)
    :callbacks_(callbacks)
{};

bool SyncObserver::Add(const std::shared_ptr<KvStoreSyncCallback> callback) {
    callbacks_.push_back(callback);
    return true;
}

bool SyncObserver::Clean() {
    callbacks_.clear();
    return true;
}

void SyncObserver::SyncCompleted(const std::map<std::string, DistributedKv::Status> &results)
{
    for (auto &callback : callbacks_) {
        callback->SyncCompleted(results);
    }
}
}
