// Copyright (C) 2019-2024 Zilliz. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not
// use this file except in compliance with the License. You may obtain a copy of
// the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations under
// the License.

#pragma once

#include <any>
#include <variant>
#include <cstdint>
#include <optional>
#include <vector>
#include "common.h"
#include "common.pb.h"
#include "pb/plan.pb.h"
#include "pb/segcore.pb.h"
#include "pb/milvus.pb.h"
#include "pb/schema.pb.h"
#include "status.h"
#include "string_util.hpp"
#include <google/protobuf/repeated_field.h>

namespace milvus::local {

namespace schema_util {

std::any
GetField(const ::milvus::proto::schema::FieldData& field_data,
         uint32_t field_index);

bool
IsVectorField(::milvus::proto::schema::DataType dtype);

bool
IsSparseVectorType(::milvus::proto::schema::DataType dtype);

bool
FindDimFromFieldParams(const ::milvus::proto::schema::FieldSchema& field,
                       std::string* dim);
int64_t
GetDim(const ::milvus::proto::schema::FieldSchema& field);

bool
FindDimFromSchema(const ::milvus::proto::schema::CollectionSchema& schema,
                  std::string* dim);

std::optional<::milvus::proto::plan::VectorType>
DataTypeToVectorType(::milvus::proto::schema::DataType dtype);

Status
FindVectorField(const ::milvus::proto::schema::CollectionSchema& schema,
                const std::string& ann_field,
                const ::milvus::proto::schema::FieldSchema** field);

std::optional<int64_t>
GetPkId(const ::milvus::proto::schema::CollectionSchema& schema);

std::optional<std::string>
GetPkName(const ::milvus::proto::schema::CollectionSchema& schema);

bool
SliceFieldData(const ::milvus::proto::schema::FieldData& src_data,
               const std::vector<std::tuple<int64_t, int64_t>>& ranges,
               ::milvus::proto::schema::FieldData* dst);

bool
PickFieldDataByIndex(const ::milvus::proto::schema::FieldData& src_data,
                     const std::vector<int64_t>& indexes,
                     ::milvus::proto::schema::FieldData* dst);

bool
FillEmptyField(const ::milvus::proto::schema::FieldSchema& field_schema,
               ::milvus::proto::schema::FieldData* field_data);

std::string
MergeIndexs(std::vector<std::string>& indexs);

bool
SchemaEquals(const std::string& schema_str_l, const std::string& schema_str_r);

// Support wildcard in output fields:
//
//"*" - all fields
//
// For example, A and B are scalar fields, C and D are vector fields, duplicated fields will automatically be removed.
//
//output_fields=["*"]  ==> [A,B,C,D]
//output_fields=["*",A]  ==> [A,B,C,D]
//output_fields=["*",C]    ==> [A,B,C,D]
bool
TranslateOutputFields(
    const ::google::protobuf::RepeatedPtrField<std::string>& raw_fields,
    const ::milvus::proto::schema::CollectionSchema& schema,
    bool add_primary,
    std::vector<std::string>* result_outputs,
    std::vector<std::string>* user_output_fields);

bool
ReduceFieldByIDs(const ::milvus::proto::schema::IDs& ids,
                 const ::milvus::proto::schema::FieldData& src,
                 ::milvus::proto::schema::FieldData* dst,
                 int64_t* real_size);

// PositivelyRelated return if metricType are "ip" or "IP"
inline bool
PositivelyRelated(const std::string& metrics_type) {
    auto upper_str = string_util::ToUpper(metrics_type);
    return upper_str == KMetricsIPName || upper_str == kMetricsCosineName ||
           upper_str == kMetricsBM25Name;
}

std::string
GetIndexMetricType(const milvus::proto::segcore::FieldIndexMeta& field_index);

Status
ParseExpr(const std::string& expr_str,
          ::milvus::proto::schema::CollectionSchema schema,
          ::milvus::proto::plan::Expr* expr_out);

bool
CheckParamsEqual(const ::google::protobuf::RepeatedPtrField<
                     ::milvus::proto::common::KeyValuePair>& left,
                 const ::google::protobuf::RepeatedPtrField<
                     ::milvus::proto::common::KeyValuePair>& right);

bool
CheckValueFieldEqual(const ::milvus::proto::schema::ValueField& left,
                     const ::milvus::proto::schema::ValueField& right);

inline int
IDsSize(const ::milvus::proto::schema::IDs& ids) {
    if (ids.has_int_id()) {
        return ids.int_id().data_size();
    } else {
        return ids.str_id().data_size();
    }
}

void
FillInFieldInfo(
    const std::vector<std::string>& output_fields,
    const ::milvus::proto::schema::CollectionSchema& schema,
    std::variant<::milvus::proto::schema::SearchResultData*,
                 ::milvus::proto::milvus::QueryResults*> result_var);

bool
GetOutputFieldsIds(const std::vector<std::string>& output_fields,
                   const ::milvus::proto::schema::CollectionSchema& schema,
                   std::vector<int64_t>* ids);

inline void
SparseVectorToByte(const SparseVector& vec, std::string* buf) {
    for (const auto& it : vec) {
        uint32_t k = it.first;
        float v = it.second;
        buf->append(reinterpret_cast<const char*>(&k), sizeof(k));
        buf->append(reinterpret_cast<const char*>(&v), sizeof(v));
    }
}

inline bool
HasFunction(const ::milvus::proto::schema::CollectionSchema& schema) {
    return !schema.functions().empty();
}

inline bool
HasFunction(const ::milvus::proto::schema::CollectionSchema& schema,
            const std::string& ann_field) {
    for (const auto& f : schema.functions()) {
        if (f.output_field_names(0) == ann_field) {
            return true;
        }
    }
    return false;
}

inline std::vector<std::string>
GetBM25SparseField(const ::milvus::proto::schema::CollectionSchema& schema) {
    std::vector<std::string> fields;
    for (const auto& f : schema.functions()) {
        if (f.type() == milvus::proto::schema::FunctionType::BM25) {
            fields.push_back(f.output_field_names(0));
        }
    }
    return fields;
}

// inline bool
// HasBM25Function(const ::milvus::proto::schema::CollectionSchema& schema) {
//     return true;
// }

}  // namespace schema_util

}  // namespace milvus::local
