#include "project_version.h"
/// project version as major.minor.patch string
const char* dhyana_runtime_project_version(){ return "LimaRoot-19.5.0"; }
/// package version as string, possibly with git commit: v1.2.3+4+g56789abc
const char* dhyana_runtime_package_version(){ return "L+3+g11a2bed"; }
/// project version as integer: major * 10000 + minor * 100 + patch
int dhyana_runtime_version_int()  { return -189500; }
/// project version as integer: major
int dhyana_runtime_version_major(){ return LimaRoot-19; }
/// project version as integer: minor
int dhyana_runtime_version_minor(){ return 5; }
/// project version as integer: patch
int dhyana_runtime_version_patch(){ return 0; }

