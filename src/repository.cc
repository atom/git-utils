#include "repository.h"
#include <string>

using namespace v8;
using namespace std;

void Repository::Init(Handle<Object> target) {
  git_threads_init();

  Local<FunctionTemplate> tpl = FunctionTemplate::New(Repository::New);
  tpl->SetClassName(v8::String::NewSymbol("Repository"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  tpl->PrototypeTemplate()->Set(v8::String::NewSymbol("getPath"), FunctionTemplate::New(Repository::GetPath)->GetFunction());
  tpl->PrototypeTemplate()->Set(v8::String::NewSymbol("exists"), FunctionTemplate::New(Repository::Exists)->GetFunction());

  Persistent<Function> constructor = Persistent<Function>::New(tpl->GetFunction());
  target->Set(v8::String::NewSymbol("Repository"), constructor);
}

NODE_MODULE(git, Repository::Init)

Handle<Value> Repository::New(const Arguments& args) {
  HandleScope scope;
  Repository* repository = new Repository(Local<String>::Cast(args[0]));
  repository->Wrap(args.This());
  return args.This();
}

Handle<Value> Repository::Exists(const Arguments& args) {
  HandleScope scope;
  Repository* repository = node::ObjectWrap::Unwrap<Repository>(args.This());
  return Boolean::New(repository->repository != NULL);
}

Handle<Value> Repository::GetPath(const Arguments& args) {
  HandleScope scope;
  Repository* repository = node::ObjectWrap::Unwrap<Repository>(args.This());
  const char* path = repository->GetPath();
  return scope.Close(String::NewSymbol(path));
}

Repository::Repository(Handle<String> path) {
  String::Utf8Value utf8Path(path);
  string repositoryPath(*utf8Path);
  if (git_repository_open_ext(&repository, repositoryPath.c_str(), 0, NULL) != GIT_OK)
    repository = NULL;
}

Repository::~Repository() {
  if (repository != NULL)
    git_repository_free(repository);
}

const char* Repository::GetPath() {
  return git_repository_path(repository);
}
