#include "kernel/kmodule.h"
#include "kernel/kprint.h"


MOD_VERSION(0, 1, 1)
MOD_NAME("sample")
MOD_DEPENDS("depmod")

extern void dependency();

int mod_init()
{
	kprint(KPRINT_MSG "sample:init\n");

	dependency();

	return 0;
}

int mod_fini()
{
	dependency();

	kprint(KPRINT_MSG "sample:fini\n");

	return 0;
}
