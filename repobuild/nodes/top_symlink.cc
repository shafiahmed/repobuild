// Copyright 2013
// Author: Christopher Van Arsdale

#include "repobuild/env/input.h"
#include "repobuild/env/resource.h"
#include "repobuild/nodes/top_symlink.h"

namespace repobuild {

TopSymlinkNode::TopSymlinkNode(const TargetInfo& target,
                               const Input& input,
                               const ResourceFileSet& input_binaries)
    : Node(target, input),
      input_binaries_(input_binaries) {
}

TopSymlinkNode::~TopSymlinkNode() {
}

void TopSymlinkNode::LocalWriteMakeClean(Makefile::Rule* out) const {
  for (const Resource r : OutBinaries()) {
    out->MaybeRemoveSymlink(r.path());
  }
}

void TopSymlinkNode::LocalWriteMake(Makefile* out) const {
  for (const Resource& r : InputBinaries()) {
    Resource local = Resource::FromLocalPath(input().root_dir(), r.basename());
    Resource bin = Resource::FromLocalPath(input().binary_dir(), r.basename());
    out->WriteRootSymlink(local.path(), r.path());
    out->WriteRootSymlink(bin.path(), r.path());
  }
  WriteBaseUserTarget(out);
}

void TopSymlinkNode::LocalFinalOutputs(LanguageType lang,
                                       ResourceFileSet* outputs) const {
  for (const Resource& r : OutBinaries()) {
    outputs->Add(r);
  }
}

ResourceFileSet TopSymlinkNode::OutBinaries() const {
  ResourceFileSet inputs = InputBinaries();
  ResourceFileSet output;
  for (const Resource& r : inputs) {
    output.Add(Resource::FromLocalPath(input().root_dir(), r.basename()));
    output.Add(Resource::FromLocalPath(input().binary_dir(), r.basename()));
  }
  return output;
}

}  // namespace repobuild
