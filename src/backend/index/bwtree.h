//===----------------------------------------------------------------------===//
//
//                         PelotonDB
//
// BWTree.h
//
// Identification: src/backend/index/BWTree.h
//
// Copyright (c) 2015, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once
#include <cstdint>
#include <assert.h>
#include <vector>
#include <map>
#include <utility>
#include <set>

namespace peloton {
namespace index {
using namespace std;

typedef uint64_t epoch_t;

// typedef enum node_type {
//   INTERNAL_BW_NODE,
//   LEAF_BW_NODE,
//   INSERT,
//   UPDATE,
//   DELETE,
//   SPLIT,
//   MERGE,
//   REMOVE,
//   SPLIT_INDEX,
//   REMOVE_INDEX
// } node_type_t;
typedef int node_type_t;

#define INTERNAL_BW_NODE 1
#define LEAF_BW_NODE 2
#define INSERT 3
#define UPDATE 4
#define DELETE 5
#define SPLIT 6
#define MERGE 7
#define REMOVE 8
#define SPLIT_INDEX 9
#define REMOVE_INDEX 10

template <typename KeyType, typename ValueType, typename KeyComparator, typename KeyEqualityChecker, typename ValueEqualityChecker>
class BWTree;

template <typename KeyType, typename ValueType, typename KeyComparator, typename KeyEqualityChecker, typename ValueEqualityChecker>
class Node {

public:
  epoch_t generation;
  BWTree<KeyType, ValueType, KeyComparator, KeyEqualityChecker, ValueEqualityChecker>& my_tree; // reference of the tree I belong to
  uint64_t id;
  uint32_t chain_len;
  node_type_t type;
  Node<KeyType, ValueType, KeyComparator, KeyEqualityChecker, ValueEqualityChecker> *next;
  Node(BWTree<KeyType, ValueType, KeyComparator, KeyEqualityChecker, ValueEqualityChecker>& bwt, uint64_t id, node_type_t type) :
  my_tree(bwt), id(id), type(type) {
    next = nullptr;
	chain_len = 0;
  }
  ~Node() {
    next -> ~Node();
  }
  virtual bool Consolidate() = 0;
};

template <typename KeyType, typename ValueType, typename KeyComparator, typename KeyEqualityChecker, typename ValueEqualityChecker>
class CASMappingTable {
  typedef Node<KeyType, ValueType, KeyComparator, KeyEqualityChecker, ValueEqualityChecker> NodeType;

