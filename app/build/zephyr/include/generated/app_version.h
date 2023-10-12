#ifndef _APP_VERSION_H_
#define _APP_VERSION_H_

/*  values come from cmake/version.cmake
 * BUILD_VERSION related  values will be 'git describe',
 * alternatively user defined BUILD_VERSION.
 */

/* #undef ZEPHYR_VERSION_CODE */
/* #undef ZEPHYR_VERSION */

#define APPVERSION          0x1000000
#define APP_VERSION_NUMBER  0x10000
#define APP_VERSION_MAJOR   1
#define APP_VERSION_MINOR   0
#define APP_PATCHLEVEL      0
#define APP_VERSION_STRING  "1.0.0"

#define APP_BUILD_VERSION bcd91214ae9d


#endif /* _APP_VERSION_H_ */
