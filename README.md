# arm64InlineHook
## This is an arm64 inlinehook framwork

- usage
```
void *func_addr = &open;
void *hooked_open_addr = (void *)&hooked_open;
void *orig_func = NULL;
Arm64InlineHook *a64hk = Arm64InlineHook::getInstance();
a64hk->hookFunction(func_addr, hook_func, &orig_func);
```
