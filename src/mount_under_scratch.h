
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

int mount_under_scratch(uid_t uid, const char * scratch_dir, char *paths[]);

#ifdef __cplusplus
}
#endif

