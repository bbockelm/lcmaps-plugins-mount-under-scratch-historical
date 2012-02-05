/***************************************************************
 *
 * Copyright (C) 2012, University of Nebraska-Lincoln
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you
 * may not use this file except in compliance with the License.  You may
 * obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ***************************************************************/

#include <iostream>
#include <sstream>

#include <sys/stat.h>
#include <dirent.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

extern "C" {
#include <lcmaps/lcmaps_log.h>
}

#include "filesystem_remap.h"
#include "mount_under_scratch.h"

static const char * logstr = "mount-under-scratch";

bool isDirectory(const char * path) {
  DIR * dirp = NULL;
  if ((dirp = opendir(path)) == NULL) {
    lcmaps_log(0, "%s: Unable to stat %s: (errno=%d) %s\n", logstr, path, errno, strerror(errno));
    return false;
  }
  closedir(dirp);
  
  return true;
}

char * get_parent(const char * path) {
  const char * last_slash;
  char * parent;
  if ((last_slash = strrchr(path, '/')) == NULL) {
    return NULL;
  }
  ssize_t len = last_slash - path + 1;
  if ((parent = (char *)malloc(len + 1)) == NULL) {
    return NULL;
  }
  parent[len] = '\0';
  memcpy(parent, path, len);
  return parent;
 
}

int mkdir_and_parents_if_needed(const char *path, mode_t mode, uid_t uid,
    gid_t gid) {

  int rc = 0;

  // Make directory, call this recursively on parent if it doesn't exist.
  if (mkdir(path, mode)) {
    char * parent;
    if (errno == EEXIST) {
      rc = 0;
    } else if ((errno == ENOENT) && ((parent = get_parent(path)) != NULL)) {
      if (mkdir_and_parents_if_needed(parent, mode, uid, gid)) {
        lcmaps_log(5, "%s: Failed to create parent %s.\n", logstr, path);
        rc = -1;
      } else {
        lcmaps_log(5, "%s: Now creating path %s.\n", logstr, path);
        rc = mkdir(path, mode);
      }
      free(parent);
    } else {
      lcmaps_log(5, "%s: Unable to create directory %s: (errno=%d) %s.\n", logstr, path, errno, strerror(errno));
      rc = -1;
    }
  } else {
    lcmaps_log(5, "%s: Successfully created directory %s.\n", logstr, path);
    rc = 0;
  }

  // chown
  if (rc != 0) {
    return rc;
  }

  // We do not need to chown in this case.
  if ((uid == (uid_t)-1) && (gid == (uid_t)-1)) {
    return rc;
  }

  if ((rc = chown(path, uid, gid)) == -1) {
    // Remove this directory, return an appropriate error.
    lcmaps_log(0, "%s: Failed to chown created directory %s: (errno=%d) %s.\n",
      logstr, path, errno, strerror(errno));

    if (rmdir(path) == -1) {
      lcmaps_log(0, "%s: Failed remove unused directory %s: (errno=%d) %s.\n",
        logstr, path, errno, strerror(errno));
    }
  }

  return rc;
}

std::string dirscat(const char *dir1, const char *dir2) {
  std::stringstream ss;
  ss << dir1 << "/" << dir2;
  return ss.str();
}

int mount_under_scratch(uid_t uid, gid_t gid, const char * working_dir,
    char *paths[]) {

  FilesystemRemap fs_remap;
  char * path;
  int idx;

  for (idx=0; (paths[idx] != NULL); idx++) {
    path = paths[idx];

    if (!isDirectory(path)) {
      lcmaps_log(0, "%s: Unable to add mapping %s -> %s because %s doesn't "
        "exist.\n", logstr, working_dir, path, path);
      return -1;
    }
    std::string full_dir_str = dirscat(working_dir, path);
    lcmaps_log(4, "%s: Creating scratch directory %s.\n", logstr,
      full_dir_str.c_str());
    if (mkdir_and_parents_if_needed( full_dir_str.c_str(), S_IRWXU, uid, gid)) {
      lcmaps_log(0, "%s: Failed to create scratch directory %s\n", logstr,
        full_dir_str.c_str());
      return -1;
    }
    lcmaps_log(4, "%s: Adding mapping: %s -> %s.\n", logstr,
      full_dir_str.c_str(), path);
    if (fs_remap.AddMapping(full_dir_str, path)) {
      // FilesystemRemap object prints out an error message for us.
      return -1;
    }

  }

  return fs_remap.PerformMappings();
}

