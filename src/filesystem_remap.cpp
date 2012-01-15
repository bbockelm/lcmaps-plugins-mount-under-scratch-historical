/***************************************************************
 *
 * Copyright (C) 1990-2011, Condor Team, Computer Sciences Department,
 * University of Wisconsin-Madison, WI.
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

#include "filesystem_remap.h"

#include "config.h"

#include <boost/tokenizer.hpp>
#include <string>
#include <syslog.h>
#include <errno.h>
#include <sys/mount.h>

FilesystemRemap::FilesystemRemap() :
	m_mappings(),
	m_mounts_shared()
{
	ParseMountinfo();
}

int FilesystemRemap::AddMapping(std::string source, std::string dest) {
	std::list<pair_strings>::const_iterator it;
	for (it = m_mappings.begin(); it != m_mappings.end(); it++) {
		if ((it->second.length() == dest.length()) && (it->second.compare(dest) == 0)) {
			syslog(LOG_ERR, "Mapping already present for %s.\n", dest.c_str());
			return -1;
		}
	}
	if (CheckMapping(dest)) {
		syslog(LOG_ERR, "Failed to convert shared mount to private mapping");
		return -1;
	}
	m_mappings.push_back( std::pair<std::string, std::string>(source, dest) );
	return 0;
}

int FilesystemRemap::CheckMapping(const std::string & mount_point) {
	bool best_is_shared = false;
	size_t best_len = 0;
	const std::string *best = NULL;

	syslog(LOG_DEBUG, "Checking the mapping of mount point %s.\n", mount_point.c_str());

	for (std::list<pair_str_bool>::const_iterator it = m_mounts_shared.begin(); it != m_mounts_shared.end(); it++) {
		std::string first = it->first;
		if ((strncmp(first.c_str(), mount_point.c_str(), first.size()) == 0) && (first.size() > best_len)) {
			best_len = first.size();
			best = &(it->first);
			best_is_shared = it->second;
		}
	}

	if (!best_is_shared) {
		return 0;
	}

	syslog(LOG_WARNING, "Current mount, %s, is shared.\n", best->c_str());

#if defined(HAVE_DECL_MS_PRIVATE) && HAVE_DECL_MS_PRIVATE
	// Re-mount the mount point as a bind mount, so we can subsequently
	// re-mount it as private.
	if (mount(mount_point.c_str(), mount_point.c_str(), NULL, MS_BIND, NULL)) {	
		syslog(LOG_ERR, "Marking %s as a bind mount failed. (errno=%d, %s)\n", mount_point.c_str(), errno, strerror(errno));
		return -1;
	}

	if (mount(mount_point.c_str(), mount_point.c_str(), NULL, MS_PRIVATE, NULL)) {
		syslog(LOG_ERR, "Marking %s as a private mount failed. (errno=%d, %s)\n", mount_point.c_str(), errno, strerror(errno));
		return -1;
	} else {
		syslog(LOG_ERR, "Marking %s as a private mount successful.\n", mount_point.c_str());
	}
#else
	syslog(LOG_ERR, "Mount, %s, is shared, but MS_PRIVATE flag doesn't exist.\n", best->c_str());
	return -1;
#endif

	return 0;
}

// This is called within the exec
int FilesystemRemap::PerformMappings() {
	int retval = 0;
	std::list<pair_strings>::iterator it;
	for (it = m_mappings.begin(); it != m_mappings.end(); it++) {
		if (strcmp(it->second.c_str(), "/") == 0) {
			if ((retval = chroot(it->first.c_str()))) {
				break;
			}
			if ((retval = chdir("/"))) {
				break;
			}
		} else if ((retval = mount(it->first.c_str(), it->second.c_str(), NULL, MS_BIND, NULL))) {
			break;
		}
	}
	return retval;
}

std::string FilesystemRemap::RemapFile(std::string target) {
	if (target[0] != '/')
		return std::string();
	size_t pos = target.rfind("/");
	if (pos == std::string::npos)
		return target;
	std::string filename = target.substr(pos, target.size() - pos);
	std::string directory = target.substr(0, target.size() - filename.size());
	return RemapDir(directory) + filename;
}

std::string FilesystemRemap::RemapDir(std::string target) {
	if (target[0] != '/')
		return std::string();
	std::list<pair_strings>::iterator it;
	for (it = m_mappings.begin(); it != m_mappings.end(); it++) {
		if ((it->first.compare(0, it->first.length(), target, 0, it->first.length()) == 0)
				&& (it->second.compare(0, it->second.length(), it->first, 0, it->second.length()) == 0)) {
			target.replace(0, it->first.length(), it->second);
		}
	}
	return target;
}

/*
  Sample mountinfo contents (from http://www.kernel.org/doc/Documentation/filesystems/proc.txt):
  36 35 98:0 /mnt1 /mnt2 rw,noatime master:1 - ext3 /dev/root rw,errors=continue
  (1)(2)(3)   (4)   (5)      (6)      (7)   (8) (9)   (10)         (11)

  (1) mount ID:  unique identifier of the mount (may be reused after umount)
  (2) parent ID:  ID of parent (or of self for the top of the mount tree)
  (3) major:minor:  value of st_dev for files on filesystem
  (4) root:  root of the mount within the filesystem
  (5) mount point:  mount point relative to the process's root
  (6) mount options:  per mount options
  (7) optional fields:  zero or more fields of the form "tag[:value]"
  (8) separator:  marks the end of the optional fields
  (9) filesystem type:  name of filesystem of the form "type[.subtype]"
  (10) mount source:  filesystem specific information or "none"
  (11) super options:  per super block options
 */

