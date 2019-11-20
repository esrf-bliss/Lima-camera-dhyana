#ifndef DHYANA_VERSION_H_
#define DHYANA_VERSION_H_

/// project version as major.minor.patch string
#define DHYANA_VERSION "LimaRoot-19.5.0"
/// project version as integer: major * 10000 + minor * 100 + patch
#define DHYANA_VERSION_INT -189500
#define DHYANA_VERSION_MAJOR LimaRoot-19
#define DHYANA_VERSION_MINOR 5
#define DHYANA_VERSION_PATCH 0
/// package version as string, possibly with git commit: v1.2.3+4+g56789abc
#define DHYANA_PACKAGE_VERSION "L+3+g11a2bed"

///runtime versions, where the above values are linked into a lib and therefore reflect the version
///of the library itself (not the version of the header at compile time of the user code)
const char* dhyana_runtime_project_version();
const char* dhyana_runtime_package_version();
int dhyana_runtime_version_int();
int dhyana_runtime_version_major();
int dhyana_runtime_version_minor();
int dhyana_runtime_version_patch();

///Check consistency of runtime vs compile-time version number. I.e. the header used
///for compilation was from the same version as the linked library.
inline bool dhyana_check_version_consistency(bool major_minor_only)
{
  return dhyana_runtime_version_major() == DHYANA_VERSION_MAJOR &&
         dhyana_runtime_version_minor() == DHYANA_VERSION_MINOR &&
         (major_minor_only ||
          dhyana_runtime_version_patch() == DHYANA_VERSION_PATCH);
}


#endif
