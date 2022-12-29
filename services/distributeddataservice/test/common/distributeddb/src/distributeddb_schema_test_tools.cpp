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
#include "distributeddb_schema_test_tools.h"
#include <fstream>
#include "distributeddb_nb_test_tools.h"

using namespace std;
using namespace DistributedDB;
using namespace DistributedDBDataGenerator;

Entry DistributedDBSchemaTestTools::GenerateFixedLenJsonSchemaRecord(const unsigned long serialNo,
    const EntrySize &entrySize, const uint8_t keyFilledChr, const uint8_t valueFilledChr)
{
    Entry entry;
    std::string serialNoStr = std::to_string(serialNo);
    entry.key.assign(entrySize.keySize - serialNoStr.length(), keyFilledChr);
    for (unsigned long index = 0; index < serialNoStr.size(); ++index) {
        entry.key.push_back(serialNoStr[index]);
    }

    string val = string("{") + "\"field" + std::to_string(FIRST_FIELD) + "\":" + std::to_string(serialNo) + ",";
    for (unsigned int fieldIndex = SECOND_FIELD; fieldIndex <= THIRTIETH_FIELD; fieldIndex++) {
        if (fieldIndex == SIXTH_FIELD) {
            val += "\"field" + std::to_string(fieldIndex) + "\":" + to_string(static_cast<double>(serialNo)) + ",";
        } else {
            val += "\"field" + std::to_string(fieldIndex) + "\":" +
                "\"SchemaPerfTest" + std::to_string(fieldIndex) + "_" + std::to_string(serialNo) + "\",";
        }
    }
    val.back() = '}';
    size_t found = val.rfind("PerfTest");
    if (found != string::npos) {
        string insert = "PerfTest";
        insert += string(entrySize.valSize - val.length() - 1, valueFilledChr); // consider skipsize 1 byte 'a' below
        val.replace(found, strlen("PerfTest"), insert);
    }
    string scmVal = "a" + val;
    Value schemaVal(scmVal.begin(), scmVal.end());
    entry.value = schemaVal;
    return entry;
}

vector<Entry> DistributedDBSchemaTestTools::GenerateFixedJsonSchemaRecords(const int recordNum,
    const EntrySize &entrySize, const uint8_t keyFilledChr, const uint8_t valueFilledChr, vector<Key> &allKeys)
{
    vector<Entry> entries;
    for (int serialNo = TEST_START_CNT; serialNo <= recordNum; ++serialNo) {
        Entry entry = GenerateFixedLenJsonSchemaRecord(serialNo, entrySize, keyFilledChr, valueFilledChr);
        allKeys.push_back(entry.key);
        entries.push_back(entry);
    }
    return entries;
}

