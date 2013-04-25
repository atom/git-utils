#include "repository.h"
#include <string>
#include <map>
#include <vector>

using namespace std;

void Repository::Init(Handle<Object> target) {
  git_threads_init();

  Local<FunctionTemplate> tpl = FunctionTemplate::New(Repository::New);
  tpl->SetClassName(String::NewSymbol("Repository"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  Local<Function> getPath = FunctionTemplate::New(Repository::GetPath)->GetFunction();
  tpl->PrototypeTemplate()->Set(String::NewSymbol("getPath"), getPath);

  Local<Function> exists = FunctionTemplate::New(Repository::Exists)->GetFunction();
  tpl->PrototypeTemplate()->Set(String::NewSymbol("exists"), exists);

  Local<Function> getHead = FunctionTemplate::New(Repository::GetHead)->GetFunction();
  tpl->PrototypeTemplate()->Set(String::NewSymbol("getHead"), getHead);

  Local<Function> refreshIndex = FunctionTemplate::New(Repository::RefreshIndex)->GetFunction();
  tpl->PrototypeTemplate()->Set(String::NewSymbol("refreshIndex"), refreshIndex);

  Local<Function> isIgnored = FunctionTemplate::New(Repository::IsIgnored)->GetFunction();
  tpl->PrototypeTemplate()->Set(String::NewSymbol("isIgnored"), isIgnored);

  Local<Function> isSubmodule = FunctionTemplate::New(Repository::IsSubmodule)->GetFunction();
  tpl->PrototypeTemplate()->Set(String::NewSymbol("isSubmodule"), isSubmodule);

  Local<Function> getConfigValue = FunctionTemplate::New(Repository::GetConfigValue)->GetFunction();
  tpl->PrototypeTemplate()->Set(String::NewSymbol("getConfigValue"), getConfigValue);

  Local<Function> getStatus = FunctionTemplate::New(Repository::GetStatus)->GetFunction();
  tpl->PrototypeTemplate()->Set(String::NewSymbol("getStatus"), getStatus);

  Local<Function> checkoutHead = FunctionTemplate::New(Repository::CheckoutHead)->GetFunction();
  tpl->PrototypeTemplate()->Set(String::NewSymbol("checkoutHead"), checkoutHead);

  Local<Function> getReferenceTarget = FunctionTemplate::New(Repository::GetReferenceTarget)->GetFunction();
  tpl->PrototypeTemplate()->Set(String::NewSymbol("getReferenceTarget"), getReferenceTarget);

  Local<Function> getDiffStats = FunctionTemplate::New(Repository::GetDiffStats)->GetFunction();
  tpl->PrototypeTemplate()->Set(String::NewSymbol("getDiffStats"), getDiffStats);

  Local<Function> GetHeadBlob = FunctionTemplate::New(Repository::GetHeadBlob)->GetFunction();
  tpl->PrototypeTemplate()->Set(String::NewSymbol("getHeadBlob"), GetHeadBlob);

  Local<Function> getCommitCount = FunctionTemplate::New(Repository::GetCommitCount)->GetFunction();
  tpl->PrototypeTemplate()->Set(String::NewSymbol("getCommitCount"), getCommitCount);

  Local<Function> getMergeBase = FunctionTemplate::New(Repository::GetMergeBase)->GetFunction();
  tpl->PrototypeTemplate()->Set(String::NewSymbol("getMergeBase"), getMergeBase);

  Local<Function> release = FunctionTemplate::New(Repository::Release)->GetFunction();
  tpl->PrototypeTemplate()->Set(String::NewSymbol("release"), release);

  Local<Function> getLineDiffs = FunctionTemplate::New(Repository::GetLineDiffs)->GetFunction();
  tpl->PrototypeTemplate()->Set(String::NewSymbol("getLineDiffs"), getLineDiffs);

  Persistent<Function> constructor = Persistent<Function>::New(tpl->GetFunction());
  target->Set(String::NewSymbol("Repository"), constructor);
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
    return scope.Close(Boolean::New(false));

  git_repository* repository = GetRepository(args);
  String::Utf8Value utf8Path(Local<String>::Cast(args[0]));
  string path(*utf8Path);
  int ignored;
  if (git_ignore_path_is_ignored(&ignored, repository, path.data()) == GIT_OK)
    return scope.Close(Boolean::New(ignored == 1));
  else
    return scope.Close(Boolean::New(false));
}

Handle<Value> Repository::IsSubmodule(const Arguments& args) {
  HandleScope scope;
  if (args.Length() < 1)
    return scope.Close(Boolean::New(false));

  git_index* index;
  git_repository* repository = GetRepository(args);
  if (git_repository_index(&index, repository) == GIT_OK) {
    String::Utf8Value utf8Path(Local<String>::Cast(args[0]));
    string path(*utf8Path);
    const git_index_entry *entry = git_index_get_bypath(index, path.data(), 0);
    Handle<Boolean> isSubmodule = Boolean::New(entry != NULL && (entry->mode & S_IFMT) == GIT_FILEMODE_COMMIT);
    git_index_free(index);
    return scope.Close(isSubmodule);
  } else
    return scope.Close(Boolean::New(false));
}

Handle<Value> Repository::GetConfigValue(const Arguments& args) {
  HandleScope scope;
  if (args.Length() < 1)
    return scope.Close(Null());

  git_config* config;
  git_repository* repository = GetRepository(args);
  if (git_repository_config(&config, repository) != GIT_OK)
    return scope.Close(Null());

  String::Utf8Value utf8ConfigKey(Local<String>::Cast(args[0]));
  string configKey(*utf8ConfigKey);
  const char* configValue;
  if (git_config_get_string(&configValue, config, configKey.data()) == GIT_OK) {
    Handle<String> configValueHandle = String::NewSymbol(configValue);
    git_config_free(config);
    return scope.Close(configValueHandle);
  } else {
    git_config_free(config);
    return scope.Close(Null());
  }
}

Handle<Value> Repository::GetStatus(const Arguments& args) {
  HandleScope scope;
  if (args.Length() < 1) {
    HandleScope scope;
    Local<Object> result = Object::New();
    map<string, unsigned int> statuses;
    git_status_options options = GIT_STATUS_OPTIONS_INIT;
    options.flags = GIT_STATUS_OPT_INCLUDE_UNTRACKED | GIT_STATUS_OPT_RECURSE_UNTRACKED_DIRS;
    if (git_status_foreach_ext(GetRepository(args), &options, StatusCallback, &statuses) == GIT_OK) {
      map<string, unsigned int>::iterator iter = statuses.begin();
      for (; iter != statuses.end(); ++iter)
        result->Set(String::NewSymbol(iter->first.data()), Number::New(iter->second));
    }
    return scope.Close(result);
  } else {
    git_repository* repository = GetRepository(args);
    String::Utf8Value utf8Path(Local<String>::Cast(args[0]));
    string path(*utf8Path);
    unsigned int status = 0;
    if (git_status_file(&status, repository, path.data()) == GIT_OK)
      return scope.Close(Number::New(status));
    else
      return scope.Close(Number::New(0));
  }
}

Handle<Value> Repository::CheckoutHead(const Arguments& args) {
  HandleScope scope;
  if (args.Length() < 1)
    return scope.Close(Boolean::New(false));

  String::Utf8Value utf8Path(Local<String>::Cast(args[0]));
  string path(*utf8Path);
  char *copiedPath = (char*) malloc(sizeof(char) * (path.length() + 1));
  strcpy(copiedPath, path.data());

  git_checkout_opts options = GIT_CHECKOUT_OPTS_INIT;
  options.checkout_strategy = GIT_CHECKOUT_FORCE | GIT_CHECKOUT_DISABLE_PATHSPEC_MATCH;
  git_strarray paths;
  paths.count = 1;
  paths.strings = &copiedPath;
  options.paths = paths;

  int result = git_checkout_head(GetRepository(args), &options);
  free(copiedPath);
  return scope.Close(Boolean::New(result == GIT_OK));
}

Handle<Value> Repository::GetReferenceTarget(const Arguments& args) {
  HandleScope scope;
  if (args.Length() < 1)
    return scope.Close(Null());

  String::Utf8Value utf8RefName(Local<String>::Cast(args[0]));
  string refName(*utf8RefName);
  git_oid sha;
  if (git_reference_name_to_id(&sha, GetRepository(args), refName.data()) == GIT_OK) {
    char oid[GIT_OID_HEXSZ + 1];
    git_oid_tostr(oid, GIT_OID_HEXSZ + 1, &sha);
    return scope.Close(String::NewSymbol(oid));
  } else
    return scope.Close(Null());
}

Handle<Value> Repository::GetDiffStats(const Arguments& args) {
  HandleScope scope;
  int added = 0;
  int deleted = 0;
  Local<Object> result = Object::New();
  result->Set(String::NewSymbol("added"), Number::New(added));
  result->Set(String::NewSymbol("deleted"), Number::New(deleted));

  if (args.Length() < 1)
    return scope.Close(result);

  git_repository *repository = GetRepository(args);
  git_reference *head;
  if (git_repository_head(&head, repository) != GIT_OK)
    return scope.Close(result);

  const git_oid* sha = git_reference_target(head);
  git_commit *commit;
  int commitStatus = git_commit_lookup(&commit, repository, sha);
  git_reference_free(head);
  if (commitStatus != GIT_OK)
    return scope.Close(result);

  git_tree *tree;
  int treeStatus = git_commit_tree(&tree, commit);
  git_commit_free(commit);
  if (treeStatus != GIT_OK)
    return scope.Close(result);

  String::Utf8Value utf8Path(Local<String>::Cast(args[0]));
  string path(*utf8Path);
  char *copiedPath = (char*) malloc(sizeof(char) * (path.length() + 1));
  strcpy(copiedPath, path.data());
  git_diff_options options = GIT_DIFF_OPTIONS_INIT;
  git_strarray paths;
  paths.count = 1;
  paths.strings = &copiedPath;
  options.pathspec = paths;
  options.context_lines = 0;
  options.flags = GIT_DIFF_DISABLE_PATHSPEC_MATCH;

  git_diff_list *diffs;
  int diffStatus = git_diff_tree_to_workdir(&diffs, repository, tree, &options);
  git_tree_free(tree);
  free(copiedPath);
  if (diffStatus != GIT_OK)
    return scope.Close(result);

  int deltas = git_diff_num_deltas(diffs);
  if (deltas != 1) {
    git_diff_list_free(diffs);
    return scope.Close(result);
  }

  git_diff_patch *patch;
  int patchStatus = git_diff_get_patch(&patch, NULL, diffs, 0);
  git_diff_list_free(diffs);
  if (patchStatus != GIT_OK)
    return scope.Close(result);

  int hunks = git_diff_patch_num_hunks(patch);
  for (int i = 0; i < hunks; i++) {
    int lines = git_diff_patch_num_lines_in_hunk(patch, i);
    for (int j = 0; j < lines; j++) {
      char lineType;
      if (git_diff_patch_get_line_in_hunk(&lineType, NULL, NULL, NULL, NULL, patch, i, j) == GIT_OK) {
        switch (lineType) {
          case GIT_DIFF_LINE_ADDITION:
            added++;
            break;
          case GIT_DIFF_LINE_DELETION:
            deleted++;
            break;
        }
      }
    }
  }
  git_diff_patch_free(patch);

  result->Set(String::NewSymbol("added"), Number::New(added));
  result->Set(String::NewSymbol("deleted"), Number::New(deleted));
  return scope.Close(result);
}

Handle<Value> Repository::GetHeadBlob(const Arguments& args) {
  HandleScope scope;
  if (args.Length() < 1)
    return scope.Close(Null());

  String::Utf8Value utf8Path(Local<String>::Cast(args[0]));
  string path(*utf8Path);

  git_repository *repo = GetRepository(args);
  git_reference *head;
  if (git_repository_head(&head, repo) != GIT_OK)
    return scope.Close(Null());

  const git_oid *sha = git_reference_target(head);
  git_commit *commit;
  int commitStatus = git_commit_lookup(&commit, repo, sha);
  git_reference_free(head);
  if (commitStatus != GIT_OK)
    return scope.Close(Null());

  git_tree *tree;
  int treeStatus = git_commit_tree(&tree, commit);
  git_commit_free(commit);
  if (treeStatus != GIT_OK)
    return scope.Close(Null());

  git_tree_entry *treeEntry;
  if (git_tree_entry_bypath(&treeEntry, tree, path.data()) != GIT_OK) {
    git_tree_free(tree);
    return scope.Close(Null());
  }

  git_blob *blob = NULL;
  const git_oid *blobSha = git_tree_entry_id(treeEntry);
  if (blobSha != NULL && git_blob_lookup(&blob, repo, blobSha) != GIT_OK)
    blob = NULL;
  git_tree_entry_free(treeEntry);
  git_tree_free(tree);
  if (blob == NULL)
    return scope.Close(Null());

  const char *content = (const char *) git_blob_rawcontent(blob);
  git_blob_free(blob);

  return scope.Close(String::NewSymbol(content));
}

int Repository::StatusCallback(const char *path, unsigned int status, void *payload) {
  map<string, unsigned int> *statuses = (map<string, unsigned int> *) payload;
  statuses->insert(pair<string, unsigned int>(string(path), status));
  return GIT_OK;
}

Handle<Value> Repository::Release(const Arguments& args) {
  Repository *repo = node::ObjectWrap::Unwrap<Repository>(args.This());
  if (repo->repository != NULL) {
    git_repository_free(repo->repository);
    repo->repository = NULL;
  }

  HandleScope scope;
  return scope.Close(Undefined());
}

Handle<Value> Repository::GetCommitCount(const Arguments& args) {
  HandleScope scope;
  if (args.Length() < 2)
    return scope.Close(Number::New(0));

  String::Utf8Value utf8FromCommitId(Local<String>::Cast(args[0]));
  string fromCommitId(*utf8FromCommitId);
  git_oid fromCommit;
  if (git_oid_fromstr(&fromCommit, fromCommitId.data()) != GIT_OK)
    return scope.Close(Number::New(0));

  String::Utf8Value utf8ToCommitId(Local<String>::Cast(args[1]));
  string toCommitId(*utf8ToCommitId);
  git_oid toCommit;
  if (git_oid_fromstr(&toCommit, toCommitId.data()) != GIT_OK)
    return scope.Close(Number::New(0));

  git_revwalk *revWalk;
  if (git_revwalk_new(&revWalk, GetRepository(args)) != GIT_OK)
    return scope.Close(Number::New(0));

  git_revwalk_push(revWalk, &fromCommit);
  git_revwalk_hide(revWalk, &toCommit);
  git_oid currentCommit;
  int count = 0;
  while (git_revwalk_next(&currentCommit, revWalk) == GIT_OK)
    count++;
  git_revwalk_free(revWalk);
  return scope.Close(Number::New(count));
}

Handle<Value> Repository::GetMergeBase(const Arguments& args) {
  HandleScope scope;
  if (args.Length() < 2)
    return scope.Close(Null());

  String::Utf8Value utf8CommitOneId(Local<String>::Cast(args[0]));
  string commitOneId(*utf8CommitOneId);
  git_oid commitOne;
  if (git_oid_fromstr(&commitOne, commitOneId.data()) != GIT_OK)
    return scope.Close(Null());

  String::Utf8Value utf8CommitTwoId(Local<String>::Cast(args[1]));
  string commitTwoId(*utf8CommitTwoId);
  git_oid commitTwo;
  if (git_oid_fromstr(&commitTwo, commitTwoId.data()) != GIT_OK)
    return scope.Close(Null());

  git_oid mergeBase;
  if (git_merge_base(&mergeBase, GetRepository(args), &commitOne, &commitTwo) == GIT_OK) {
    char mergeBaseId[GIT_OID_HEXSZ + 1];
    git_oid_tostr(mergeBaseId, GIT_OID_HEXSZ + 1, &mergeBase);
    return scope.Close(String::NewSymbol(mergeBaseId));
  }

  return scope.Close(Null());
}

int Repository::DiffHunkCallback(const git_diff_delta *delta,
                                 const git_diff_range *range,
                                 const char *header, size_t header_len,
                                 void *payload) {
  vector<git_diff_range> *ranges = (vector<git_diff_range> *) payload;
  ranges->push_back(*range);
  return GIT_OK;
}

Handle<Value> Repository::GetLineDiffs(const Arguments& args) {
  HandleScope scope;
  if (args.Length() < 2)
    return scope.Close(Null());

  String::Utf8Value utf8Path(Local<String>::Cast(args[0]));
  string path(*utf8Path);
  String::Utf8Value utf8Text(Local<String>::Cast(args[1]));
  string text(*utf8Text);

  git_repository *repo = GetRepository(args);
  git_reference *head;
  if (git_repository_head(&head, repo) != GIT_OK)
    return scope.Close(Null());

  const git_oid *sha = git_reference_target(head);
  git_commit *commit;
  int commitStatus = git_commit_lookup(&commit, repo, sha);
  git_reference_free(head);
  if (commitStatus != GIT_OK)
    return scope.Close(Null());

  git_tree *tree;
  int treeStatus = git_commit_tree(&tree, commit);
  git_commit_free(commit);
  if (treeStatus != GIT_OK)
    return scope.Close(Null());

  git_tree_entry *treeEntry;
  if (git_tree_entry_bypath(&treeEntry, tree, path.data()) != GIT_OK) {
    git_tree_free(tree);
    return scope.Close(Null());
  }

  git_blob *blob = NULL;
  const git_oid *blobSha = git_tree_entry_id(treeEntry);
  if (blobSha != NULL && git_blob_lookup(&blob, repo, blobSha) != GIT_OK)
    blob = NULL;
  git_tree_entry_free(treeEntry);
  git_tree_free(tree);
  if (blob == NULL)
    return scope.Close(Null());

  vector<git_diff_range> ranges;
  git_diff_options options = GIT_DIFF_OPTIONS_INIT;
  options.context_lines = 0;
  if (git_diff_blob_to_buffer(blob, text.data(), text.length(), &options, NULL,
                              DiffHunkCallback, NULL, &ranges) == GIT_OK) {
    Local<Object> v8Ranges = Array::New(ranges.size());
    for(size_t i = 0; i < ranges.size(); i++) {
      Local<Object> v8Range = Object::New();
      v8Range->Set(String::NewSymbol("oldStart"), Number::New(ranges[i].old_start));
      v8Range->Set(String::NewSymbol("oldLines"), Number::New(ranges[i].old_lines));
      v8Range->Set(String::NewSymbol("newStart"), Number::New(ranges[i].new_start));
      v8Range->Set(String::NewSymbol("newLines"), Number::New(ranges[i].new_lines));
      v8Ranges->Set(i, v8Range);
    }
    git_blob_free(blob);
    return v8Ranges;
  } else {
    git_blob_free(blob);
    return scope.Close(Null());
  }
}

Repository::Repository(Handle<String> path) {
  String::Utf8Value utf8Path(path);
  string repositoryPath(*utf8Path);
  if (git_repository_open_ext(&repository, repositoryPath.data(), 0, NULL) != GIT_OK)
    repository = NULL;
}

Repository::~Repository() {
  if (repository != NULL) {
    git_repository_free(repository);
    repository = NULL;
  }
}
