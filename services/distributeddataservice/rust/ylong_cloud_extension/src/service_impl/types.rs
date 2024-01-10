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

use crate::ipc_conn;
use crate::service_impl::error::SyncError;
use std::collections::HashMap;

pub(crate) const GID_FIELD: &str = "#_gid";
pub(crate) const CREATE_FIELD: &str = "#_createTime";
pub(crate) const MODIFY_FIELD: &str = "#_modifyTime";

/// Struct of single value.
#[derive(Clone, Debug, PartialEq)]
pub enum Value {
    /// Empty Value
    Empty,

    /// Value containing an int
    Int(i64),

    /// Value containing an float
    Float(f64),

    /// Value containing a string
    String(String),

    /// Value containing a bool
    Bool(bool),

    /// Value containing a byte array
    Bytes(Vec<u8>),

    /// Value containing an asset
    Asset(ipc_conn::CloudAsset),

    /// Value containing an asset array
    Assets(Vec<ipc_conn::CloudAsset>),
}

impl Value {
    pub(crate) fn has_asset(&self) -> bool {
        matches!(self, Value::Asset(_) | Value::Assets(_))
    }
}

impl From<&Value> for ipc_conn::FieldRaw {
    fn from(value: &Value) -> Self {
        match value {
            Value::Empty => ipc_conn::FieldRaw::Null,
            Value::Int(i) => ipc_conn::FieldRaw::Number(*i),
            Value::Float(f) => ipc_conn::FieldRaw::Real(*f),
            Value::String(s) => ipc_conn::FieldRaw::Text(s.clone()),
            Value::Bool(b) => ipc_conn::FieldRaw::Bool(*b),
            Value::Bytes(b) => ipc_conn::FieldRaw::Blob(b.clone()),
            Value::Asset(a) => ipc_conn::FieldRaw::Asset(a.clone()),
            Value::Assets(a) => ipc_conn::FieldRaw::Assets(ipc_conn::CloudAssets(a.clone())),
        }
    }
}

impl From<&ipc_conn::FieldRaw> for Value {
    fn from(value: &ipc_conn::FieldRaw) -> Self {
        match value {
            ipc_conn::FieldRaw::Null => Value::Empty,
            ipc_conn::FieldRaw::Number(i) => Value::Int(*i),
            ipc_conn::FieldRaw::Real(f) => Value::Float(*f),
            ipc_conn::FieldRaw::Text(s) => Value::String(s.clone()),
            ipc_conn::FieldRaw::Bool(b) => Value::Bool(*b),
            ipc_conn::FieldRaw::Blob(b) => Value::Bytes(b.clone()),
            ipc_conn::FieldRaw::Asset(a) => Value::Asset(a.clone()),
            ipc_conn::FieldRaw::Assets(a) => Value::Assets(a.0.clone()),
        }
    }
}

impl From<ipc_conn::FieldRaw> for Value {
    fn from(value: ipc_conn::FieldRaw) -> Self {
        match value {
            ipc_conn::FieldRaw::Null => Value::Empty,
            ipc_conn::FieldRaw::Number(i) => Value::Int(i),
            ipc_conn::FieldRaw::Real(f) => Value::Float(f),
            ipc_conn::FieldRaw::Text(ref s) => Value::String(s.to_string()),
            ipc_conn::FieldRaw::Bool(b) => Value::Bool(b),
            ipc_conn::FieldRaw::Blob(ref b) => Value::Bytes(b.to_vec()),
            ipc_conn::FieldRaw::Asset(ref a) => Value::Asset(a.clone()),
            ipc_conn::FieldRaw::Assets(ref a) => Value::Assets(a.0.clone()),
        }
    }
}

/// Hashmap of Value name as keys, Value as values.
pub type RawValueBucket = HashMap<String, Value>;

impl From<ipc_conn::ValueBucket> for RawValueBucket {
    fn from(value: ipc_conn::ValueBucket) -> Self {
        let mut map = HashMap::new();
        for (key, field) in value.0 {
            map.insert(key, Value::from(field));
        }
        map
    }
}

/// Struct of Database stored in the cloud server. Database can be obtained from schema meta.
#[derive(Clone)]
pub struct Database {
    pub(crate) name: String,
    pub(crate) alias: String,
    pub(crate) tables: HashMap<String, Table>,
}

impl From<&ipc_conn::Database> for Database {
    fn from(value: &ipc_conn::Database) -> Self {
        let mut tables = HashMap::new();
        for t in &value.tables.0 {
            tables.insert(t.table_name.clone(), Table::from(t));
        }
        Database {
            name: value.name.clone(),
            alias: value.alias.clone(),
            tables,
        }
    }
}