Entry DistributedDBSchemaTestTools::GenerateFixedLenSchemaPerfRecord(
    const uint64_t presetRecordsCnt, const uint64_t serialNo, const RecordInfo &recordInfo,
    const string &valueSkipString)
{
    Entry entry;
    std::string serialNoStr = std::to_string(serialNo);
    entry.key.assign(recordInfo.keyLength - serialNoStr.length(), recordInfo.keyFilledChr);
    for (unsigned long index = 0; index < serialNoStr.size(); ++index) {
        entry.key.push_back(serialNoStr[index]);
    }

    string val = string("{") + "\"field" + std::to_string(FIRST_FIELD) + "\":" + std::to_string(serialNo) + ",";
    unsigned int fieldIndex;
    for (fieldIndex = SECOND_FIELD; fieldIndex <= THIRD_FIELD; fieldIndex++) {
        val += "\"field" + std::to_string(fieldIndex) + "\":" +
            "\"SchemaPerfTest" + std::to_string(fieldIndex) + "_" + serialNoStr + "\",";
    }
    if (serialNo <= static_cast<uint64_t>(presetRecordsCnt) / 2) { // 2 is an half of records.
        for (fieldIndex = FOURTH_FIELD; fieldIndex <= FIFTH_FIELD; fieldIndex++) {
            val += "\"field" + std::to_string(fieldIndex) + "\":" +
                "\"SchemaPerfTest" + std::to_string(fieldIndex) + "_" + serialNoStr + "\",";
        }
    }
    for (; fieldIndex <= THIRTIETH_FIELD; fieldIndex++) {
        if (fieldIndex == SIXTH_FIELD) {
            val += "\"field" + std::to_string(fieldIndex) + "\":" + to_string(static_cast<double>(serialNo)) + ",";
        } else {
            val += "\"field" + std::to_string(fieldIndex) + "\":" +
                "\"SchemaPerfTest" + std::to_string(fieldIndex) + "\",";
        }
    }
    val.back() = '}';
    size_t found = val.rfind("PerfTest");
    if (found != string::npos) {
        if (recordInfo.valueLength > val.length()) {
            string insert = "PerfTest";
            insert += string(recordInfo.valueLength - val.length(), recordInfo.valueFilledChr);
            val.replace(found, strlen("PerfTest"), insert);
        } else {
            MST_LOG("[GenerateFixedLenSchemaPerfRecord] recordInfo.valueLength(%u) is too short, " \
                "it should be more than %zu.", recordInfo.valueLength, val.length());
            return entry;
        }
    }
    string scmVal = valueSkipString + val;
    Value schemaVal(scmVal.begin(), scmVal.end());
    entry.value = schemaVal;
    return entry;
}

bool DistributedDBSchemaTestTools::SchemaIndexQuery(const DBParameters &parameters,
    const string &dbPath, const string &schemaIndex, const Option &option)
{
    int count = 0;
    string identifier = parameters.userId + "-" + parameters.appId + "-" + parameters.storeId;
    string hashIdentifierRes = TransferStringToHashHexString(identifier);
    const string dbName = dbPath + hashIdentifierRes + DATABASE_INFOR_FILE;
    EncrypteAttribute attribute = {option.isEncryptedDb, option.passwd};

    const string SCHEMA_INDEX_QUERY_SQL = "select count(*) from sqlite_master where name= \'" +
        schemaIndex + "\' and type = \'index\'";

    DistributedTestTools::QuerySpecifiedData(dbName, SCHEMA_INDEX_QUERY_SQL, attribute, count);
    return (count == 1);
}

vector<string> DistributedDBSchemaTestTools::GenerateCombinationSchemaValue(const vector<vector<string>> &fieldValue)
{
    vector<string> schemasValue;
    int valueNum = fieldValue.size();
    for (int index = 0; index < valueNum; index++) {
        string valueStr;
        string field1Val = fieldValue[index][INDEX_ZEROTH];
        if (field1Val != "null") {
            valueStr = valueStr + "{\"field1\":\"" + field1Val + "\"";
        } else {
            valueStr = valueStr + "{\"field1\":" + field1Val;
        }

        valueStr = valueStr + ",\"field2\":{\"field3\":" + fieldValue[index][INDEX_FIRST] + ",\"field4\":{\"field5\":" +
        fieldValue[index][INDEX_SECOND] + ",\"field6\":{\"field7\":" + fieldValue[index][INDEX_THIRD] + ",\"field8\":" +
        fieldValue[index][INDEX_FORTH] + "}}}}";
        schemasValue.push_back(valueStr);
    }
    return schemasValue;
}
void DistributedDBSchemaTestTools::GenerateCombineSchemaEntries(std::vector<DistributedDB::Entry> &entries,
    const std::vector<vector<std::string>> &fieldValues, const std::vector<uint8_t> &keyPrefix, int startPoint)
{
    entries.clear();
    vector<Key> keys;
    int recordNo = fieldValues.size();
    for (int cnt = startPoint; cnt < (startPoint + recordNo); cnt++) {
        Key key = keyPrefix;
        std::string keyNo = std::to_string(cnt);
        key.insert(key.end(), keyNo.begin(), keyNo.end());
        keys.push_back(key);
    }
    vector<string> valueStr = GenerateCombinationSchemaValue(fieldValues);
    for (int index = 0; index < recordNo; index++) {
        Entry entry;
        Value value(valueStr[index].begin(), valueStr[index].end());
        entry.key = keys[index];
        entry.value = value;
        entries.push_back(entry);
    }
}

