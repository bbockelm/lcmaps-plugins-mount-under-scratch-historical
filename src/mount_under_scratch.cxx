
#include <sys/stat.h>
#include <dirent.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "filesystem_remap.h"

const char * logstr = "mount-under-scratch";

bool isDirectory(const char * path) {
  DIR * dirp = NULL;
  if ((dirp = opendir(path)) == NULL) {
    syslog(LOG_ERR, "Unable to stat %s: (errno=%d) %s\n", path, errno, strerror(errno));
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
  if ((parent = malloc(len + 1)) == NULL) {
    return NULL;
  }
  parent[len] = '\0';
  memcpy(parent, path, len);
  return parent;
 
}

int mkdir_and_parents_if_needed(const char *path, mode_t mode, uid_t uid) {

  int rc = 0;

  // UID switching
  int orig_uid = geteuid();
  if ((orig_uid != uid) && seteuid(uid)) {
    syslog(LOG_ERR, "Unable to set effective UID to %d: (errno=%d) %s.\n", uid, errno, strerror(errno));
    return -1;
  }

  // Core logic.
  if (mkdir(path, mode)) {
    const char * parent;
    if ((errno == ENOTDIR) && ((parent = get_parent(path)) == NULL)) {
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
    syslog(LOG_ERR, "Unable to return to previous UID %d: (errno=%d) %s.\n", orig_uid, errno, strerror(errno));
    abort(); // We are definitely the wrong user and can't fix it.  Instant abort!
  }
  return true;
}

char * dirscat() {
  return NULL;
}

bool mount_under_scratch(uid_t uid, const char * working_dir, const char *paths[]) {

  // TODO: openlog();

  FilesystemRemap fs_remap;
  const char * path;
  int idx;

  for (idx=0; path[idx] != NULL; idx++) {

    if (!isDirectory(path)) {
      dprintf(D_ALWAYS, "Unable to add mapping %s -> %s because %s doesn't exist.\n", working_dir.c_str(), next_dir, next_dir);
      return false;
    }
    char * full_dir = dirscat(working_dir, next_dir_str);
    if ((full_dir=dirscat(working_dir, next_dir_str)) == NULL) {
      dprintf(D_ALWAYS, "Unable to concatenate %s and %s.\n", working_dir.c_str(), next_dir_str.c_str());
      return false;;
    }

    std::string full_dir_str(full_dir);
    delete [] full_dir; full_dir = NULL;
    if (!mkdir_and_parents_if_needed( full_dir_str.c_str(), S_IRWXU, uid)) {
      dprintf(D_ALWAYS, "Failed to create scratch directory %s\n", full_dir_str.c_str());
      return false;
    }
    dprintf(D_FULLDEBUG, "Adding mapping: %s -> %s.\n", full_dir_str.c_str(), next_dir_str.c_str());
    if (fs_remap->AddMapping(full_dir_str, next_dir_str)) {
      // FilesystemRemap object prints out an error message for us.
      return false;
    }

  }

cleanup:
  // TODO: closelog();
  return true;

}

