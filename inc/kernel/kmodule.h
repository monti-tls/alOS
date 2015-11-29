#ifndef ALOS_KMODULE_H
#define ALOS_KMODULE_H

#include "kernel/kelf.h"

//////////////////////////////
//// Module's definitions ////
//////////////////////////////

//! (PRIVATE) Encode a module's version into the internal format
#define _MOD_VER_NUMBER(major, minor, revision) ((major) << 28) | ((minor) << 16) | (revision)

//! (PRIVATE) Create a version string
#define _MOD_VER_STRING(major, minor, revision) #major "." #minor "-r" #revision

//! (PRIVATE) Get the number of variadic arguments in a macro
#define _MOD_NDEPS(...) (sizeof((void*[]){0, ##__VA_ARGS__})/sizeof(void*) - 1)

//! (PRIVATE) Build an array of any number of strings
#define _MOD_DEPS(list...) { list }

//! Specify the module's version (must be used in all modules)
#define MOD_VERSION(major, minor, revision)               \
    int mod_ver = _MOD_VER_NUMBER(major, minor, revision); \
    char mod_ver_string[] = _MOD_VER_STRING(major, minor, revision);

//! Specify the module's name (must be used in all modules)
#define MOD_NAME(name) char mod_name[] = name;

//! Specify the module's dependencies as a name list (must be set in all modules)
#define MOD_DEPENDS(...) \
    int mod_depends_size = _MOD_NDEPS(__VA_ARGS__); \
    const char* mod_depends[] = _MOD_DEPS(__VA_ARGS__);

///////////////////////////////////////
//// Module's forward declarations ////
///////////////////////////////////////

//! Opaque struct representing a kernel module
typedef struct kmodule kmodule;

/////////////////////////////
//// Public module's API ////
/////////////////////////////

//! Load and initialize a module in the kernel
//! \param name The name of the module
//! \param load_deps Set to 1 if you want to
//!        also load unsatisfied depencies
kmodule* kmodule_insert(const char* name, int load_deps);

//! Remove a module from the kernel
//! \param name The name of the module to remove
//! \param unload_deps Set to 1 if you want to
//!        also unload modules that depends on this one
//! \return 0 if OK, -1 otherwise
int kmodule_remove(const char* name, int unload_deps);

#endif // ALOS_KMODULE_H