bool DistributedDBSchemaTestTools::CombinationCheckQueryResult(KvStoreNbDelegate &delegate, const Query &query,
    vector<Entry> &expectEntry, const DBStatus status, bool canGetCount)
{
    vector<Entry> entries;
    bool result = false;
    result = (delegate.GetEntries(query, entries) == status);
    if (!expectEntry.empty()) {
        if (entries.size() != expectEntry.size()) {
            MST_LOG("The entries from query is %zd, The expectEntry is %zd, they are not equal",
                entries.size(), expectEntry.size());
            return false;
        }
        for (vector<Entry>::size_type index = 0; index < entries.size(); index++) {
            if (entries[index].key != expectEntry[index].key || entries[index].value != expectEntry[index].value) {
                string keyGot(entries[index].key.begin(), entries[index].key.end());
                string keyExpect(expectEntry[index].key.begin(), expectEntry[index].key.end());
                MST_LOG("entry key compare failed, expectKey:%s, gotKey:%s, line:%d", keyExpect.c_str(), keyGot.c_str(),
                    __LINE__);
                return false;
            }
        }
    }
    KvStoreResultSet *resultSet = nullptr;
    if (status != NOT_FOUND) {
        result = (delegate.GetEntries(query, resultSet) == status) && result;
    } else {
        result = (delegate.GetEntries(query, resultSet) == OK) && result;
        if (resultSet != nullptr) {
            Entry entry;
            result = (resultSet->GetEntry(entry) == status) && result;
        }
    }
    int expectCnt = expectEntry.size();
    if (resultSet != nullptr) {
        result = (resultSet->GetCount() == expectCnt) && result;
        result = (delegate.CloseResultSet(resultSet) == DBStatus::OK) && result;
    }
    if (canGetCount) {
        int cnt = 0;
        result = (delegate.GetCount(query, cnt) == status) && result;
        result = (cnt == expectCnt) && result;
    }
    return result;
}

Value DistributedDBSchemaTestTools::GenSchemaValue(Value notSchemaValue)
{
    // emit the first 'v' character
    string notSchemaVal(notSchemaValue.begin() + INDEX_FIRST, notSchemaValue.end());
    string valueRes = "{" + VALUE_MATCH_1 + "," + VALUE_MATCH_2 + ",\"field17\":" + notSchemaVal + "}";
    Value valueSchema(valueRes.begin(), valueRes.end());
    return valueSchema;
}

bool DistributedDBSchemaTestTools::PreInsertRecords(KvStoreNbDelegate *&delegate, vector<Key> &keys,
    const std::vector<std::string> &values, int beginNumber)
{
    bool result = true;
    for (vector<string>::size_type index = 0; index < values.size(); index++) {
        keys.push_back({'k', static_cast<uint8_t>(index + 49 + beginNumber)}); // 49 is the ASCII number of 'a'
    }
    for (vector<string>::size_type index = 0; index < values.size(); index++) {
        Value value(values[index].begin(), values[index].end());
        result = (DistributedDBNbTestTools::Put(*delegate, keys[index], value) == OK);
    }
    return result;
}

