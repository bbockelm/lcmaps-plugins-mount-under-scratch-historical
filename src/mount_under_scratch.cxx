
#include <iostream>
#include <sstream>

#include <syslog.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "filesystem_remap.h"

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
    syslog(LOG_ERR, "Unable to set effective UID to %d: (errno=%d) %s.\n", uid, errno, strerror(errno));
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
    syslog(LOG_ERR, "Unable to return to previous UID %d: (errno=%d) %s.\n", orig_uid, errno, strerror(errno));
    abort(); // We are definitely the wrong user and can't fix it.  Instant abort!
  }
  return true;
}

std::string dirscat(const char *dir1, const char *dir2) {
  std::stringstream ss;
  ss << dir1 << "/" << dir2;
  return ss.str();
}

class SyslogManager {

public:
	SyslogManager(const std::string &ident) {openlog(ident.c_str(), 0, LOG_DAEMON);}

	~SyslogManager() {closelog();}

};

bool mount_under_scratch(uid_t uid, const char * working_dir, char *paths[]) {

  SyslogManager sm("mount-under-scratch");

  FilesystemRemap fs_remap;
  char * path;
  int idx;

  for (idx=0; (paths[idx] != NULL); idx++) {
    path = paths[idx];

    if (!isDirectory(path)) {
      syslog(LOG_ERR, "Unable to add mapping %s -> %s because %s doesn't exist.\n", working_dir, path, path);
      return false;
    }
    std::string full_dir_str = dirscat(working_dir, path);
    if (!mkdir_and_parents_if_needed( full_dir_str.c_str(), S_IRWXU, uid)) {
      syslog(LOG_ERR, "Failed to create scratch directory %s\n", full_dir_str.c_str());
      return false;
    }
    syslog(LOG_DEBUG, "Adding mapping: %s -> %s.\n", full_dir_str.c_str(), path);
    if (fs_remap.AddMapping(full_dir_str, path)) {
      // FilesystemRemap object prints out an error message for us.
      return false;
    }

  }

  fs_remap.PerformMappings();

  return true;
}

