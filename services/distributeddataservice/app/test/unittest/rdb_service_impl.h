//
// Created by wuchunbo on 2022/1/21.
//

#ifndef RDB_SERVICE_IMPL_MOCK_H
#define RDB_SERVICE_IMPL_MOCK_H

#include "iremote_broker.h"
#include "iremote_stub.h"

namespace OHOS::DistributedRdb {
class IRdbService : public IRemoteBroker {
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"OHOS.DistributedRdb.IRdbService");
};

class RdbServiceImpl : public IRemoteStub<IRdbService> {
public:
    RdbServiceImpl() = default;
    ~RdbServiceImpl() override = default;
    
    int OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override
    {
        return 0;
    }
};
}
#endif
