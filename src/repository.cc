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

#include "repository.h"

#include <string.h>

#include <map>
#include <utility>

void Repository::Init(Handle<Object> target) {
  NanScope();

  git_threads_init();

  Local<FunctionTemplate> newTemplate = NanNew<FunctionTemplate>(
      Repository::New);
  newTemplate->SetClassName(NanNew<String>("Repository"));
  newTemplate->InstanceTemplate()->SetInternalFieldCount(1);

  Local<ObjectTemplate> proto = newTemplate->PrototypeTemplate();
  NODE_SET_METHOD(proto, "getPath", Repository::GetPath);
  NODE_SET_METHOD(proto, "_getWorkingDirectory",
                  Repository::GetWorkingDirectory);
  NODE_SET_METHOD(proto, "exists", Repository::Exists);
  NODE_SET_METHOD(proto, "getSubmodulePaths", Repository::GetSubmodulePaths);
  NODE_SET_METHOD(proto, "getHead", Repository::GetHead);
  NODE_SET_METHOD(proto, "refreshIndex", Repository::RefreshIndex);
  NODE_SET_METHOD(proto, "isIgnored", Repository::IsIgnored);
  NODE_SET_METHOD(proto, "isSubmodule", Repository::IsSubmodule);
  NODE_SET_METHOD(proto, "getConfigValue", Repository::GetConfigValue);
  NODE_SET_METHOD(proto, "setConfigValue", Repository::SetConfigValue);
  NODE_SET_METHOD(proto, "getStatus", Repository::GetStatus);
  NODE_SET_METHOD(proto, "checkoutHead", Repository::CheckoutHead);
  NODE_SET_METHOD(proto, "getReferenceTarget", Repository::GetReferenceTarget);
  NODE_SET_METHOD(proto, "getDiffStats", Repository::GetDiffStats);
  NODE_SET_METHOD(proto, "getIndexBlob", Repository::GetIndexBlob);
  NODE_SET_METHOD(proto, "getHeadBlob", Repository::GetHeadBlob);
  NODE_SET_METHOD(proto, "getCommitCount", Repository::GetCommitCount);
  NODE_SET_METHOD(proto, "getMergeBase", Repository::GetMergeBase);
  NODE_SET_METHOD(proto, "_release", Repository::Release);
  NODE_SET_METHOD(proto, "getLineDiffs", Repository::GetLineDiffs);
  NODE_SET_METHOD(proto, "getLineDiffDetails", Repository::GetLineDiffDetails);
  NODE_SET_METHOD(proto, "getReferences", Repository::GetReferences);
  NODE_SET_METHOD(proto, "checkoutRef", Repository::CheckoutReference);
  NODE_SET_METHOD(proto, "add", Repository::Add);

  target->Set(NanNew<String>("Repository"), newTemplate->GetFunction());
}

NODE_MODULE(git, Repository::Init)

NAN_METHOD(Repository::New) {
  NanScope();
  Repository* repository = new Repository(Local<String>::Cast(args[0]));
  repository->Wrap(args.This());
  NanReturnUndefined();
}

git_repository* Repository::GetRepository(_NAN_METHOD_ARGS_TYPE args) {
  return node::ObjectWrap::Unwrap<Repository>(args.This())->repository;
}

