//===----------------------------------------------------------------------===//
//
//                         PelotonDB
//
// materialization_node.h
//
// Identification: src/backend/planner/materialization_node.h
//
// Copyright (c) 2015, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//


#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "backend/common/types.h"
#include "backend/planner/abstract_plan_node.h"

namespace peloton {

namespace catalog {
class Schema;
}

namespace planner {

class MaterializationNode : public AbstractPlanNode {
public:
  MaterializationNode(const MaterializationNode &) = delete;
  MaterializationNode &operator=(const MaterializationNode &) = delete;
  MaterializationNode(MaterializationNode &&) = delete;
  MaterializationNode &operator=(MaterializationNode &&) = delete;

  MaterializationNode(const std::unordered_map<oid_t, oid_t> &old_to_new_cols,
                      catalog::Schema *schema, bool physify_flag)
      : old_to_new_cols_(old_to_new_cols), schema_(schema),
        physify_flag_(physify_flag) {}

  MaterializationNode(bool physify_flag)
      : schema_(nullptr), physify_flag_(physify_flag) {}

  ~MaterializationNode() {
    // Clean up schema
    if (schema_)
      delete schema_;
  }

  inline const std::unordered_map<oid_t, oid_t> &old_to_new_cols() const {
    return old_to_new_cols_;
  }

  inline const catalog::Schema *GetSchema() const { return schema_; }

  inline bool GetPhysifyFlag() const { return physify_flag_; }

  inline PlanNodeType GetPlanNodeType() const {
    return PLAN_NODE_TYPE_MATERIALIZE;
  }

  inline std::string GetInfo() const { return "Materialize"; }

private:
  /**
   * @brief Mapping of old column ids to new column ids after materialization.
   */
  std::unordered_map<oid_t, oid_t> old_to_new_cols_;

  /** @brief Schema of newly materialized tile. */
  catalog::Schema *schema_;

  /**
   * @brief whether to create a physical tile or just pass thru underlying
   * logical tile
   */
  bool physify_flag_;
};

} // namespace planner
} // namespace peloton
