// Copyright (c) 2013 GitHub Inc.
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#ifndef SRC_REPOSITORY_H_
#define SRC_REPOSITORY_H_

#include <string>
#include <vector>

#include "git2.h"
#include "nan.h"
using namespace v8;  // NOLINT

class Repository : public node::ObjectWrap {
 public:
  static void Init(Handle<Object> target);

 private:
  static Handle<Value> New(const Arguments& args);
  static Handle<Value> GetPath(const Arguments& args);
  static Handle<Value> Exists(const Arguments& args);
  static Handle<Value> GetHead(const Arguments& args);
  static Handle<Value> RefreshIndex(const Arguments& args);
  static Handle<Value> IsIgnored(const Arguments& args);
  static Handle<Value> IsSubmodule(const Arguments& args);
  static Handle<Value> GetConfigValue(const Arguments& args);
  static Handle<Value> SetConfigValue(const Arguments& args);
  static Handle<Value> GetStatus(const Arguments& args);
  static Handle<Value> CheckoutHead(const Arguments& args);
  static Handle<Value> GetReferenceTarget(const Arguments& args);
  static Handle<Value> GetDiffStats(const Arguments& args);
  static Handle<Value> GetHeadBlob(const Arguments& args);
  static Handle<Value> GetCommitCount(const Arguments& args);
  static Handle<Value> GetMergeBase(const Arguments& args);
  static Handle<Value> Release(const Arguments& args);
  static Handle<Value> GetLineDiffs(const Arguments& args);
  static Handle<Value> GetReferences(const Arguments& args);
  static Handle<Value> CheckoutReference(const Arguments& args);

  static int StatusCallback(const char *path, unsigned int status,
                            void *payload);
  static int DiffHunkCallback(const git_diff_delta *delta,
                              const git_diff_hunk *hunk,
                              void *payload);
  static Handle<Value> ConvertStringVectorToV8Array(
      const std::vector<std::string>& std::vector);

  static git_repository* GetRepository(const Arguments& args);

  static git_diff_options CreateDefaultGitDiffOptions();

  explicit Repository(Handle<String> path);
  ~Repository();

  git_repository* repository;
};

#endif  // SRC_REPOSITORY_H_