  private:
  map<uint64_t, NodeType*> cas_mapping_table; // should be capable of mapping to internal and leaf bw nodes and delta nodes of any type
  uint64_t cur_max_id;
  public:
  CASMappingTable() : cur_max_id(1) {};
  bool Install(uint64_t id, Node<KeyType, ValueType, KeyComparator, KeyEqualityChecker, ValueEqualityChecker>* node_ptr); // install into mapping table via compare and swap
  NodeType* Get (uint64_t id);
  uint64_t Get_next_id();
};

// Look up the stx btree interface for background.
// peloton/third_party/stx/btree.h
/**
 * BW tree-based index implementation.
 *
 * @see Index
 */
template <typename KeyType, typename ValueType, typename KeyComparator, typename KeyEqualityChecker, typename ValueEqualityChecker>
class BWTree {
  // friend class BWTreeIndex<KeyType, ValueType, KeyComparator, KeyEqualityChecker, ValueEqualityChecker>;
 public:
  KeyComparator comparator;
  KeyEqualityChecker equals;
  ValueEqualityChecker value_equals;
  CASMappingTable<KeyType, ValueType, KeyComparator, KeyEqualityChecker, ValueEqualityChecker> table;
 // BWTree() {CASMappingTable<KeyType, ValueType, KeyComparator, KeyEqualityChecker, ValueEqualityChecker> b;
//BWTree() {}
BWTree(KeyComparator comparator, KeyEqualityChecker equals, ValueEqualityChecker value_equals) 
: comparator(comparator), equals(equals), value_equals(value_equals) {}
 // BWTree(CASMappingTable<KeyType, ValueType, KeyComparator, KeyEqualityChecker, ValueEqualityChecker> table) : table(table){}
  uint32_t min_node_size;
  uint32_t max_node_size;
  uint64_t tree_height;
  uint64_t root; // root points to an id in the mapping table
  bool Consolidate(uint64_t id, bool force); // id is that of the mapping table entry
  bool Split_root(KeyType split_key, uint64_t left_pointer, uint64_t right_pointer);
  //NodeType * CreateNode(uint64_t id, node_type_t t){return nullptr;} // for creating when consolidating
  //bool DeleteNode(uint64_t id){return false;}
  // tianyuan - GC and the epoch mechanism
  bool Insert(KeyType key, ValueType value);
  bool Delete(KeyType key, ValueType value);
  uint64_t Search(KeyType key, uint64_t *path, uint64_t &location);
  vector<ValueType> Search_key(KeyType key);
  vector<ValueType> Search_all_keys();
  vector<ValueType> Search_range(KeyType low, KeyType high);
  uint64_t Get_size(uint64_t id) ;
};

template <typename KeyType, typename ValueType, typename KeyComparator, typename KeyEqualityChecker, typename ValueEqualityChecker>
class InternalBWNode : public Node<KeyType, ValueType, KeyComparator, KeyEqualityChecker, ValueEqualityChecker> {
  public:
  multimap<KeyType, uint64_t, KeyComparator> key_list; // all keys have children
  uint64_t leftmost_pointer;
  uint64_t sibling_id;
  uint64_t low;
  uint64_t high;
  InternalBWNode(BWTree<KeyType, ValueType, KeyComparator, KeyEqualityChecker, ValueEqualityChecker>& bwt, uint64_t id) :
  Node<KeyType, ValueType, KeyComparator, KeyEqualityChecker, ValueEqualityChecker>(bwt, id, INTERNAL_BW_NODE) {}
  bool Internal_insert(KeyType split_key, KeyType boundary_key, uint64_t new_node_id);
  bool Internal_split(uint64_t *path, uint64_t index, KeyType split_key, KeyType boundary_key, uint64_t new_node_id);
  bool Internal_delete(KeyType merged_key); 
  bool Internal_merge(uint64_t *path, uint64_t index, KeyType deleted_key);
  bool Consolidate() {return false;}
  uint64_t Get_child_id(KeyType key);
};

template <typename KeyType, typename ValueType, typename KeyComparator, typename KeyEqualityChecker, typename ValueEqualityChecker>
class LeafBWNode : public Node<KeyType, ValueType, KeyComparator, KeyEqualityChecker, ValueEqualityChecker> {

