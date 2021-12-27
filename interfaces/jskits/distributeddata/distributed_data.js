const distributedDataSo = requireInternal('data.distributedData');

export default {
    createKVManager: distributedDataSo.createKVManager,
    Query: distributedDataSo.Query,
    FieldNode: distributedDataSo.FieldNode,
    Schema: distributedDataSo.Schema,
    UserType: {
        SAME_USER_ID: 0,
    },
    Constants: {
        MAX_KEY_LENGTH: 1024,
        MAX_VALUE_LENGTH: 4194303,
        MAX_KEY_LENGTH_DEVICE: 896,
        MAX_STORE_ID_LENGTH: 128,
        MAX_QUERY_LENGTH: 512000,
        MAX_BATCH_SIZE: 128,
    },
    ValueType: {
        STRING: 0,
        INTEGER: 1,
        FLOAT: 2,
        BYTE_ARRAY: 3,
        BOOLEAN: 4,
        DOUBLE: 5,
    },
    SyncMode: {
        PULL_ONLY: 0,
        PUSH_ONLY: 1,
        PUSH_PULL: 2,
    },
    SubscribeType: {
        SUBSCRIBE_TYPE_LOCAL: 0,
        SUBSCRIBE_TYPE_REMOTE: 1,
        SUBSCRIBE_TYPE_ALL: 2,
    },
    KVStoreType: {
        DEVICE_COLLABORATION: 0,
        SINGLE_VERSION: 1,
        MULTI_VERSION: 2,
    },
    SecurityLevel: {
        NO_LEVEL: 0,
        S0: 1,
        S1: 2,
        S2: 3,
        S3: 5,
        S4: 6,
    },
}