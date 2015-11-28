#ifndef ALOS_KMODULE_H
#define ALOS_KMODULE_H

#include "kernel/kelf.h"

//////////////////////////////
//// Module's definitions ////
//////////////////////////////

#define MOD_VER_NUMBER(major, minor, revision) ((major) << 28) | ((minor) << 16) | (revision)

#define MOD_VER_STRING(major, minor, revision) #major "." #minor "-r" #revision

#define MOD_VERSION(major, minor, revision)               \
    int mod_ver = MOD_VER_NUMBER(major, minor, revision); \
    char mod_ver_string[] = MOD_VER_STRING(major, minor, revision);

#define MOD_NAME(name) char mod_name[] = name;

///////////////////////////////////////
//// Module's forward declarations ////
///////////////////////////////////////

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
