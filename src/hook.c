#include "hook.h"
#include "minhook/include/MinHook.h"

bool hook_init(void) {
    return (MH_Initialize() == MH_OK);
}

bool hook_install(void* target, void* detour, void** original) {
    if (MH_CreateHook(target, detour, original) != MH_OK) {
        return false;
    }
    if (MH_EnableHook(target) != MH_OK) {
        return false;
    }
    return true;
}

void hook_cleanup(void) {
    MH_Uninitialize();
}

