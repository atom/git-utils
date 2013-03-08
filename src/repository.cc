#include "repository.h"
#include <string>

using namespace v8;
using namespace std;

void Repository::Init(Handle<Object> target) {
  git_threads_init();

  Local<FunctionTemplate> tpl = FunctionTemplate::New(Repository::New);
  tpl->SetClassName(v8::String::NewSymbol("Repository"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  Local<Function> getPath = FunctionTemplate::New(Repository::GetPath)->GetFunction();
  tpl->PrototypeTemplate()->Set(v8::String::NewSymbol("getPath"), getPath);

  Local<Function> exists = FunctionTemplate::New(Repository::Exists)->GetFunction();
  tpl->PrototypeTemplate()->Set(v8::String::NewSymbol("exists"), exists);

  Local<Function> getHead = FunctionTemplate::New(Repository::GetHead)->GetFunction();
  tpl->PrototypeTemplate()->Set(v8::String::NewSymbol("getHead"), getHead);

  Local<Function> refreshIndex = FunctionTemplate::New(Repository::RefreshIndex)->GetFunction();
  tpl->PrototypeTemplate()->Set(v8::String::NewSymbol("refreshIndex"), refreshIndex);

  Local<Function> IsIgnored = FunctionTemplate::New(Repository::IsIgnored)->GetFunction();
  tpl->PrototypeTemplate()->Set(v8::String::NewSymbol("isIgnored"), IsIgnored);

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

git_repository* Repository::GetRepository(const Arguments& args) {
  return node::ObjectWrap::Unwrap<Repository>(args.This())->repository;
}

Handle<Value> Repository::Exists(const Arguments& args) {
  HandleScope scope;
  return scope.Close(Boolean::New(GetRepository(args) != NULL));
}

Handle<Value> Repository::GetPath(const Arguments& args) {
  HandleScope scope;
  git_repository* repository = GetRepository(args);
  const char* path = git_repository_path(repository);
  return scope.Close(String::NewSymbol(path));
}

Handle<Value> Repository::GetHead(const Arguments& args) {
  HandleScope scope;
  git_repository* repository = GetRepository(args);
  git_reference *head;
  if (git_repository_head(&head, repository) != GIT_OK)
    return scope.Close(Null());

  if (git_repository_head_detached(repository) == 1) {
    const git_oid* sha = git_reference_target(head);
    if (sha != NULL) {
      char oid[GIT_OID_HEXSZ + 1];
      git_oid_tostr(oid, GIT_OID_HEXSZ + 1, sha);
      git_reference_free(head);
      return scope.Close(String::NewSymbol(oid));
    }
  }

  Local<String> referenceName =  String::NewSymbol(git_reference_name(head));
  git_reference_free(head);
  return scope.Close(referenceName);
}

Handle<Value> Repository::RefreshIndex(const Arguments& args) {
  git_repository* repository = GetRepository(args);
  git_index* index;
  if (git_repository_index(&index, repository) == GIT_OK) {
    git_index_read(index);
    git_index_free(index);
  }
  HandleScope scope;
  return scope.Close(Undefined());
}

Handle<Value> Repository::IsIgnored(const Arguments& args) {
  HandleScope scope;
  if (args.Length() < 1)
    return scope.Close(Boolean::New(FALSE));

  git_repository* repository = GetRepository(args);
  String::Utf8Value utf8Path(Local<String>::Cast(args[0]));
  string path(*utf8Path);
  int ignored;
  if (git_ignore_path_is_ignored(&ignored, repository, path.c_str()) == GIT_OK)
    return scope.Close(Boolean::New(ignored == 1));
  else
    return scope.Close(Boolean::New(FALSE));
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