int Repository::GetBlob(_NAN_METHOD_ARGS_TYPE args,
                        git_repository* repo, git_blob*& blob) {
  std::string path(*String::Utf8Value(args[0]));

  int useIndex = false;
  if (args.Length() >= 3) {
    Local<Object> optionsArg(Local<Object>::Cast(args[2]));
    if (optionsArg->Get(NanNew<String>("useIndex"))->BooleanValue())
      useIndex = true;
  }

  if (useIndex) {
    git_index* index;
    if (git_repository_index(&index, repo) != GIT_OK)
      return -1;

    git_index_read(index, 0);
    const git_index_entry* entry = git_index_get_bypath(index, path.data(), 0);
    if (entry == NULL) {
      git_index_free(index);
      return -1;
    }

    const git_oid* blobSha = &entry->id;
    if (blobSha != NULL && git_blob_lookup(&blob, repo, blobSha) != GIT_OK)
      blob = NULL;
  } else {
    git_reference* head;
    if (git_repository_head(&head, repo) != GIT_OK)
      return -1;

    const git_oid* sha = git_reference_target(head);
    git_commit* commit;
    int commitStatus = git_commit_lookup(&commit, repo, sha);
    git_reference_free(head);
    if (commitStatus != GIT_OK)
      return -1;

    git_tree* tree;
    int treeStatus = git_commit_tree(&tree, commit);
    git_commit_free(commit);
    if (treeStatus != GIT_OK)
      return -1;

    git_tree_entry* treeEntry;
    if (git_tree_entry_bypath(&treeEntry, tree, path.c_str()) != GIT_OK) {
      git_tree_free(tree);
      return -1;
    }

    const git_oid* blobSha = git_tree_entry_id(treeEntry);
    if (blobSha != NULL && git_blob_lookup(&blob, repo, blobSha) != GIT_OK)
      blob = NULL;
    git_tree_entry_free(treeEntry);
    git_tree_free(tree);
  }

  if (blob == NULL)
    return -1;

  return 0;
}

// C++ equivalent to GIT_DIFF_OPTIONS_INIT, we can not use it directly because
// of C++'s strong typing.
git_diff_options Repository::CreateDefaultGitDiffOptions() {
  git_diff_options options = { 0 };
  options.version = GIT_DIFF_OPTIONS_VERSION;
  options.context_lines = 3;
  return options;
}

NAN_METHOD(Repository::Exists) {
  NanScope();
  NanReturnValue(NanNew<Boolean>(GetRepository(args) != NULL));
}

NAN_METHOD(Repository::GetPath) {
  NanScope();
  git_repository* repository = GetRepository(args);
  const char* path = git_repository_path(repository);
  NanReturnValue(NanNew<String>(path));
}

NAN_METHOD(Repository::GetWorkingDirectory) {
  NanScope();
  git_repository* repository = GetRepository(args);
  const char* path = git_repository_workdir(repository);
  NanReturnValue(NanNew<String>(path));
}

NAN_METHOD(Repository::GetSubmodulePaths) {
  NanScope();
  git_repository* repository = GetRepository(args);
  std::vector<std::string> paths;
  git_submodule_foreach(repository, SubmoduleCallback, &paths);
  Local<Object> v8Paths = NanNew<Array>(paths.size());
  for (size_t i = 0; i < paths.size(); i++)
    v8Paths->Set(i, NanNew<String>(paths[i].data()));
  NanReturnValue(v8Paths);
}

NAN_METHOD(Repository::GetHead) {
  NanScope();
  git_repository* repository = GetRepository(args);
  git_reference* head;
  if (git_repository_head(&head, repository) != GIT_OK)
    NanReturnNull();

  if (git_repository_head_detached(repository) == 1) {
    const git_oid* sha = git_reference_target(head);
    if (sha != NULL) {
      char oid[GIT_OID_HEXSZ + 1];
      git_oid_tostr(oid, GIT_OID_HEXSZ + 1, sha);
      git_reference_free(head);
      NanReturnValue(NanNew<String>(oid, -1));
    }
  }

  Local<String> referenceName = NanNew<String>(git_reference_name(head));
  git_reference_free(head);
  NanReturnValue(referenceName);
}

NAN_METHOD(Repository::RefreshIndex) {
  NanScope();
  git_repository* repository = GetRepository(args);
  git_index* index;
  if (git_repository_index(&index, repository) == GIT_OK) {
    git_index_read(index, 0);
    git_index_free(index);
  }
  NanReturnUndefined();
}

