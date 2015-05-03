/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
/* Modified by unsigned char* */
#include "detect.h"

#if defined(__cplusplus)
extern "C" {
#endif

/*
	Function: fs_is_dir
		Checks if directory exists

	Returns:
		Returns 1 on success, 0 on failure.
*/
int fs_is_dir(const char *path);

/*
	Function: fs_is_file
		Check if file exists

	Returns:
		Returns 1 on success, 0 on failure.
*/
int fs_is_file(const char *filename);

#if defined(__cplusplus)
}
#endif
