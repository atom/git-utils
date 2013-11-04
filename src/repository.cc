#include "repository.h"
#include <cstring>
#include <string>
#include <map>
#include <utility>
#include <vector>

using ::v8::Array;
using ::v8::Boolean;
using ::v8::Function;
using ::v8::FunctionTemplate;
using ::v8::HandleScope;
using ::v8::Local;
using ::v8::Number;
using ::v8::Null;
using ::v8::ObjectTemplate;
using ::v8::Persistent;
using ::v8::Undefined;

using ::std::map;
using ::std::pair;
using ::std::string;
using ::std::vector;

void Repository::Init(Handle<Object> target) {
  git_threads_init();

  Local<FunctionTemplate> newTemplate = FunctionTemplate::New(Repository::New);
  newTemplate->SetClassName(String::NewSymbol("Repository"));
  newTemplate->InstanceTemplate()->SetInternalFieldCount(1);

  Local<ObjectTemplate> prototype = newTemplate->PrototypeTemplate();

  prototype->Set(String::NewSymbol("getPath"),
                 FunctionTemplate::New(Repository::GetPath)->GetFunction());

  prototype->Set(String::NewSymbol("exists"),
                 FunctionTemplate::New(Repository::Exists)->GetFunction());

  prototype->Set(String::NewSymbol("getHead"),
                 FunctionTemplate::New(Repository::GetHead)->GetFunction());

  prototype->Set(String::NewSymbol("refreshIndex"),
                 FunctionTemplate::New(Repository::RefreshIndex)->GetFunction());

  prototype->Set(String::NewSymbol("isIgnored"),
                 FunctionTemplate::New(Repository::IsIgnored)->GetFunction());

  prototype->Set(String::NewSymbol("isSubmodule"),
                 FunctionTemplate::New(Repository::IsSubmodule)->GetFunction());

  prototype->Set(String::NewSymbol("getConfigValue"),
                 FunctionTemplate::New(Repository::GetConfigValue)->GetFunction());

  prototype->Set(String::NewSymbol("setConfigValue"),
                 FunctionTemplate::New(Repository::SetConfigValue)->GetFunction());

  prototype->Set(String::NewSymbol("getStatus"),
                 FunctionTemplate::New(Repository::GetStatus)->GetFunction());

  prototype->Set(String::NewSymbol("checkoutHead"),
                 FunctionTemplate::New(Repository::CheckoutHead)->GetFunction());

  prototype->Set(String::NewSymbol("getReferenceTarget"),
                 FunctionTemplate::New(Repository::GetReferenceTarget)->GetFunction());

  prototype->Set(String::NewSymbol("getDiffStats"),
                 FunctionTemplate::New(Repository::GetDiffStats)->GetFunction());

  prototype->Set(String::NewSymbol("getHeadBlob"),
                 FunctionTemplate::New(Repository::GetHeadBlob)->GetFunction());

  prototype->Set(String::NewSymbol("getCommitCount"),
                 FunctionTemplate::New(Repository::GetCommitCount)->GetFunction());

  prototype->Set(String::NewSymbol("getMergeBase"),
                 FunctionTemplate::New(Repository::GetMergeBase)->GetFunction());

  prototype->Set(String::NewSymbol("release"),
                 FunctionTemplate::New(Repository::Release)->GetFunction());

  prototype->Set(String::NewSymbol("getLineDiffs"),
                 FunctionTemplate::New(Repository::GetLineDiffs)->GetFunction());

  prototype->Set(String::NewSymbol("getReferences"),
                 FunctionTemplate::New(Repository::GetReferences)->GetFunction());

  prototype->Set(String::NewSymbol("checkoutRef"),
                 FunctionTemplate::New(Repository::CheckoutReference)->GetFunction());

  target->Set(String::NewSymbol("Repository"),
              Persistent<Function>::New(newTemplate->GetFunction()));
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

// C++ equivalent to GIT_DIFF_OPTIONS_INIT, we can not use it directly because
// of C++'s strong typing.
git_diff_options Repository::CreateDefaultGitDiffOptions() {
  git_diff_options options = { 0 };
  options.version = GIT_DIFF_OPTIONS_VERSION;
  options.context_lines = 3;
  return options;
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
    git_index_read(index, 0);
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

Handle<Value> Repository::SetConfigValue(const Arguments& args) {
  HandleScope scope;
  if (args.Length() != 2)
    return scope.Close(Boolean::New(false));

  git_config* config;
  git_repository* repository = GetRepository(args);
  if (git_repository_config(&config, repository) != GIT_OK)
    return scope.Close(Boolean::New(false));

  String::Utf8Value utf8ConfigKey(Local<String>::Cast(args[0]));
  string configKey(*utf8ConfigKey);

  String::Utf8Value utf8ConfigValue(Local<String>::Cast(args[1]));
  string configValue(*utf8ConfigValue);

  int errorCode = git_config_set_string(config, configKey.data(), configValue.data());
  git_config_free(config);
  return scope.Close(Boolean::New(errorCode == GIT_OK));
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

  String::Utf8Value utf8Path(args[0]);
  char* path = *utf8Path;

  git_checkout_opts options = GIT_CHECKOUT_OPTS_INIT;
  options.checkout_strategy = GIT_CHECKOUT_FORCE | GIT_CHECKOUT_DISABLE_PATHSPEC_MATCH;
  git_strarray paths;
  paths.count = 1;
  paths.strings = &path;
  options.paths = paths;

  int result = git_checkout_head(GetRepository(args), &options);
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

  String::Utf8Value utf8Path(args[0]);
  char* path = *utf8Path;

  git_diff_options options = CreateDefaultGitDiffOptions();
  git_strarray paths;
  paths.count = 1;
  paths.strings = &path;
  options.pathspec = paths;
  options.context_lines = 0;
  options.flags = GIT_DIFF_DISABLE_PATHSPEC_MATCH;

  git_diff *diffs;
  int diffStatus = git_diff_tree_to_workdir(&diffs, repository, tree, &options);
  git_tree_free(tree);
  if (diffStatus != GIT_OK)
    return scope.Close(result);

  int deltas = git_diff_num_deltas(diffs);
  if (deltas != 1) {
    git_diff_free(diffs);
    return scope.Close(result);
  }

  git_patch *patch;
  int patchStatus = git_patch_from_diff(&patch, diffs, 0);
  git_diff_free(diffs);
  if (patchStatus != GIT_OK)
    return scope.Close(result);

  int hunks = git_patch_num_hunks(patch);
  for (int i = 0; i < hunks; i++) {
    int lines = git_patch_num_lines_in_hunk(patch, i);
    for (int j = 0; j < lines; j++) {
      const git_diff_line *line;
      if (git_patch_get_line_in_hunk(&line, patch, i, j) == GIT_OK) {
        switch (line->origin) {
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
  git_patch_free(patch);

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
  Handle<Value> value = scope.Close(String::NewSymbol(content));
  git_blob_free(blob);
  return value;
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
                                 const git_diff_hunk *range,
                                 void *payload) {
  vector<git_diff_hunk> *ranges = (vector<git_diff_hunk> *) payload;
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

  vector<git_diff_hunk> ranges;
  git_diff_options options = CreateDefaultGitDiffOptions();

  // Set GIT_DIFF_IGNORE_WHITESPACE_EOL when ignoreEolWhitespace: true
  if (args.Length() >= 3) {
    Local<Object> optionsArg(Local<Object>::Cast(args[2]));
    if (optionsArg->Get(String::NewSymbol("ignoreEolWhitespace"))->ToBoolean()->Value())
      options.flags = GIT_DIFF_IGNORE_WHITESPACE_EOL;
  }

  options.context_lines = 0;
  if (git_diff_blob_to_buffer(blob, NULL, text.data(), text.length(), NULL,
                              &options, NULL, DiffHunkCallback, NULL,
                              &ranges) == GIT_OK) {
    Local<Object> v8Ranges = Array::New(ranges.size());
    for (size_t i = 0; i < ranges.size(); i++) {
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

Handle<Value> Repository::ConvertStringVectorToV8Array(vector<string> vector) {
  size_t i = 0, size = vector.size();
  Local<Object> array = Array::New(size);
  for (i = 0; i < size; i++)
    array->Set(i, String::NewSymbol(vector[i].data()));

  return array;
}

Handle<Value> Repository::GetReferences(const Arguments& args) {
  Local<Object> references = Object::New();
  vector<string> heads, remotes, tags;

  git_strarray strarray;
  git_reference_list(&strarray, GetRepository(args));

  for (unsigned int i = 0; i < strarray.count; i++)
    if (strncmp(strarray.strings[i], "refs/heads/", 11) == 0)
      heads.push_back(strarray.strings[i]);
    else if (strncmp(strarray.strings[i], "refs/remotes/", 13) == 0)
      remotes.push_back(strarray.strings[i]);
    else if (strncmp(strarray.strings[i], "refs/tags/", 10) == 0)
      tags.push_back(strarray.strings[i]);

  git_strarray_free(&strarray);

  references->Set(String::NewSymbol("heads"), ConvertStringVectorToV8Array(heads));
  references->Set(String::NewSymbol("remotes"), ConvertStringVectorToV8Array(remotes));
  references->Set(String::NewSymbol("tags"), ConvertStringVectorToV8Array(tags));

  return references;
}

int branch_checkout(git_repository *repo, const char *refName) {
  git_reference *ref = NULL;
  git_object *git_obj = NULL;
  git_checkout_opts opts = GIT_CHECKOUT_OPTS_INIT;
  opts.checkout_strategy = GIT_CHECKOUT_SAFE;
  int success = -1;

  if (!(success = git_reference_lookup(&ref, repo, refName)) &&
      !(success = git_reference_peel(&git_obj, ref, GIT_OBJ_TREE)) &&
      !(success = git_checkout_tree(repo, git_obj, &opts)))
    success = git_repository_set_head(repo, refName);

  git_object_free(git_obj);
  git_obj = NULL;
  git_reference_free(ref);
  ref = NULL;

  return success;
}

Handle<Value> Repository::CheckoutReference(const Arguments& args) {
  HandleScope scope;

  if (args.Length() < 1)
    return scope.Close(Boolean::New(false));

  bool shouldCreateNewRef;
  if (args.Length() > 1 && args[1]->ToBoolean()->Value())
    shouldCreateNewRef = true;
  else
    shouldCreateNewRef = false;

  String::Utf8Value utf8RefName(Local<String>::Cast(args[0]));
  string strRefName(*utf8RefName);
  const char* refName = strRefName.data();

  git_repository *repo = GetRepository(args);

  if (branch_checkout(repo, refName) == GIT_OK) {
    return scope.Close(Boolean::New(true));
  } else if (shouldCreateNewRef) {
    git_reference *head;
    if (git_repository_head(&head, repo) != GIT_OK)
      return scope.Close(Boolean::New(false));

    const git_oid* sha = git_reference_target(head);
    git_commit *commit;
    int commitStatus = git_commit_lookup(&commit, repo, sha);
    git_reference_free(head);

    if (commitStatus != GIT_OK)
      return scope.Close(Boolean::New(false));

    git_reference *branch;
    // N.B.: git_branch_create needs a name like 'xxx', not 'refs/heads/xxx'
    const int kShortNameLength = strRefName.length() - 11;
    string shortRefName(strRefName.data() + 11, kShortNameLength);

    int branchCreateStatus = git_branch_create(&branch, repo, shortRefName.c_str(), commit, 0);
    git_commit_free(commit);

    if (branchCreateStatus != GIT_OK)
      return scope.Close(Boolean::New(false));

    git_reference_free(branch);

    if (branch_checkout(repo, refName) == GIT_OK)
      return scope.Close(Boolean::New(true));
  }

  return scope.Close(Boolean::New(false));
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