NAN_METHOD(Repository::IsIgnored) {
  NanScope();
  if (args.Length() < 1)
    NanReturnValue(NanNew<Boolean>(false));

  git_repository* repository = GetRepository(args);
  std::string path(*String::Utf8Value(args[0]));
  int ignored;
  if (git_ignore_path_is_ignored(&ignored,
                                 repository,
                                 path.c_str()) == GIT_OK)
    NanReturnValue(NanNew<Boolean>(ignored == 1));
  else
    NanReturnValue(NanNew<Boolean>(false));
}

NAN_METHOD(Repository::IsSubmodule) {
  NanScope();
  if (args.Length() < 1)
    NanReturnValue(NanNew<Boolean>(false));

  git_index* index;
  git_repository* repository = GetRepository(args);
  if (git_repository_index(&index, repository) == GIT_OK) {
    std::string path(*String::Utf8Value(args[0]));
    const git_index_entry* entry = git_index_get_bypath(index, path.c_str(), 0);
    Handle<Boolean> isSubmodule = NanNew<Boolean>(
        entry != NULL && (entry->mode & S_IFMT) == GIT_FILEMODE_COMMIT);
    git_index_free(index);
    NanReturnValue(isSubmodule);
  } else {
    NanReturnValue(NanNew<Boolean>(false));
  }
}

NAN_METHOD(Repository::GetConfigValue) {
  NanScope();
  if (args.Length() < 1)
    NanReturnNull();

  git_config* config;
  git_repository* repository = GetRepository(args);
  if (git_repository_config(&config, repository) != GIT_OK)
    NanReturnNull();

  std::string configKey(*String::Utf8Value(args[0]));
  const char* configValue;
  if (git_config_get_string(
        &configValue, config, configKey.c_str()) == GIT_OK) {
    Handle<String> configValueHandle = NanNew<String>(configValue);
    git_config_free(config);
    NanReturnValue(configValueHandle);
  } else {
    git_config_free(config);
    NanReturnNull();
  }
}

NAN_METHOD(Repository::SetConfigValue) {
  NanScope();
  if (args.Length() != 2)
    NanReturnValue(NanNew<Boolean>(false));

  git_config* config;
  git_repository* repository = GetRepository(args);
  if (git_repository_config(&config, repository) != GIT_OK)
    NanReturnValue(NanNew<Boolean>(false));

  std::string configKey(*String::Utf8Value(args[0]));
  std::string configValue(*String::Utf8Value(args[1]));

  int errorCode = git_config_set_string(
      config, configKey.c_str(), configValue.c_str());
  git_config_free(config);
  NanReturnValue(NanNew<Boolean>(errorCode == GIT_OK));
}


NAN_METHOD(Repository::GetStatus) {
  NanScope();
  if (args.Length() < 1) {
    Local<Object> result = NanNew<Object>();
    std::map<std::string, unsigned int> statuses;
    git_status_options options = GIT_STATUS_OPTIONS_INIT;
    options.flags = GIT_STATUS_OPT_INCLUDE_UNTRACKED |
                    GIT_STATUS_OPT_RECURSE_UNTRACKED_DIRS;
    if (git_status_foreach_ext(GetRepository(args),
                               &options,
                               StatusCallback,
                               &statuses) == GIT_OK) {
      std::map<std::string, unsigned int>::iterator iter = statuses.begin();
      for (; iter != statuses.end(); ++iter)
        result->Set(NanNew<String>(iter->first.c_str()),
                    NanNew<Number>(iter->second));
    }
    NanReturnValue(result);
  } else {
    git_repository* repository = GetRepository(args);
    std::string path(*String::Utf8Value(args[0]));
    unsigned int status = 0;
    if (git_status_file(&status, repository, path.c_str()) == GIT_OK)
      NanReturnValue(NanNew<Number>(status));
    else
      NanReturnValue(NanNew<Number>(0));
  }
}

