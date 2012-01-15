
#include <iostream>
#include <sstream>

#include <sys/stat.h>
#include <dirent.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <lcmaps/lcmaps_log.h>

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
  char * last_slash, * parent;
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

int mkdir_and_parents_if_needed(const char *path, mode_t mode, uid_t uid) {

  int rc = 0;

  // UID switching
  uid_t orig_uid = geteuid();
  if ((orig_uid != uid) && seteuid(uid)) {
    lcmaps_log(0, "%s: Unable to set effective UID to %d: (errno=%d) %s.\n", logstr, uid, errno, strerror(errno));
    return -1;
  }

  // Core logic.
  if (mkdir(path, mode)) {
    char * parent;
    if (errno == EEXIST) {
      rc = 0;
    } else if ((errno == ENOTDIR) && ((parent = get_parent(path)) == NULL)) {
      if (mkdir_and_parents_if_needed(parent, mode, uid)) {
        rc = -1;
      } else {
        rc = mkdir(path, mode);
      }
      free(parent);
    } else {
      rc = -1;
    }
  } else {
    rc = 0;
  }

  // Switch back
  if ((orig_uid != uid) && seteuid(orig_uid)) {
    lcmaps_log(0, "%s: Unable to return to previous UID %d: (errno=%d) %s.\n", logstr, orig_uid, errno, strerror(errno));
    abort(); // We are definitely the wrong user and can't fix it.  Instant abort!
  }
  return true;
}

std::string dirscat(const char *dir1, const char *dir2) {
  std::stringstream ss;
  ss << dir1 << "/" << dir2;
  return ss.str();
}

int mount_under_scratch(uid_t uid, const char * working_dir, char *paths[]) {

  FilesystemRemap fs_remap;
  char * path;
  int idx;

  for (idx=0; (paths[idx] != NULL); idx++) {
    path = paths[idx];

    if (!isDirectory(path)) {
      lcmaps_log(0, "%s: Unable to add mapping %s -> %s because %s doesn't exist.\n", logstr, working_dir, path, path);
      return -1;
    }
    std::string full_dir_str = dirscat(working_dir, path);
    if (!mkdir_and_parents_if_needed( full_dir_str.c_str(), S_IRWXU, uid)) {
      lcmaps_log(0, "%s: Failed to create scratch directory %s\n", logstr, full_dir_str.c_str());
      return -1;
    }
    lcmaps_log(4, "%s: Adding mapping: %s -> %s.\n", logstr, full_dir_str.c_str(), path);
    if (fs_remap.AddMapping(full_dir_str, path)) {
      // FilesystemRemap object prints out an error message for us.
      return -1;
    }

  }

  fs_remap.PerformMappings();

  return 0;
}

