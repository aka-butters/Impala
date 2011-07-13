// Copyright (c) 2011 Cloudera, Inc. All rights reserved.

#include "exec/exec-node.h"
#include "exec/text-scan-node.h"
#include "common/object-pool.h"
#include "common/status.h"
#include "runtime/mem-pool.h"
#include "gen-cpp/PlanNodes_types.h"

using namespace std;

namespace impala {

Status ExecNode::CreateTree(ObjectPool* pool, const TPlan& plan, ExecNode** root) {
  if (plan.nodes.size() == 0) {
    *root = NULL;
    return Status::OK;
  }
  int node_idx = 0;
  RETURN_IF_ERROR(CreateTreeHelper(pool, plan.nodes, NULL, &node_idx, root));
  if (node_idx + 1 != plan.nodes.size()) {
    // TODO: print thrift msg for diagnostic purposes.
    return Status(
        "Plan tree only partially reconstructed. Not all thrift nodes were used.");
  }
  return Status::OK;
}

Status ExecNode::CreateTreeHelper(
    ObjectPool* pool,
    const vector<TPlanNode>& tnodes,
    ExecNode* parent,
    int* node_idx,
    ExecNode** root) {
  // propagate error case
  if (*node_idx >= tnodes.size()) {
    // TODO: print thrift msg
    return Status("Failed to reconstruct plan tree from thrift.");
  }
  int num_children = tnodes[*node_idx].num_children;
  ExecNode* node = NULL;
  RETURN_IF_ERROR(CreateNode(pool, tnodes[*node_idx], &node));
  // assert(parent != NULL || (node_idx == 0 && root_expr != NULL));
  if (parent != NULL) {
    parent->children_.push_back(node);
  } else {
    *root = node;
  }
  for (int i = 0; i < num_children; i++) {
    ++*node_idx;
    RETURN_IF_ERROR(CreateTreeHelper(pool, tnodes, node, node_idx, NULL));
    // we are expecting a child, but have used all nodes
    // this means we have been given a bad tree and must fail
    if (*node_idx >= tnodes.size()) {
      // TODO: print thrift msg
      return Status("Failed to reconstruct plan tree from thrift.");
    }
  }
  return Status::OK;
}

Status ExecNode::CreateNode(ObjectPool* pool, const TPlanNode& tnode, ExecNode** node) {
  switch (tnode.node_type) {
    case TPlanNodeType::TEXT_SCAN_NODE:
      *node = pool->Add(new TextScanNode(tnode.scan_node));
      return Status::OK;
    case TPlanNodeType::AGGREGATION_NODE:
      return Status("Aggregation node not implemented");
    case TPlanNodeType::SORT_NODE:
      return Status("Sort node not implemented");
  }
}

}