NAN_METHOD(Repository::CheckoutHead) {
  NanScope();
  if (args.Length() < 1)
    NanReturnValue(NanNew<Boolean>(false));

  String::Utf8Value utf8Path(args[0]);
  char* path = *utf8Path;

  git_checkout_options options = GIT_CHECKOUT_OPTIONS_INIT;
  options.checkout_strategy = GIT_CHECKOUT_FORCE |
                              GIT_CHECKOUT_DISABLE_PATHSPEC_MATCH;
  git_strarray paths;
  paths.count = 1;
  paths.strings = &path;
  options.paths = paths;

  int result = git_checkout_head(GetRepository(args), &options);
  NanReturnValue(NanNew<Boolean>(result == GIT_OK));
}

NAN_METHOD(Repository::GetReferenceTarget) {
  NanScope();
  if (args.Length() < 1)
    NanReturnNull();

  std::string refName(*String::Utf8Value(args[0]));
  git_oid sha;
  if (git_reference_name_to_id(
        &sha, GetRepository(args), refName.c_str()) == GIT_OK) {
    char oid[GIT_OID_HEXSZ + 1];
    git_oid_tostr(oid, GIT_OID_HEXSZ + 1, &sha);
    NanReturnValue(NanNew<String>(oid, -1));
  } else {
    NanReturnNull();
  }
}

