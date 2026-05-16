# H3API 集成说明

> 本文档记录了把 [RoseKavalier/H3API](https://github.com/RoseKavalier/H3API) 集成到 MyMod 的全过程，以及踩过的坑。

## 为什么要集成 H3API

### 集成前

```cpp
// 没有类型，全靠地址 + 偏移
int __stdcall MyHook(HiHook* h, void* self, int arg)
{
    int level = *(int*)((char*)self + 0x44);   // 偏移得查表
    int owner = *(char*)((char*)self + 0x21);
    LOG("hero level=%d owner=%d", level, owner);
}
```

### 集成后

```cpp
#include "../mymod_h3api.h"

int __stdcall MyHook(::HiHook* h, h3::H3Hero* hero)
{
    LOG("hero='%s' lvl=%d owner=%d move=%d/%d",
        hero->name, hero->level, hero->owner,
        hero->movement, hero->maxMovement);
}
```

代码清晰度差距巨大。

## 集成步骤

### 1. Clone 到 third_party

```cmd
cd MyMod
git clone --depth 1 https://github.com/RoseKavalier/H3API.git third_party\H3API
```

### 2. 在 CMakeLists.txt 加 include 路径

```cmake
target_include_directories(MyMod PRIVATE
    src
    third_party
    third_party/H3API/single_header  # ← 新增
)
```

### 3. 创建集成头文件 `src/mymod_h3api.h`

集中管理 H3API 的预处理宏，避免每个 .cpp 都要写一遍。

关键点：
- **必须定义 `_H3API_PATCHER_X86_`**，否则 H3API.hpp 不会包含 patcher_x86 的类定义（只有前向声明）
- 把 `_CRT_SECURE_NO_WARNINGS` 临时撤销避免重定义警告

```cpp
#define _H3API_PATCHER_X86_       // 包含 patcher_x86 类
#define _H3API_STD_CONVERSIONS_   // std::string ↔ H3String
#define _H3API_UNPREFIXED_NAMES_  // h3::Hero 别名

#ifdef _CRT_SECURE_NO_WARNINGS
#define MYMOD_HAD_CRT_SECURE
#undef _CRT_SECURE_NO_WARNINGS
#endif

#include "H3API.hpp"

#ifdef MYMOD_HAD_CRT_SECURE
#define _CRT_SECURE_NO_WARNINGS
#undef MYMOD_HAD_CRT_SECURE
#endif
```

### 4. 删除手写的 `third_party/patcher_x86.hpp`

H3API 自带的版本更完整、参数签名 (`const char*` vs `char*`) 也更对。
两者会冲突，必须只保留一个。

### 5. 改写 hook 代码使用 H3API 类型

旧代码：
```cpp
HiHook* g_hk = nullptr;
g_hk = g_pi->WriteHiHook(0x4ABCDE, SPLICE_, EXTENDED_, THISCALL_, MyHook);
```

新代码（注意 `::HiHook` 显式全局命名空间，避免歧义）：
```cpp
::HiHook* g_hk = nullptr;
g_hk = g_pi->WriteHiHook(0x4ABCDE, SPLICE_, EXTENDED_, THISCALL_, MyHook);
```

## 踩过的坑

### 坑 1: H3API.hpp 里 `Patcher` 只是前向声明

`H3API.hpp` 第 159 行：
```cpp
class Patcher;
class PatcherInstance;
```

完整定义在 `#ifdef _H3API_PATCHER_X86_` 块里（第 34363+ 行）。
**忘了定义这个宏会得到一堆 `使用了未定义类型 Patcher` 的错误**。

### 坑 2: `P_Game` 是宏，已经包含 `h3::` 前缀

错的：
```cpp
auto game = h3::P_Game;   // 展开为 h3::h3::H3Internal::H3Main_ptr() ❌
```

对的：
```cpp
auto game = P_Game;       // 展开为 h3::H3Internal::H3Main_ptr() ✅
```

或者用 `h3::P_Game()` 形式（如果 `_H3API_DONT_USE_MACROS_` 启用了）。

### 坑 3: H3DataPointer 不可拷贝

`P_Game` 返回 `H3DataPointer<H3Main>`（继承 `H3Uncopyable`），不能直接传给 printf。
要先转成裸指针：

```cpp
h3::H3Main* game = P_Game;  // 利用 operator pointer() 隐式转换
LOG("game = %p", game);     // 现在可以
```

### 坑 4: H3API 是基于 H3 SoD 3.2 的

地址不一定对得上 HotA 1.8。具体应对：
- **类的字段布局**：大部分仍然有效（HotA 一般在末尾追加字段）
- **函数地址**：可能变了——需要在 IDA 里验证，或用模式扫描

`hota_addresses.h` 集中维护"已经验证过"的 HotA 1.8 地址。

### 坑 5: `_CRT_SECURE_NO_WARNINGS` 宏重定义

CMake 里已经定义了，H3API.hpp 也要定义，编译时报 C4005 警告。
解决：在 include 前临时 undef，include 后恢复（mymod_h3api.h 已处理）。

## 兼容性矩阵

| 内容 | H3 SoD 3.2 | HotA 1.8 状态 |
|---|---|---|
| `H3Main` 全局对象地址 (0x699538) | ✅ | ⚠️ 待验证 |
| `H3Hero` 字段布局 | ✅ | ✅ 一般兼容 |
| `H3Town` 字段布局 | ✅ | ⚠️ HotA 加了港口/工厂/棱堡相关字段 |
| `H3Hero::CalcMaxMovement` 函数地址 | ✅ | ⚠️ 待验证 |
| `H3CombatManager` | ✅ | ⚠️ HotA 改动较多 |

**经验法则**：
- **读字段**通常没问题
- **调函数**需要验证地址
- **HotA 新增的内容**（港口阵营兵种、棱堡新机制等）H3API 不知道——你得自己逆

## 工作流改进

集成 H3API 后，新写一个 hook 的步骤：

```
1. 在 IDA 里找到要 hook 的函数地址
2. 看函数参数（用 H3API 找对应的类型）
3. 写 hook：
   #include "../mymod_h3api.h"
   int __stdcall MyHook(::HiHook* h, h3::H3SomeClass* obj, int arg)
   {
       auto orig = (origtype)h->GetDefaultFunc();
       // 直接用 obj->fieldName 访问字段
       return orig(obj, arg);
   }
4. 在 Install_XXX() 里 WriteHiHook 注册
```

效率比裸写 `void*` 高 10 倍。

## 推荐学习路径

1. **看 H3API.hpp 中的 H3Hero / H3Town / H3Main 定义**——这是逆向了的核心数据结构。
2. **clone H3Plugins (RoseKavalier 写的 SoD_SP 等插件)**——上百个真实 hook 范例：
   ```cmd
   git clone https://github.com/RoseKavalier/H3Plugins.git
   ```
3. **遇到 H3API 没覆盖的东西**（HotA 新内容）——自己用 IDA 加到 `hota_addresses.h`。

## 参考

- [H3API GitHub](https://github.com/RoseKavalier/H3API)
- [H3API Readme](https://github.com/RoseKavalier/H3API/blob/master/Readme.md)
- [H3Plugins（SoD_SP 等开源插件）](https://github.com/RoseKavalier/H3Plugins)
- 上一级目录的 [HotA二次开发实战指南.md](../../HotA二次开发实战指南.md)
