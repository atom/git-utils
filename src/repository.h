#include <node.h>
#include <v8.h>
#include "git2.h"

using namespace v8;

class Repository : public node::ObjectWrap {
  public:
    static void Init(Handle<Object> target);

  private:
    static Handle<Value> New(const Arguments& args);
    static Handle<Value> GetPath(const Arguments& args);
    static Handle<Value> Exists(const Arguments& args);

    Repository(Handle<String> path);
    ~Repository();
    const char* GetPath();

    git_repository* repository;
};