#define ADVANCE_TOKEN(token, line_str) { \
	if (token++ != tok.end()) { \
		syslog(LOG_ERR, "Invalid line in mountinfo file: %s\n", line_str.c_str()); \
		return; \
	} \
}

#define SHARED_STR "shared:"

void FilesystemRemap::ParseMountinfo() {
	FILE *fd;
	bool is_shared;

	if ((fd = fopen("/proc/self/mountinfo", "r")) == NULL) {
		if (errno == ENOENT) {
			syslog(LOG_DEBUG, "The /proc/self/mountinfo file does not exist; kernel support probably lacking.  Will assume normal mount structure.\n");
		} else {
			syslog(LOG_ERR, "Unable to open the mountinfo file (/proc/self/mountinfo). (errno=%d, %s)\n", errno, strerror(errno));
		}
		return;
	}

	char line_buffer[1024];
	std::string token;

	while (fgets(line_buffer, 1024, fd) != NULL) {
		std::string line_str(line_buffer);
		boost::tokenizer<> tok(line_str);
		boost::tokenizer<>::iterator token = tok.begin();
		ADVANCE_TOKEN(token, line_str) // mount ID
		ADVANCE_TOKEN(token, line_str) // parent ID
		ADVANCE_TOKEN(token, line_str) // major:minor
		ADVANCE_TOKEN(token, line_str) // root
		ADVANCE_TOKEN(token, line_str) // mount point
		std::string mp(*token);
		ADVANCE_TOKEN(token, line_str) // mount options
		ADVANCE_TOKEN(token, line_str) // optional fields
		is_shared = false;
		while (strcmp(token->c_str(), "-") != 0) {
			is_shared = is_shared || (strncmp(token->c_str(), SHARED_STR, strlen(SHARED_STR)) == 0);
			ADVANCE_TOKEN(token, line_str)
		}
		// This seems a bit too chatty - disabling for now.
		// syslog(LOG_DEBUG, "Mount: %s, shared: %d.\n", mp.c_str(), is_shared);
		m_mounts_shared.push_back(pair_str_bool(mp, is_shared));
	}

	fclose(fd);

}