// insert query condition between brackets.
static void SpliceQueryMethod(int flag, Query &query)
{
    switch (flag) {
        case 0: // query condition 0
            query.LessThan("$.field2.field4.field5", "100").And().NotEqualTo("$.field2.field4.field6.field7", "-100");
            break;
        case 1: // query condition 1
            query.EqualTo("$.field2.field3", "true").Or().NotLike("$.field1", "%c");
            break;
        case 2: // query condition 2
            query.Like("$.field1", "ab%");
            break;
        case 3: // query condition 3
            query.GreaterThanOrEqualTo("$.field2.field4.field6.field8", "0");
            break;
        default: // other query condition
            break;
    }
}
// Generate rand query by the number of brackets.
void DistributedDBSchemaTestTools::GenerateRandQuery(Query &query, int beginNum, int endNum)
{
    int left = 0;
    int right = 0;
    for (int cnt = 0; cnt < beginNum + endNum; cnt++) {
        int bracketTest = GetRandInt(0, 1);
        if (bracketTest == 0 && left < beginNum) {
            left++;
            if (cnt != 0) {
                query.Or();
            }
            query.BeginGroup();
        } else if (bracketTest == 1 && right < endNum) {
            right++;
            query.EndGroup();
        } else {
            cnt--;
            continue;
        }
        int flag = GetRandInt(0, 4); // add query condition 0-4 to brackets.
        if ((bracketTest == 0 && cnt < beginNum + endNum - 1) || (bracketTest == 1 && cnt == 0)) {
            SpliceQueryMethod(flag, query);
        } else if (bracketTest == 1 && cnt < beginNum + endNum - 1) {
            query.And();
            SpliceQueryMethod(flag, query);
        }
    }
}

bool DistributedDBSchemaTestTools::TransformToSchemaEntry(std::vector<DistributedDB::Entry> &entries,
    const std::vector<DistributedDB::Key> &keys, const std::vector<std::string> &schemasValue)
{
    if (keys.size() != schemasValue.size()) {
        MST_LOG("The number of keys is not equal to the value strings' number!");
        return false;
    }
    for (vector<string>::size_type index = 0; index < schemasValue.size(); index++) {
        Value value(schemasValue[index].begin(), schemasValue[index].end());
        entries.push_back({keys[index], value});
    }
    return true;
}

void DistributedDBSchemaTestTools::GenerateSpecificSchemaEntries(const int startPoint, const int recordNumber,
    std::vector<DistributedDB::Entry> &entries, const std::vector<std::vector<std::string>> &values)
{
    entries.clear();
    int switchFactor;
    vector<Key> keys;
    vector<string> schemasValue;
    std::string indexNumStr, field1, field3, field5, field7, field8, value, switchFactorStr;
    for (int indexNum = startPoint; indexNum < startPoint + recordNumber; indexNum++) {
        indexNumStr = std::to_string(indexNum);
        field1 = "\"field1\":\"SubscribeRemoteQueryTest" + indexNumStr + "\"";
        field3 = "\"field3\":false";
        field5 = "\"field5\":" + indexNumStr;
        field7 = "\"field7\":" + indexNumStr;
        field8 = "\"field8\":" + indexNumStr + ".1234";
        for (const auto &iter : values) {
            switchFactorStr = iter[INDEX_ZEROTH];
            switchFactor = switchFactorStr.back() - '0';
            switch (switchFactor) {
                case 1: // field 1 has assigned value
                    field1 = "\"field1\":\"" + iter[INDEX_FIRST] + "\"";
                    break;
                case 3: // field 3 has assigned value
                    field3 = "\"field3\":" + iter[INDEX_FIRST];
                    break;
                case 5: // field 5 has assigned value
                    field5 = "\"field5\":" + iter[INDEX_FIRST];
                    break;
                case 7: // field 7 has assigned value
                    field7 = "\"field7\":" + iter[INDEX_FIRST];
                    break;
                case 8: // field 8 has assigned value
                    field8 = "\"field8\":" + iter[INDEX_FIRST];
                    break;
                default:
                    MST_LOG("invalid field !");
                    break;
            }
        }
        value = "{" + field1 + ",\"field2\":{" + field3 + ",\"field4\":{" + field5 + ",\"field6\":{"
            + field7 + "," + field8 + "}}}}";
        indexNumStr = "k" + indexNumStr;
        Key key(indexNumStr.begin(), indexNumStr.end());
        keys.push_back(key);
        schemasValue.push_back(value);
    }
    DistributedDBSchemaTestTools::TransformToSchemaEntry(entries, keys, schemasValue);
}
