/***************************************************************
 *
 * Copyright (C) 2011, University of Nebraska-Lincoln
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

/*
 * lcmaps_mount_under_scratch.c
 * By Brian Bockelman, 2011 
 */

/*****************************************************************************
                            Include header files
******************************************************************************/

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/prctl.h>

#include "lcmaps/lcmaps_modules.h"
#include "lcmaps/lcmaps_cred_data.h"
#include "lcmaps/lcmaps_arguments.h"

#include "mount_under_scratch.h"

#define PLUGIN_ARG "-path"
#define PLUGIN_DEFAULT_1 "/tmp"
#define PLUGIN_DEFAULT_2 "/var/tmp"

// I hate globals, but that's how you have to do it in lcmaps argument parsing.
// Array of strings, the array itself will be null-terminated.
char ** paths = NULL;
const char * logstr = "mount-under-scratch";

/******************************************************************************
Function:   plugin_initialize
Description:
    Initialize plugin; each 
Parameters:
    argc, argv
    argv[0]: the name of the plugin
Returns:
    LCMAPS_MOD_SUCCESS : success
******************************************************************************/
int plugin_initialize(int argc, char **argv)
{

  int idx, idx2=0;
  char * execname;

  // Allocate the global paths array.
  // Takes 2 args per path, plus one for NULL. argv[0] is the plugin name.
  int path_count = (argc+1)/2+1;
  if (path_count < 3) {
    path_count = 3;
  }
  if ((paths = (char**)calloc(path_count, sizeof(char*))) == NULL) {
    lcmaps_log(0, "%s: Unable to allocate memory for paths.\n", logstr);
    return LCMAPS_MOD_FAIL;
  }

  // Notice that we start at 1, as argv[0] is the plugin name.
  for (idx=1; idx<argc; argc++) {
    lcmaps_log_debug(2, "%s: arg %d is %s\n", logstr, idx, argv[idx]);
    if ((strncasecmp(argv[idx], PLUGIN_ARG, sizeof(PLUGIN_ARG)) == 0) && ((idx+1) < argc)) {
      if ((argv[idx+1] != NULL) && (strlen(argv[idx+1]) > 0)) {
        execname = strdup(argv[++idx]);
        if (execname == NULL) {
          lcmaps_log(0, "%s: String allocation error: %s\n", logstr, argv[idx-1]);
          return LCMAPS_MOD_FAIL;
        }
        paths[idx2++] = execname;
        lcmaps_log_debug(2, "%s: Path %d is %s\n", logstr, idx2, execname);
      }
    } else {
      lcmaps_log(0, "%s: Invalid plugin option: %s\n", logstr, argv[idx]);
      return LCMAPS_MOD_FAIL;
    }
  }

  if (idx2 == 0) {
    paths[0] = strdup(PLUGIN_DEFAULT_1);
    paths[1] = strdup(PLUGIN_DEFAULT_2);
    if ((paths[0] == NULL) || (paths[1] == NULL)) {
      lcmaps_log(0, "%s: String allocation error.\n", logstr);
      return LCMAPS_MOD_FAIL;
    }
    paths[2] = NULL;
  }

  return LCMAPS_MOD_SUCCESS;

}


/******************************************************************************
Function:   plugin_introspect
Description:
    return list of required arguments
Parameters:

Returns:
    LCMAPS_MOD_SUCCESS : success
******************************************************************************/
int plugin_introspect(int *argc, lcmaps_argument_t **argv)
{
  static lcmaps_argument_t argList[] = {
    {NULL        ,  NULL    , -1, NULL}
  };

  *argv = argList;
  *argc = lcmaps_cntArgs(argList);

  return LCMAPS_MOD_SUCCESS;
}




/******************************************************************************
Function:   plugin_run
Description:
    Create bind mounts for the glexec'd process
    Heavy lifting is done in C++.
Parameters:
    argc: number of arguments
    argv: list of arguments
Returns:
    LCMAPS_MOD_SUCCESS: authorization succeeded
    LCMAPS_MOD_FAIL   : authorization failed
******************************************************************************/
int plugin_run(int argc, lcmaps_argument_t *argv)
{
  int uid_count;
  uid_t uid;

  uid_count = 0;
  uid_t * uid_array;
  uid_array = (uid_t *)getCredentialData(UID, &uid_count);
  if (uid_count != 1) {
    lcmaps_log(0, "%s: No UID set yet; must map to a UID before running the process tracking module.\n", logstr);
    goto process_tracking_uid_failure;
  }
  uid = uid_array[0];

  mount_under_scratch(uid, paths);

  lcmaps_log(0, "%s: monitor process successfully launched\n", logstr);

  return LCMAPS_MOD_SUCCESS;

process_tracking_uid_failure:
  return LCMAPS_MOD_FAIL;
}

int plugin_verify(int argc, lcmaps_argument_t * argv)
{
    return plugin_run(argc, argv);
}

/******************************************************************************
Function:   plugin_terminate
Description:
    Terminate plugin.  Boilerplate - doesn't do anything
Parameters:

Returns:
    LCMAPS_MOD_SUCCESS : success
******************************************************************************/
int plugin_terminate()
{
  if (paths != NULL) {
    int idx=0;
    while (paths[idx] != NULL) {
      free(paths[idx]);
      idx++;
    }
    free(paths);
    paths
  }
  return LCMAPS_MOD_SUCCESS;
}
