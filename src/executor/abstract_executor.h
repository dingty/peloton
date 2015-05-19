/**
 * @brief Header file for abstract executor.
 *
 * Copyright(c) 2015, CMU
 */

#pragma once

#include <cassert>
#include <memory>
#include <vector>

#include "common/transaction.h"
#include "executor/logical_tile.h"
#include "planner/abstract_plan_node.h"

namespace nstore {
namespace executor {

class AbstractExecutor {

 public:
  AbstractExecutor(const AbstractExecutor &) = delete;
  AbstractExecutor& operator=(const AbstractExecutor &) = delete;
  AbstractExecutor(AbstractExecutor &&) = delete;
  AbstractExecutor& operator=(AbstractExecutor &&) = delete;

  virtual ~AbstractExecutor(){}

  bool Init();

  bool Execute();

  //===--------------------------------------------------------------------===//
  // Children + Parent Helpers
  //===--------------------------------------------------------------------===//

  void AddChild(AbstractExecutor *child);

  //===--------------------------------------------------------------------===//
  // Accessors
  //===--------------------------------------------------------------------===//

  // Virtual because we want to be able to intercept via the mock executor
  // in test cases.
  virtual LogicalTile *GetOutput();

 protected:
  explicit AbstractExecutor(planner::AbstractPlanNode *node);
  explicit AbstractExecutor(planner::AbstractPlanNode *node, Transaction *context);

  /** @brief Init function to be overriden by derived class. */
  virtual bool DInit() = 0;

  /** @brief Workhorse function to be overriden by derived class. */
  virtual bool DExecute() = 0;

  void SetOutput(LogicalTile* val);

  /**
   * @brief Convenience method to return plan node corresponding to this
   *        executor, appropriately type-casted.
   *
   * @return Reference to plan node.
   */
  template <class T> inline const T& GetNode() {
    const T *node = dynamic_cast<const T *>(node_);
    assert(node);
    return *node;
  }

  /** @brief Children nodes of this executor in the executor tree. */
  std::vector<AbstractExecutor*> children_;

  // Transaction context
  Transaction *context_ = nullptr;

 private:

  // Output logical tile
  // This is where we will write the results of the plan node's execution
  std::unique_ptr<LogicalTile> output;

  /** @brief Plan node corresponding to this executor. */
  const planner::AbstractPlanNode *node_ = nullptr;
};

} // namespace executor
} // namespace nstore
