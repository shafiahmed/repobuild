// Copyright 2013
// Author: Christopher Van Arsdale

#include <iostream>
#include <set>
#include <vector>
#include <string>
#include "common/log/log.h"
#include "repobuild/env/input.h"
#include "repobuild/env/resource.h"
#include "repobuild/generator/generator.h"
#include "repobuild/nodes/allnodes.h"
#include "repobuild/nodes/node.h"
#include "repobuild/reader/parser.h"

using std::string;
using std::vector;
using std::set;

namespace repobuild {
namespace {

void ExpandNode(const Parser& parser,
                const Node* node,
                set<const Node*>* parents,
                set<const Node*>* seen,
                vector<const Node*>* to_process) {
  if (!seen->insert(node).second) {
    // Already processed.
    return;
  }

  // Check for recursive dependency
  if (!parents->insert(node).second) {
    LOG(FATAL) << "Recursive dependency: " << node->target().full_path();
  }

  // Now expand our sub-node dependencies.
  for (const Node* dep : node->dependencies()) {
    ExpandNode(parser, dep, parents, seen, to_process);
  }

  // And record this node as ready to go in the queue.
  parents->erase(node);
  to_process->push_back(node);
}

}  // anonymous namespace

Generator::Generator() {
}

Generator::~Generator() {
}

string Generator::GenerateMakefile(const Input& input) {
  // Our set of node types (cc_library, etc.).
  NodeBuilderSet builder_set;

  // Initialize makefile.
  Makefile out;
  out.SetSilent(input.silent_make());
  out.append("# Auto-generated by repobuild, do not modify directly.\n\n");
  builder_set.WriteMakeHead(input, &out);

  // Get our input tree of nodes.
  repobuild::Parser parser(&builder_set);
  parser.Parse(input);

  // Figure out the order we want to write in our Makefile.
  set<const Node*> parents, seen;
  vector<const Node*> process_order;
  for (const Node* node : parser.input_nodes()) {
    ExpandNode(parser, node, &parents, &seen, &process_order);
  }

  std::cout << "Generating: Makefile" << std::endl;

  // Generate the makefile.
  for (const Node* node : process_order) {
    node->WriteMake(&out);
  }

  // Finish up node make files
  builder_set.FinishMakeFile(input, process_order, &out);

  // Write the make clean rule.
  Makefile::Rule* clean = out.StartRule("clean", "");
  for (const Node* node : process_order) {
    node->WriteMakeClean(clean);
  }
  clean->WriteCommand("rm -rf " + input.object_dir());
  clean->WriteCommand("rm -rf " + input.binary_dir());
  clean->WriteCommand("rm -rf " + input.genfile_dir());
  clean->WriteCommand("rm -rf " + input.source_dir());
  clean->WriteCommand("rm -rf " + input.pkgfile_dir());
  out.FinishRule(clean);

  // Write the all rule.
  ResourceFileSet outputs;
  for (const Node* node : parser.all_nodes()) {
    if (input.contains_target(node->target().full_path())) {
      node->FinalOutputs(Node::NO_LANG, &outputs);
      outputs.Add(Resource::FromRootPath(node->target().make_path()));
    }
  }
  out.WriteRule("all", strings::JoinAll(outputs.files(), " "));

  // Not real files:
  out.WriteRule(".PHONY", "clean all");

  // Default build everything.
  out.append(".DEFAULT_GOAL=all\n\n");

  return out.out();
}

}  // namespace repobuild