  public:
  multimap<KeyType, ValueType, KeyComparator> kv_list; // all key value pairs
  uint64_t sibling_id;  
  uint64_t low;
  uint64_t high;
  LeafBWNode(BWTree<KeyType, ValueType, KeyComparator, KeyEqualityChecker, ValueEqualityChecker>& bwt, uint64_t id) :
  Node<KeyType, ValueType, KeyComparator, KeyEqualityChecker, ValueEqualityChecker>(bwt, id, LEAF_BW_NODE) {}
  bool Leaf_insert(KeyType key, ValueType value);
  bool Leaf_delete(KeyType key, ValueType value);
  bool Leaf_split(uint64_t *path, uint64_t index, KeyType key, ValueType value);
  bool Leaf_merge(uint64_t *path, uint64_t index, KeyType key, ValueType value);
  bool Consolidate();
};

template <typename KeyType, typename ValueType, typename KeyComparator, typename KeyEqualityChecker, typename ValueEqualityChecker>
class DeltaNode : public Node<KeyType, ValueType, KeyComparator, KeyEqualityChecker, ValueEqualityChecker> {
  public:
  KeyType key;
  ValueType value;
  DeltaNode(BWTree<KeyType, ValueType, KeyComparator, KeyEqualityChecker, ValueEqualityChecker>& bwt, uint64_t id, node_type_t type) :
  Node<KeyType, ValueType, KeyComparator, KeyEqualityChecker, ValueEqualityChecker>(bwt, id, type) {} // Default is INSERT type
  bool Consolidate() {return false;}
};

template <typename KeyType, typename ValueType, typename KeyComparator, typename KeyEqualityChecker, typename ValueEqualityChecker>
class SplitIndexDeltaNode : public Node<KeyType, ValueType, KeyComparator, KeyEqualityChecker, ValueEqualityChecker> {
  public:
  SplitIndexDeltaNode(BWTree<KeyType, ValueType, KeyComparator, KeyEqualityChecker, ValueEqualityChecker>& bwt, uint64_t id) :
  Node<KeyType, ValueType, KeyComparator, KeyEqualityChecker, ValueEqualityChecker>(bwt, id, SPLIT_INDEX) {}
  KeyType split_key, boundary_key;
  uint64_t new_split_node_id;
  bool Consolidate() {return false;}
};

template <typename KeyType, typename ValueType, typename KeyComparator, typename KeyEqualityChecker, typename ValueEqualityChecker>
class RemoveIndexDeltaNode : public Node<KeyType, ValueType, KeyComparator, KeyEqualityChecker, ValueEqualityChecker> { 
  public:
  uint64_t id;
  RemoveIndexDeltaNode(BWTree<KeyType, ValueType, KeyComparator, KeyEqualityChecker, ValueEqualityChecker>& bwt, uint64_t id) :
  Node<KeyType, ValueType, KeyComparator, KeyEqualityChecker, ValueEqualityChecker>(bwt, id, REMOVE_INDEX) {}
  KeyType deleted_key;
  bool Consolidate() {return false;}
};

template <typename KeyType, typename ValueType, typename KeyComparator, typename KeyEqualityChecker, typename ValueEqualityChecker>
class SplitDeltaNode : public Node<KeyType, ValueType, KeyComparator, KeyEqualityChecker, ValueEqualityChecker> {
  public:
  SplitDeltaNode(BWTree<KeyType, ValueType, KeyComparator, KeyEqualityChecker, ValueEqualityChecker>& bwt, uint64_t id) :
  Node<KeyType, ValueType, KeyComparator, KeyEqualityChecker, ValueEqualityChecker>(bwt, id, SPLIT) {}
  KeyType split_key;
  uint64_t target_node_id; 
  bool Consolidate() {return false;}
};

template <typename KeyType, typename ValueType, typename KeyComparator, typename KeyEqualityChecker, typename ValueEqualityChecker>
class RemoveDeltaNode : public Node<KeyType, ValueType, KeyComparator, KeyEqualityChecker, ValueEqualityChecker> {
  public:
  //KeyType deleted_key; //The entire node is deleted and not a key, hence we don't need this
  //Node<KeyType, ValueType, KeyComparator, KeyEqualityChecker, ValueEqualityChecker> *node_to_be_removed; // can be delta Node<KeyType, ValueType, KeyComparator, KeyEqualityChecker, ValueEqualityChecker> or internal_bw_node or leaf_bw_node
  RemoveDeltaNode(BWTree<KeyType, ValueType, KeyComparator, KeyEqualityChecker, ValueEqualityChecker>& bwt, uint64_t id) :
  Node<KeyType, ValueType, KeyComparator, KeyEqualityChecker, ValueEqualityChecker>(bwt, id, REMOVE) {}
  bool Consolidate() {return false;}
};

template <typename KeyType, typename ValueType, typename KeyComparator, typename KeyEqualityChecker, typename ValueEqualityChecker>
class MergeDeltaNode : public Node<KeyType, ValueType, KeyComparator, KeyEqualityChecker, ValueEqualityChecker> {
  public:
  MergeDeltaNode(BWTree<KeyType, ValueType, KeyComparator, KeyEqualityChecker, ValueEqualityChecker>& bwt, uint64_t id) :
  Node<KeyType, ValueType, KeyComparator, KeyEqualityChecker, ValueEqualityChecker>(bwt, id, MERGE) {}
  KeyType merge_key;
  Node<KeyType, ValueType, KeyComparator, KeyEqualityChecker, ValueEqualityChecker> *node_to_be_merged;
  bool Consolidate() {return false;}
};
}  // End index namespace
}  // End peloton namespace