NAN_METHOD(Repository::GetDiffStats) {
  NanScope();

  int added = 0;
  int deleted = 0;
  Local<Object> result = NanNew<Object>();
  result->Set(NanNew<String>("added"), NanNew<Number>(added));
  result->Set(NanNew<String>("deleted"), NanNew<Number>(deleted));

  if (args.Length() < 1)
    NanReturnValue(result);

  git_repository* repository = GetRepository(args);
  git_reference* head;
  if (git_repository_head(&head, repository) != GIT_OK)
    NanReturnValue(result);

  const git_oid* sha = git_reference_target(head);
  git_commit* commit;
  int commitStatus = git_commit_lookup(&commit, repository, sha);
  git_reference_free(head);
  if (commitStatus != GIT_OK)
    NanReturnValue(result);

  git_tree* tree;
  int treeStatus = git_commit_tree(&tree, commit);
  git_commit_free(commit);
  if (treeStatus != GIT_OK)
    NanReturnValue(result);

  String::Utf8Value utf8Path(args[0]);
  char* path = *utf8Path;

  git_diff_options options = CreateDefaultGitDiffOptions();
  git_strarray paths;
  paths.count = 1;
  paths.strings = &path;
  options.pathspec = paths;
  options.context_lines = 0;
  options.flags = GIT_DIFF_DISABLE_PATHSPEC_MATCH;

  git_diff* diffs;
  int diffStatus = git_diff_tree_to_workdir(&diffs, repository, tree, &options);
  git_tree_free(tree);
  if (diffStatus != GIT_OK)
    NanReturnValue(result);

  int deltas = git_diff_num_deltas(diffs);
  if (deltas != 1) {
    git_diff_free(diffs);
    NanReturnValue(result);
  }

  git_patch* patch;
  int patchStatus = git_patch_from_diff(&patch, diffs, 0);
  git_diff_free(diffs);
  if (patchStatus != GIT_OK)
    NanReturnValue(result);

  int hunks = git_patch_num_hunks(patch);
  for (int i = 0; i < hunks; i++) {
    int lines = git_patch_num_lines_in_hunk(patch, i);
    for (int j = 0; j < lines; j++) {
      const git_diff_line* line;
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

  result->Set(NanNew<String>("added"), NanNew<Number>(added));
  result->Set(NanNew<String>("deleted"), NanNew<Number>(deleted));
  NanReturnValue(result);
}

NAN_METHOD(Repository::GetHeadBlob) {
  NanScope();
  if (args.Length() < 1)
    NanReturnNull();

  std::string path(*String::Utf8Value(args[0]));

  git_repository* repo = GetRepository(args);
  git_reference* head;
  if (git_repository_head(&head, repo) != GIT_OK)
    NanReturnNull();

  const git_oid* sha = git_reference_target(head);
  git_commit* commit;
  int commitStatus = git_commit_lookup(&commit, repo, sha);
  git_reference_free(head);
  if (commitStatus != GIT_OK)
    NanReturnNull();

  git_tree* tree;
  int treeStatus = git_commit_tree(&tree, commit);
  git_commit_free(commit);
  if (treeStatus != GIT_OK)
    NanReturnNull();

  git_tree_entry* treeEntry;
  if (git_tree_entry_bypath(&treeEntry, tree, path.c_str()) != GIT_OK) {
    git_tree_free(tree);
    NanReturnNull();
  }

  git_blob* blob = NULL;
  const git_oid* blobSha = git_tree_entry_id(treeEntry);
  if (blobSha != NULL && git_blob_lookup(&blob, repo, blobSha) != GIT_OK)
    blob = NULL;
  git_tree_entry_free(treeEntry);
  git_tree_free(tree);
  if (blob == NULL)
    NanReturnNull();

  const char* content = static_cast<const char*>(git_blob_rawcontent(blob));
  Handle<Value> value = NanNew<String>(content);
  git_blob_free(blob);
  NanReturnValue(value);
}

NAN_METHOD(Repository::GetIndexBlob) {
  NanScope();
  if (args.Length() < 1)
    NanReturnNull();

  std::string path(*String::Utf8Value(args[0]));

  git_repository* repo = GetRepository(args);
  git_index* index;
  if (git_repository_index(&index, repo) != GIT_OK)
    NanReturnNull();

  git_index_read(index, 0);
  const git_index_entry* entry = git_index_get_bypath(index, path.data(), 0);
  if (entry == NULL) {
    git_index_free(index);
    NanReturnNull();
  }

  git_blob* blob = NULL;
  const git_oid* blobSha = &entry->id;
  if (blobSha != NULL && git_blob_lookup(&blob, repo, blobSha) != GIT_OK)
    blob = NULL;
  git_index_free(index);
  if (blob == NULL)
    NanReturnNull();

  const char* content = static_cast<const char*>(git_blob_rawcontent(blob));
  Handle<Value> value = NanNew<String>(content);
  git_blob_free(blob);
  NanReturnValue(value);
}

int Repository::StatusCallback(
    const char* path, unsigned int status, void* payload) {
  std::map<std::string, unsigned int>* statuses =
      static_cast<std::map<std::string, unsigned int>*>(payload);
  statuses->insert(std::make_pair(std::string(path), status));
  return GIT_OK;
}

int Repository::SubmoduleCallback(
    git_submodule* submodule, const char* name, void* payload) {
  std::vector<std::string>* submodules =
      static_cast<std::vector<std::string>*>(payload);
  const char* submodulePath = git_submodule_path(submodule);
  if (submodulePath != NULL)
    submodules->push_back(submodulePath);
  return GIT_OK;
}

NAN_METHOD(Repository::Release) {
  NanScope();
  Repository* repo = node::ObjectWrap::Unwrap<Repository>(args.This());
  if (repo->repository != NULL) {
    git_repository_free(repo->repository);
    repo->repository = NULL;
  }
  NanReturnUndefined();
}

NAN_METHOD(Repository::GetCommitCount) {
  NanScope();
  if (args.Length() < 2)
    NanReturnValue(NanNew<Number>(0));

  std::string fromCommitId(*String::Utf8Value(args[0]));
  git_oid fromCommit;
  if (git_oid_fromstr(&fromCommit, fromCommitId.c_str()) != GIT_OK)
    NanReturnValue(NanNew<Number>(0));

  std::string toCommitId(*String::Utf8Value(args[1]));
  git_oid toCommit;
  if (git_oid_fromstr(&toCommit, toCommitId.c_str()) != GIT_OK)
    NanReturnValue(NanNew<Number>(0));

  git_revwalk* revWalk;
  if (git_revwalk_new(&revWalk, GetRepository(args)) != GIT_OK)
    NanReturnValue(NanNew<Number>(0));

  git_revwalk_push(revWalk, &fromCommit);
  git_revwalk_hide(revWalk, &toCommit);
  git_oid currentCommit;
  int count = 0;
  while (git_revwalk_next(&currentCommit, revWalk) == GIT_OK)
    count++;
  git_revwalk_free(revWalk);
  NanReturnValue(NanNew<Number>(count));
}

NAN_METHOD(Repository::GetMergeBase) {
  NanScope();
  if (args.Length() < 2)
    NanReturnNull();

  std::string commitOneId(*String::Utf8Value(args[0]));
  git_oid commitOne;
  if (git_oid_fromstr(&commitOne, commitOneId.c_str()) != GIT_OK)
    NanReturnNull();

  std::string commitTwoId(*String::Utf8Value(args[1]));
  git_oid commitTwo;
  if (git_oid_fromstr(&commitTwo, commitTwoId.c_str()) != GIT_OK)
    NanReturnNull();

  git_oid mergeBase;
  if (git_merge_base(
        &mergeBase, GetRepository(args), &commitOne, &commitTwo) == GIT_OK) {
    char mergeBaseId[GIT_OID_HEXSZ + 1];
    git_oid_tostr(mergeBaseId, GIT_OID_HEXSZ + 1, &mergeBase);
    NanReturnValue(NanNew<String>(mergeBaseId, -1));
  }

  NanReturnNull();
}

int Repository::DiffHunkCallback(const git_diff_delta* delta,
                                 const git_diff_hunk* range,
                                 void* payload) {
  std::vector<git_diff_hunk>* ranges =
      static_cast<std::vector<git_diff_hunk>*>(payload);
  ranges->push_back(*range);
  return GIT_OK;
}

NAN_METHOD(Repository::GetLineDiffs) {
  NanScope();
  if (args.Length() < 2)
    NanReturnNull();

  std::string text(*String::Utf8Value(args[1]));

  git_repository* repo = GetRepository(args);

  git_blob* blob = NULL;

  int getBlobResult = GetBlob(args, repo, blob);

  if (getBlobResult != 0)
    NanReturnNull();

  std::vector<git_diff_hunk> ranges;
  git_diff_options options = CreateDefaultGitDiffOptions();

  // Set GIT_DIFF_IGNORE_WHITESPACE_EOL when ignoreEolWhitespace: true
  if (args.Length() >= 3) {
    Local<Object> optionsArg(Local<Object>::Cast(args[2]));
    if (optionsArg->Get(NanNew<String>("ignoreEolWhitespace"))->BooleanValue())
      options.flags = GIT_DIFF_IGNORE_WHITESPACE_EOL;
  }

  options.context_lines = 0;
  if (git_diff_blob_to_buffer(blob, NULL, text.data(), text.length(), NULL,
                              &options, NULL, DiffHunkCallback, NULL,
                              &ranges) == GIT_OK) {
    Local<Object> v8Ranges = NanNew<Array>(ranges.size());
    for (size_t i = 0; i < ranges.size(); i++) {
      Local<Object> v8Range = NanNew<Object>();
      v8Range->Set(NanNew<String>("oldStart"),
                   NanNew<Number>(ranges[i].old_start));
      v8Range->Set(NanNew<String>("oldLines"),
                   NanNew<Number>(ranges[i].old_lines));
      v8Range->Set(NanNew<String>("newStart"),
                   NanNew<Number>(ranges[i].new_start));
      v8Range->Set(NanNew<String>("newLines"),
                   NanNew<Number>(ranges[i].new_lines));
      v8Ranges->Set(i, v8Range);
    }
    git_blob_free(blob);
    NanReturnValue(v8Ranges);
  } else {
    git_blob_free(blob);
    NanReturnNull();
  }
}

struct LineDiff {
  git_diff_hunk hunk;
  git_diff_line line;
};

int Repository::DiffLineCallback(const git_diff_delta* delta,
                                 const git_diff_hunk* range,
                                 const git_diff_line* line,
                                 void* payload) {
  LineDiff lineDiff;
  lineDiff.hunk = *range;
  lineDiff.line = *line;
  std::vector<LineDiff> * lineDiffs =
      static_cast<std::vector<LineDiff>*>(payload);
  lineDiffs->push_back(lineDiff);
  return GIT_OK;
}

NAN_METHOD(Repository::GetLineDiffDetails) {
  NanScope();
  if (args.Length() < 2)
    NanReturnNull();

  std::string text(*String::Utf8Value(args[1]));

  git_repository* repo = GetRepository(args);

  git_blob* blob = NULL;

  int getBlobResult = GetBlob(args, repo, blob);

  if (getBlobResult != 0)
    NanReturnNull();

  std::vector<LineDiff> lineDiffs;
  git_diff_options options = CreateDefaultGitDiffOptions();

  // Set GIT_DIFF_IGNORE_WHITESPACE_EOL when ignoreEolWhitespace: true
  if (args.Length() >= 3) {
    Local<Object> optionsArg(Local<Object>::Cast(args[2]));
    if (optionsArg->Get(NanNew<String>("ignoreEolWhitespace"))->BooleanValue())
      options.flags = GIT_DIFF_IGNORE_WHITESPACE_EOL;
  }

  options.context_lines = 0;
  if (git_diff_blob_to_buffer(blob, NULL, text.data(), text.length(), NULL,
                              &options, NULL, NULL, DiffLineCallback,
                              &lineDiffs) == GIT_OK) {
    Local<Object> v8Ranges = NanNew<Array>(lineDiffs.size());
    for (size_t i = 0; i < lineDiffs.size(); i++) {
      Local<Object> v8Range = NanNew<Object>();

      v8Range->Set(NanNew<String>("oldLineNumber"),
                   NanNew<Number>(lineDiffs[i].line.old_lineno));
      v8Range->Set(NanNew<String>("newLineNumber"),
                   NanNew<Number>(lineDiffs[i].line.new_lineno));
      v8Range->Set(NanNew<String>("oldStart"),
                   NanNew<Number>(lineDiffs[i].hunk.old_start));
      v8Range->Set(NanNew<String>("newStart"),
                   NanNew<Number>(lineDiffs[i].hunk.new_start));
      v8Range->Set(NanNew<String>("oldLines"),
                   NanNew<Number>(lineDiffs[i].hunk.old_lines));
      v8Range->Set(NanNew<String>("newLines"),
                   NanNew<Number>(lineDiffs[i].hunk.new_lines));
      v8Range->Set(NanNew<String>("line"),
                   NanNew<String>(lineDiffs[i].line.content,
                                  lineDiffs[i].line.content_len));

      v8Ranges->Set(i, v8Range);
    }
    git_blob_free(blob);
    NanReturnValue(v8Ranges);
  } else {
    git_blob_free(blob);
    NanReturnNull();
  }
}

Handle<Value> Repository::ConvertStringVectorToV8Array(
    const std::vector<std::string>& vector) {
  size_t i = 0, size = vector.size();
  Local<Object> array = NanNew<Array>(size);
  for (i = 0; i < size; i++)
    array->Set(i, NanNew<String>(vector[i].c_str()));

  return array;
}

NAN_METHOD(Repository::GetReferences) {
  NanScope();

  Local<Object> references = NanNew<Object>();
  std::vector<std::string> heads, remotes, tags;

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

  references->Set(NanNew<String>("heads"), ConvertStringVectorToV8Array(heads));
  references->Set(NanNew<String>("remotes"),
                  ConvertStringVectorToV8Array(remotes));
  references->Set(NanNew<String>("tags"), ConvertStringVectorToV8Array(tags));

  NanReturnValue(references);
}

int branch_checkout(git_repository* repo, const char* refName) {
  git_reference* ref = NULL;
  git_object* git_obj = NULL;
  git_checkout_options opts = GIT_CHECKOUT_OPTIONS_INIT;
  opts.checkout_strategy = GIT_CHECKOUT_SAFE;
  int success = -1;

  if (!(success = git_reference_lookup(&ref, repo, refName)) &&
      !(success = git_reference_peel(&git_obj, ref, GIT_OBJ_TREE)) &&
      !(success = git_checkout_tree(repo, git_obj, &opts)))
    success = git_repository_set_head(repo, refName, NULL, NULL);

  git_object_free(git_obj);
  git_obj = NULL;
  git_reference_free(ref);
  ref = NULL;

  return success;
}

NAN_METHOD(Repository::CheckoutReference) {
  NanScope();

  if (args.Length() < 1)
    NanReturnValue(NanNew<Boolean>(false));

  bool shouldCreateNewRef;
  if (args.Length() > 1 && args[1]->BooleanValue())
    shouldCreateNewRef = true;
  else
    shouldCreateNewRef = false;

  std::string strRefName(*String::Utf8Value(args[0]));
  const char* refName = strRefName.c_str();

  git_repository* repo = GetRepository(args);

  if (branch_checkout(repo, refName) == GIT_OK) {
    NanReturnValue(NanNew<Boolean>(true));
  } else if (shouldCreateNewRef) {
    git_reference* head;
    if (git_repository_head(&head, repo) != GIT_OK)
      NanReturnValue(NanNew<Boolean>(false));

    const git_oid* sha = git_reference_target(head);
    git_commit* commit;
    int commitStatus = git_commit_lookup(&commit, repo, sha);
    git_reference_free(head);

    if (commitStatus != GIT_OK)
      NanReturnValue(NanNew<Boolean>(false));

    git_reference* branch;
    // N.B.: git_branch_create needs a name like 'xxx', not 'refs/heads/xxx'
    const int kShortNameLength = strRefName.length() - 11;
    std::string shortRefName(strRefName.c_str() + 11, kShortNameLength);

    int branchCreateStatus = git_branch_create(
        &branch, repo, shortRefName.c_str(), commit, 0, NULL, NULL);
    git_commit_free(commit);

    if (branchCreateStatus != GIT_OK)
      NanReturnValue(NanNew<Boolean>(false));

    git_reference_free(branch);

    if (branch_checkout(repo, refName) == GIT_OK)
      NanReturnValue(NanNew<Boolean>(true));
  }

  NanReturnValue(NanNew<Boolean>(false));
}

NAN_METHOD(Repository::Add) {
  NanScope();

  git_repository* repository = GetRepository(args);
  std::string path(*String::Utf8Value(args[0]));

  git_index* index;
  if (git_repository_index(&index, repository) != GIT_OK) {
    const git_error* e = giterr_last();
    if (e != NULL)
      return NanThrowError(e->message);
    else
      return NanThrowError("Unknown error opening index");
  }
  // Modify the in-memory index.
  if (git_index_add_bypath(index, path.c_str()) != GIT_OK) {
    git_index_free(index);
    const git_error* e = giterr_last();
    if (e != NULL)
      return NanThrowError(e->message);
    else
      return NanThrowError("Unknown error adding path to index");
  }
  // Write this change in the index back to disk, so it is persistent
  if (git_index_write(index) != GIT_OK) {
    git_index_free(index);
    const git_error* e = giterr_last();
    if (e != NULL)
      return NanThrowError(e->message);
    else
      return NanThrowError("Unknown error adding path to index");
  }
  git_index_free(index);
  NanReturnValue(NanNew<Boolean>(true));
}

Repository::Repository(Handle<String> path) {
  NanScope();

  std::string repositoryPath(*String::Utf8Value(path));
  if (git_repository_open_ext(
        &repository, repositoryPath.c_str(), 0, NULL) != GIT_OK)
    repository = NULL;
}

Repository::~Repository() {
  if (repository != NULL) {
    git_repository_free(repository);
    repository = NULL;
  }
}
