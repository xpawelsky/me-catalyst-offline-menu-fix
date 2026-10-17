#ifndef HOOK_H
#define HOOK_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

bool hook_init(void);
bool hook_install(void* target, void* detour, void** original);
void hook_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif /* HOOK_H */

