# arm64InlineHook
## 使用场景
  - 因第三方hook框架中存在兼容性，重写了hook的实现，主要针对arm64平台的函数hook
## 优化点
  - arm64跳转指令优先使用adrp
  - 在不能使用adrp的情况下使用ldr指令，但针对arm64指令乱序执行的情况作了优化

## 使用方法
```
void *func_addr = &open;
void *hooked_open_addr = (void *)&hooked_open;
void *orig_func = NULL;
Arm64InlineHook *a64hk = Arm64InlineHook::getInstance();
a64hk->hookFunction(func_addr, hook_func, &orig_func);
```
