#include "kernel/kmodule.h"
#include "kernel/kprint.h"
#include "kernel/ksymbols.h"

MOD_VERSION(0, 1, 1)
MOD_NAME("depmod")
MOD_DEPENDS()

void dependency();

int mod_init()
{
    kprint(KPRINT_MSG "depmod:init\n");
    ksymbol_add("dependency", dependency);

    return 0;
}

int mod_fini()
{
    kprint(KPRINT_MSG "depmod:fini\n");

    return 0;
}

void dependency()
{
    kprint(KPRINT_MSG "depmod::dependency\n");
}
