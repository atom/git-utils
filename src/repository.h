#ifndef SRC_REPOSITORY_H_
#define SRC_REPOSITORY_H_

#include <node.h>
#include <v8.h>
#include "git2.h"

using ::v8::Arguments;
using ::v8::Handle;
using ::v8::Object;
using ::v8::String;
using ::v8::Value;

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
    static Handle<Value> GetStatus(const Arguments& args);
    static Handle<Value> CheckoutHead(const Arguments& args);
    static Handle<Value> GetReferenceTarget(const Arguments& args);
    static Handle<Value> GetDiffStats(const Arguments& args);
    static Handle<Value> GetHeadBlob(const Arguments& args);
    static Handle<Value> GetCommitCount(const Arguments& args);
    static Handle<Value> GetMergeBase(const Arguments& args);
    static Handle<Value> Release(const Arguments& args);
    static Handle<Value> GetLineDiffs(const Arguments& args);

    static int StatusCallback(const char *path, unsigned int status,
                              void *payload);
    static int DiffHunkCallback(const git_diff_delta *delta,
                                const git_diff_range *range,
                                const char *header, size_t header_len,
                                void *payload);
    static git_repository* GetRepository(const Arguments& args);

    explicit Repository(Handle<String> path);
    ~Repository();

    git_repository* repository;
};

#endif  // SRC_REPOSITORY_H_
