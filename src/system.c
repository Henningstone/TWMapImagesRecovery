/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
/* Modified by unsigned char* */
#include "system.h"
#if defined(CONF_FAMILY_UNIX)
	#include <sys/stat.h>
#endif

#if defined(__cplusplus)
extern "C" {
#endif

int fs_is_dir(const char *path)
{
#if defined(CONF_FAMILY_WINDOWS)
	/* TODO: do this smarter */
	WIN32_FIND_DATA finddata;
	HANDLE handle;
	char buffer[1024*2];
	str_format(buffer, sizeof(buffer), "%s/*", path);

	if ((handle = FindFirstFileA(buffer, &finddata)) == INVALID_HANDLE_VALUE)
		return 0;

	FindClose(handle);
	return 1;
#else
	struct stat sb;
	if (stat(path, &sb) == -1)
		return 0;

	if (S_ISDIR(sb.st_mode))
		return 1;
	else
		return 0;
#endif
}

int fs_is_file(const char *filename) // H-Client
{
#if defined(CONF_FAMILY_WINDOWS)
	/* TODO: do this smarter */
	WIN32_FIND_DATA finddata;
	HANDLE handle;

	if ((handle = FindFirstFileA(filename, &finddata)) == INVALID_HANDLE_VALUE)
		return 0;

	FindClose(handle);
	return 1;
#else
  struct stat sb;
  return (stat(filename, &sb) == 0);
#endif
}

#if defined(__cplusplus)
}
#endif