impl TryFrom<&Database> for ipc_conn::Database {
    type Error = SyncError;
    fn try_from(value: &Database) -> Result<Self, SyncError> {
        let mut tables = vec![];
        for table in value.tables.values() {
            tables.push(table.try_into()?);
        }

        Ok(ipc_conn::Database {
            name: value.name.clone(),
            alias: value.alias.to_string(),
            tables: ipc_conn::SchemaOrderTables(tables),
        })
    }
}

impl Database {
    /// New a database instance
    pub fn new(name: String, alias: String, tables: HashMap<String, Table>) -> Database {
        Database {
            name,
            alias,
            tables,
        }
    }

    /// Get local name of Database instance.
    pub fn name(&self) -> &str {
        self.name.as_str()
    }

    /// Get alias, or say, cloud name, of Database instance.
    pub fn alias(&self) -> &str {
        self.alias.as_str()
    }

    /// Get tables of Database instance.
    pub fn tables(&self) -> &HashMap<String, Table> {
        &self.tables
    }
}

/// Struct of Table in Database.
#[derive(Clone)]
pub struct Table {
    pub(crate) name: String,
    pub(crate) alias: String,
    pub(crate) fields: Vec<Field>,
}

impl Table {
    /// New a table instance
    pub fn new(name: String, alias: String, fields: Vec<Field>) -> Table {
        Table {
            name,
            alias,
            fields,
        }
    }

    /// Get name of Table instance.
    pub fn name(&self) -> &str {
        self.name.as_str()
    }

    /// Get name of Table instance.
    pub fn alias(&self) -> &str {
        self.alias.as_str()
    }

    /// Get fields of Table instance.
    pub fn fields(&self) -> &[Field] {
        self.fields.as_slice()
    }
}

impl From<&ipc_conn::OrderTable> for Table {
    fn from(value: &ipc_conn::OrderTable) -> Self {
        let mut fields = vec![];
        for fd in &value.fields.0 {
            fields.push(Field::from(fd));
        }
        Table {
            name: value.table_name.clone(),
            alias: value.alias.clone(),
            fields,
        }
    }
}

impl TryFrom<&Table> for ipc_conn::OrderTable {
    type Error = SyncError;

    fn try_from(value: &Table) -> Result<Self, Self::Error> {
        let mut fields = vec![];
        for fd in &value.fields {
            let ipc_fd = ipc_conn::Field::try_from(fd)?;
            fields.push(ipc_fd);
        }
        Ok(ipc_conn::OrderTable {
            alias: value.alias.clone(),
            table_name: value.name.clone(),
            fields: ipc_conn::Fields(fields),
        })
    }
}

/// Struct of Field in Database Table.
#[derive(Clone)]
pub struct Field {
    pub(crate) col_name: String,
    pub(crate) alias: String,
    pub(crate) typ: u8,
    // By default is false
    pub(crate) primary: bool,
    // By default is true
    pub(crate) nullable: bool,
}

impl From<&ipc_conn::Field> for Field {
    fn from(value: &ipc_conn::Field) -> Self {
        Field {
            col_name: value.col_name.clone(),
            alias: value.alias.clone(),
            typ: u8::from(&value.typ),
            primary: value.primary,
            nullable: value.nullable,
        }
    }
}

impl TryFrom<&Field> for ipc_conn::Field {
    type Error = SyncError;

    fn try_from(value: &Field) -> Result<Self, SyncError> {
        Ok(ipc_conn::Field {
            col_name: value.col_name.clone(),
            alias: value.alias.clone(),
            typ: ipc_conn::FieldType::try_from(value.typ)?,
            primary: value.primary,
            nullable: value.nullable,
        })
    }
}

impl Field {
    /// New a field instance
    pub fn new(col_name: String, alias: String, typ: u8, primary: bool, nullable: bool) -> Field {
        Field {
            col_name,
            alias,
            typ,
            primary,
            nullable,
        }
    }

    /// Get column name of Field instance.
    pub fn col_name(&self) -> &str {
        self.col_name.as_str()
    }

    /// Get alias of Field instance.
    pub fn alias(&self) -> &str {
        self.alias.as_str()
    }

    /// Get type of Field instance.
    pub fn typ(&self) -> u8 {
        self.typ
    }

    /// Check whether this Field is primary.
    pub fn primary(&self) -> bool {
        self.primary
    }

    /// Check whether this Field is nullable.
    pub fn nullable(&self) -> bool {
        self.nullable
    }
}
