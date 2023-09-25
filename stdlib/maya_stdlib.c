#include <stdio.h>
#include <stdlib.h>

#include "maya.h"

MayaError maya_alloc(MayaVm* maya) {
    if (maya->sp < 1)
        return ERR_STACK_UNDERFLOW;

    maya->stack[maya->sp - 1].as_ptr = malloc(maya->stack[maya->sp - 1].as_u64);
    return ERR_OK;
}

MayaError maya_free(MayaVm* maya) {
    if (maya->sp < 1)
        return ERR_STACK_UNDERFLOW;

    free(maya->stack[maya->sp - 1].as_ptr);
    maya->sp--;
    return ERR_OK;
}

MayaError maya_print_f64(MayaVm* maya) {
    if (maya->sp < 1)
        return ERR_STACK_UNDERFLOW;

    printf("%lf\n", maya->stack[maya->sp - 1].as_f64);
    maya->sp--;
    return ERR_OK;
}

MayaError maya_print_i64(MayaVm* maya) {
    if (maya->sp < 1)
        return ERR_STACK_UNDERFLOW;

    printf("%ld\n", maya->stack[maya->sp - 1].as_i64);
    maya->sp--;
    return ERR_OK;
}

MayaError maya_print_str(MayaVm* maya) {
    if (maya->sp < 1)
        return ERR_STACK_UNDERFLOW;

    printf("%s\n", (char*)maya->stack[maya->sp - 1].as_ptr);
    maya->sp--;
    return ERR_OK;
}
