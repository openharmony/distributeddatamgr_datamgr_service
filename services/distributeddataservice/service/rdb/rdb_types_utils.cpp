/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "rdb_types_utils.h"
namespace OHOS::ITypesUtil {
template<>
bool Marshalling(const NotifyConfig &input, MessageParcel &data)
{
    return ITypesUtil::Marshal(data, input.delay_, input.isFull_);
}

template<>
bool Marshalling(const Option &input, MessageParcel &data)
{
    return ITypesUtil::Marshal(
        data, input.mode, input.seqNum, input.isAsync, input.isAutoSync, input.isCompensation,
        input.isEnablePredicate, input.isDownloadOnly, input.enableErrorDetail, input.isFullSync);
}

template<>
bool Marshalling(const SubOption &input, MessageParcel &data)
{
    return ITypesUtil::Marshal(data, static_cast<int32_t>(input.mode));
}

template<>
bool Marshalling(const RdbChangedData &input, MessageParcel &data)
{
    return Marshal(data, input.tableData);
}

template<>
bool Marshalling(const Reference &input, MessageParcel &data)
{
    return Marshal(data, input.sourceTable, input.targetTable, input.refFields);
}

template<>
bool Marshalling(const StatReporter &input, MessageParcel &data)
{
    return Marshal(data, input.statType, input.bundleName, input.storeName, input.subType, input.costTime);
}

template<>
bool Unmarshalling(NotifyConfig &output, MessageParcel &data)
{
    return ITypesUtil::Unmarshal(data, output.delay_, output.isFull_);
}

template<>
bool Unmarshalling(Option &output, MessageParcel &data)
{
    return ITypesUtil::Unmarshal(
        data, output.mode, output.seqNum, output.isAsync, output.isAutoSync, output.isCompensation,
        output.isEnablePredicate, output.isDownloadOnly, output.enableErrorDetail, output.isFullSync);
}

template<>
bool Unmarshalling(SubOption &output, MessageParcel &data)
{
    int32_t mode = static_cast<int32_t>(output.mode);
    auto ret = ITypesUtil::Unmarshal(data, mode);
    output.mode = static_cast<decltype(output.mode)>(mode);
    return ret;
}

template<>
bool Unmarshalling(RdbChangedData &output, MessageParcel &data)
{
    return Unmarshal(data, output.tableData);
}

template<>
bool Unmarshalling(Reference &output, MessageParcel &data)
{
    return Unmarshal(data, output.sourceTable, output.targetTable, output.refFields);
}

template<>
bool Unmarshalling(StatReporter &output, MessageParcel &data)
{
    return Unmarshal(data, output.statType, output.bundleName, output.storeName, output.subType, output.costTime);
}

template<>
bool Marshalling(const RdbPredicates &input, MessageParcel &data)
{
    return Marshal(data, input.tables_, input.devices_, input.operations_);
}

template<>
bool Unmarshalling(RdbPredicates &output, MessageParcel &data)
{
    return Unmarshal(data, output.tables_, output.devices_, output.operations_);
}

template<>
bool Marshalling(const RdbOperation &input, MessageParcel &data)
{
    return Marshal(data, static_cast<int32_t>(input.operator_), input.field_, input.values_);
}

template<>
bool Unmarshalling(RdbOperation &output, MessageParcel &data)
{
    int32_t option;
    auto ret = Unmarshal(data, option, output.field_, output.values_);
    output.operator_ = static_cast<decltype(output.operator_)>(option);
    return ret;
}

template<>
bool Marshalling(const SyncerParam &input, MessageParcel &data)
{
    return Marshal(data, input.bundleName_, input.hapName_, input.storeName_, input.area_, input.level_,
        input.type_, input.isEncrypt_, input.password_, input.customDir_, input.isAutoClean_, input.isSearchable_,
        input.haMode_, input.infos_, input.tokenIds_, input.uids_, input.user_, input.permissionNames_,
        input.asyncDownloadAsset_, input.enableCloud_, input.subUser_, input.dfxInfo_, input.distributedTableMode_,
        input.isAutoCleanDevice_, input.customSwitch_, input.autoSyncSwitch_, input.assetConflictPolicy_,
        input.assetTempPath_, input.assetDownloadOnDemand_, input.dbPath_);
}

template<>
bool Unmarshalling(SyncerParam &output, MessageParcel &data)
{
    return Unmarshal(data, output.bundleName_, output.hapName_, output.storeName_, output.area_,
        output.level_, output.type_, output.isEncrypt_, output.password_, output.customDir_, output.isAutoClean_,
        output.isSearchable_, output.haMode_, output.infos_, output.tokenIds_, output.uids_, output.user_,
        output.permissionNames_, output.asyncDownloadAsset_, output.enableCloud_, output.subUser_, output.dfxInfo_,
        output.distributedTableMode_, output.isAutoCleanDevice_, output.customSwitch_, output.autoSyncSwitch_,
        output.assetConflictPolicy_, output.assetTempPath_, output.assetDownloadOnDemand_, output.dbPath_);
}

template<>
bool Marshalling(const Origin &input, MessageParcel &data)
{
    return Marshal(data, input.origin, input.dataType, input.id, input.store);
}

template<>
bool Unmarshalling(Origin &output, MessageParcel &data)
{
    return Unmarshal(data, output.origin, output.dataType, output.id, output.store);
}

template<>
bool Marshalling(const BigInt &input, MessageParcel &data)
{
    return Marshal(data, input.Sign(), input.Value());
}

template<>
bool Unmarshalling(BigInt &output, MessageParcel &data)
{
    int32_t sign = 0;
    std::vector<uint64_t> value;
    if (!Unmarshal(data, sign, value)) {
        return false;
    }
    output = BigInt(sign, std::move(value));
    return true;
}

template<>
bool Marshalling(const RdbProperties &input, MessageParcel &data)
{
    return Marshal(data, input.isTrackedDataChange, input.isP2pSyncDataChange, input.isKnowledgeDataChange);
}

template<>
bool Unmarshalling(RdbProperties &output, MessageParcel &data)
{
    return Unmarshal(data, output.isTrackedDataChange, output.isP2pSyncDataChange, output.isKnowledgeDataChange);
}

template<>
bool Marshalling(const RdbDebugInfo &input, MessageParcel &data)
{
    return Marshal(data, input.inode_, input.mode_, input.uid_, input.gid_);
}

template<>
bool Unmarshalling(RdbDebugInfo &output, MessageParcel &data)
{
    return Unmarshal(data, output.inode_, output.mode_, output.uid_, output.gid_);
}

template<>
bool Marshalling(const RdbDfxInfo &input, MessageParcel &data)
{
    return Marshal(data, input.lastOpenTime_, input.curUserId_);
}

template<>
bool Unmarshalling(RdbDfxInfo &output, MessageParcel &data)
{
    return Unmarshal(data, output.lastOpenTime_, output.curUserId_);
}

template<>
bool Marshalling(const ProgressDetail &input, MessageParcel &data)
{
    return Marshal(data, input.progress, input.code, input.details, input.message);
}

template<>
bool Unmarshalling(ProgressDetail &output, MessageParcel &data)
{
    return Unmarshal(data, output.progress, output.code, output.details, output.message);
}

template<>
bool Marshalling(const TableDetail &input, MessageParcel &data)
{
    return Marshal(data, input.upload, input.download);
}

template<>
bool Unmarshalling(TableDetail &output, MessageParcel &data)
{
    return Unmarshal(data, output.upload, output.download);
}

template<>
bool Marshalling(const Statistic &input, MessageParcel &data)
{
    return Marshal(data, input.total, input.success, input.failed, input.untreated);
}

template<>
bool Unmarshalling(Statistic &output, MessageParcel &data)
{
    return Unmarshal(data, output.total, output.success, output.failed, output.untreated);
}

template<>
bool Marshalling(const PrimaryKeys &input, MessageParcel &data)
{
    return Marshal(
        data, input[Observer::CHG_TYPE_INSERT], input[Observer::CHG_TYPE_UPDATE], input[Observer::CHG_TYPE_DELETE]);
}

template<>
bool Marshalling(const ValueObject &input, MessageParcel &data)
{
    return Marshal(data, input.value);
}

template<>
bool Unmarshalling(ValueObject &output, MessageParcel &data)
{
    return Unmarshal(data, output.value);
}

template<>
bool Marshalling(const ValuesBucket &input, MessageParcel &data)
{
    return Marshal(data, input.values_);
}

template<>
bool Unmarshalling(ValuesBucket &output, MessageParcel &data)
{
    return Unmarshal(data, output.values_);
}

template<>
bool Marshalling(const Asset &input, MessageParcel &data)
{
    return Marshal(data, input.version, input.name, input.size, input.modifyTime, input.uri);
}

template<>
bool Unmarshalling(Asset &output, MessageParcel &data)
{
    return Unmarshal(data, output.version, output.name, output.size, output.modifyTime, output.uri);
}
} // namespace OHOS::ITypesUtil