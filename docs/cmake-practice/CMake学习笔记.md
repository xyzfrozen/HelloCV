# CMake 学习笔记

## 1. CMake 的作用

一个简单的 C++ 程序可以直接编译：

```bash
g++ main.cpp -o app
```

项目扩大后，需要管理多个源文件、头文件目录、编译标准和第三方库，把这些要求写入 `CMakeLists.txt`，可以减少手动维护复杂编译命令的工作。

CMake 是构建系统生成工具，编译器负责将源码转换成机器代码，Make 或 Ninja 等构建工具负责按依赖关系组织编译和链接。

```text
CMakeLists.txt：描述源码、目标和依赖
          ↓ CMake 配置并生成
Makefile 或 build.ninja：具体构建规则
          ↓ Make 或 Ninja 调用编译器等工具
目标文件、库、可执行程序
```



## 2. 基本语法和变量

CMakeLists 使用 CMake 语言，基本形式为：

```cmake
命令名(参数...)
```

实际编写时，命令名一般使用小写。`#` 表示注释，不需要在每行结尾写分号。

```cmake
set(APP_SOURCES main.cpp helper.cpp)
add_executable(app ${APP_SOURCES})
```

`set()` 保存变量的值，`${APP_SOURCES}` 读取变量的值。这里的多个源文件组成一个列表。变量名应保持大小写一致，包含空格的路径应使用双引号。[set 官方说明](https://cmake.org/cmake/help/latest/command/set.html)



## 3. 版本、项目和 C++ 标准

```cmake
cmake_minimum_required(VERSION 3.16)
project(UnitConverter LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
```

| 语句 | 作用 |
|---|---|
| `cmake_minimum_required(VERSION 3.16)` | 要求 CMake 至少为 3.16，并设置相关兼容策略 |
| `project(UnitConverter LANGUAGES CXX)` | 声明项目名称，启用 C++ 语言并检查编译环境 |
| `set(CMAKE_CXX_STANDARD 17)` | 为后续创建的目标设置 C++17 标准要求 |
| `set(CMAKE_CXX_STANDARD_REQUIRED ON)` | 要求满足该标准，而不允许回退到较旧标准 |
| `set(CMAKE_CXX_EXTENSIONS OFF)` | 禁用 CMake 所管理的编译器语言扩展模式 |

参考：[最低版本要求](https://cmake.org/cmake/help/latest/command/cmake_minimum_required.html)、[project](https://cmake.org/cmake/help/latest/command/project.html)、[C++ 标准设置](https://cmake.org/cmake/help/latest/variable/CMAKE_CXX_STANDARD.html)。



## 4. 目标：可执行程序和库

目标是 CMake 中对一个构建对象的描述，包含它的源码、类型、头文件目录、依赖和编译设置。

```cmake
add_executable(unit_converter src/main.cpp)
add_library(unitlib STATIC src/unitlib.cpp)
```

第一行创建可执行目标，第二行创建静态库目标。

项目名、目标名和源码文件名可以不同：例如项目名是 `UnitConverter`，程序目标名是 `unit_converter`，源码名是 `main.cpp`。

| 库类型 | Ubuntu 下常见文件形式 | 基本作用 |
|---|---|---|
| `STATIC` | `libunitlib.a` | 目标文件的归档，链接时把所需实现加入最终程序 |
| `SHARED` | `libunitlib.so` | 共享库，程序运行时需要能够加载相应库 |

入门示例使用静态库。`unitlib` 是逻辑目标名，`libunitlib.a` 是按平台约定生成的文件名。[add_library 官方说明](https://cmake.org/cmake/help/v3.22/command/add_library.html)



## 5. 拆分主程序和转换库

主程序处理输入、菜单和输出，转换库负责计算。拆分后，修改交互方式通常不需要修改转换公式，其他程序也可以复用转换库。

```text
UnitConverter/
├── CMakeLists.txt
├── src/
│   └── main.cpp
├── unitlib/
│   ├── CMakeLists.txt
│   ├── include/
│   │   └── unitlib.h
│   └── src/
│       └── unitlib.cpp
└── build/                  # 本地构建目录，不提交
```

| 文件 | 职责 |
|---|---|
| `src/main.cpp` | 与用户交互，调用转换函数 |
| `unitlib/include/unitlib.h` | 声明转换函数，提供使用接口 |
| `unitlib/src/unitlib.cpp` | 定义转换函数，提供具体实现 |
| 根目录的 `CMakeLists.txt` | 设置项目、引入子模块、创建主程序、链接库 |
| 库目录的 `CMakeLists.txt` | 描述库的源码和头文件使用要求 |

头文件中的普通函数声明说明“怎么调用”，实现文件中的函数定义说明“具体怎么计算”。

```text
main.cpp → 编译 → main.cpp.o ───────────────┐
                                          ├─ 链接 → unit_converter
unitlib.cpp → 编译 → unitlib.cpp.o → 静态库 ─┘
```

包含 `unitlib.h` 不会自动编译 `unitlib.cpp`。编译器需要找到声明，链接器还需要找到实现。模块化构建就是把这些关系明确写出来。[官方入门教程](https://cmake.org/cmake/help/latest/guide/tutorial/Getting%20Started%20with%20CMake.html)



## 6. 模块构建

以下五个文件共同组成一个最小示例，可用于理解两个 CMakeLists 如何配合。

### 6.1 根目录 CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.16)
project(UnitConverter LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(unitlib)
add_executable(unit_converter src/main.cpp)
target_link_libraries(unit_converter PRIVATE unitlib)
target_compile_definitions(unit_converter PRIVATE SHOW_DETAILS=1)

install(TARGETS unit_converter
    RUNTIME DESTINATION bin
)
```

`add_subdirectory(unitlib)` 处理子目录中的 CMakeLists，定义库目标；`add_executable()` 定义主程序；`target_link_libraries()` 建立主程序使用库的关系。

引入子目录不会自动把库链接到主程序。只有源码中调用了函数，还不足以让 CMake 知道链接依赖，必须明确描述依赖关系。

### 6.2 unitlib/CMakeLists.txt

```cmake
add_library(unitlib STATIC src/unitlib.cpp)

target_include_directories(unitlib PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
```

此处的 `src/unitlib.cpp` 相对于 `unitlib/` 源码目录。`CMAKE_CURRENT_SOURCE_DIR` 也指向当前处理的 `unitlib/` 源码目录，因此后面拼接 `/include` 得到库的头文件目录。

根目录设置的普通变量可以在这个子目录中使用，库也会采用前面设置的 C++ 标准。

### 6.3 unitlib/include/unitlib.h

```cpp
#ifndef UNITLIB_H
#define UNITLIB_H

double celsiusToFahrenheit(double c);

#endif
```

头文件保护避免同一编译单元重复包含该头文件。这里仅声明函数，没有实现计算。

### 6.4 unitlib/src/unitlib.cpp

```cpp
#include"unitlib.h"

double celsiusToFahrenheit(double c)
{
    return c*9.0/5.0+32;
}
```

实现文件也包含自己的头文件，可以帮助检查声明和定义是否匹配。

### 6.5 src/main.cpp

```cpp
#include<iostream>
#include"unitlib.h"

int main()
{
#if SHOW_DETAILS
    std::cout<<"CMake 分模块示例\n";
#endif
    std::cout<<celsiusToFahrenheit(0)<<"\n";
    return 0;
}
```



## 7. 头文件路径、库链接和配置传播

### 7.1 两类配置解决不同问题

| 配置 | 解决的问题 |
|---|---|
| `target_include_directories()` | 编译器在哪里查找头文件？ |
| `target_link_libraries()` | 当前目标依赖哪些库，链接时需要哪些实现？ |

`#include "unitlib.h"` 与链接 `unitlib` 不能互相替代，头文件路径错误通常发生在编译阶段，函数实现缺失通常发生在链接阶段。

### 7.2 PRIVATE、PUBLIC 和 INTERFACE

对于头文件目录、编译宏等使用要求：

| 关键字 | 当前目标使用 | 依赖它的目标继承 |
|---|---|---|
| `PRIVATE` | 是 | 否 |
| `PUBLIC` | 是 | 是 |
| `INTERFACE` | 否 | 是 |

库的头文件目录设置为 `PUBLIC`，因为库自身和调用库的程序都需要看到头文件。

```cmake
target_include_directories(unitlib PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
target_link_libraries(unit_converter PRIVATE unitlib)
```

主程序链接库后，会获得库公开的头文件搜索目录，因此不用重复配置相同目录。

这里的 `PRIVATE` 控制主程序是否向自己的使用者继续公开这项依赖，不阻止主程序接收库的公开配置。

不要把 `PRIVATE` 理解成“最终程序一定不含这个依赖”。例如静态库内部依赖另一个库时，最终程序可能仍需链接它；使用要求的传播与最终所需的链接文件并非完全相同。

参考：[头文件目录说明](https://cmake.org/cmake/help/latest/command/target_include_directories.html)、[链接及依赖传播说明](https://cmake.org/cmake/help/latest/command/target_link_libraries.html)。



## 8. target_compile_definitions：定义编译宏

```cmake
target_compile_definitions(unit_converter PRIVATE SHOW_DETAILS=1)
```

这会为主程序目标的源码提供预处理宏 `SHOW_DETAILS`，在本例中取值为 `1`。C++ 可以用 `#if SHOW_DETAILS` 选择是否编译某段代码。

CMake 变量与 C++ 宏不同：`set(SHOW_DETAILS 1)` 只设置 CMake 变量，不会自动让 C++ 源码获得同名宏；要传入编译宏，需要 `target_compile_definitions()` 等相应配置。

命令参数通常直接写 `SHOW_DETAILS=1`，不需要手写编译器参数的 `-D` 前缀。[官方说明](https://cmake.org/cmake/help/latest/command/target_compile_definitions.html)



## 9. 源码外构建和日常命令

在包含根 CMakeLists 的项目目录中执行：

```bash
cmake -S . -B build
cmake --build build
./build/unit_converter
```

| 参数或命令 | 含义 |
|---|---|
| `-S .` | 当前目录为源码目录 |
| `-B build` | 把生成文件和构建产物放入 build 目录 |
| `cmake --build build` | 调用已经配置的构建工具 |
| `./build/unit_converter` | 运行本例生成的程序 |

源码外构建把生成文件集中放在构建目录中，便于管理源码、使用不同构建配置和排查缓存问题。`build/` 仍可以位于项目目录下，关键是避免把生成文件直接混入源码目录。

学习示例预期输出：

```text
CMake 分模块示例
32
```

任务文档中的传统流程也可以使用，但 `make` 要与 Makefile 生成器配合：

```bash
mkdir -p build
cd build
cmake .. -G "Unix Makefiles"
make
./unit_converter
```

这段流程适用于新构建目录，或已使用同一生成器的目录。已有构建目录不能直接改用另一种生成器，应另外选择一个构建目录。一般使用 `cmake --build` 更方便。

常用命令（以下示例均从项目根目录执行）：

```bash
# 修改源码后，重新构建
cmake --build build

# 查看实际编译、链接命令
cmake --build build --verbose

# 只构建库目标
cmake --build build --target unitlib

# 使用独立目录配置 Debug 构建，适用于 Make/Ninja 等单配置生成器
cmake -S . -B build-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build-debug
```



## 10. find_package：查找外部依赖

```cmake
find_package(Threads REQUIRED)
target_link_libraries(unit_converter PRIVATE Threads::Threads)
```

第一行查找线程支持并读取使用信息，第二行把相应依赖加入目标。此代码是额外的命令示例，单位转换器不需要添加线程依赖。

`REQUIRED` 表示依赖不可缺少，查找失败时停止配置。某些包支持请求版本，例如 `find_package(Foo 1.2 REQUIRED)`；版本接受规则由该包的查找或配置文件决定。

常见查找机制：

- Module 模式：通过 `FindFoo.cmake` 等查找模块寻找依赖。
- Config 模式：读取依赖提供的 `FooConfig.cmake` 或 `foo-config.cmake`。

在需要使用自己安装的依赖时，可以通过 `CMAKE_PREFIX_PATH` 提供安装前缀。它不等同于库的头文件目录，也不意味着给出任意路径都能找到依赖。

[官方说明](https://cmake.org/cmake/help/v3.22/command/find_package.html)



## 11. install：定义和执行安装

根构建文件中的安装规则：

```cmake
install(TARGETS unit_converter
    RUNTIME DESTINATION bin
)
```

意思是把程序安装到安装前缀下的 `bin/`。写下规则不会立即复制文件，配置、构建和安装是不同阶段。

在本例项目根目录构建完成后，执行：

```bash
cmake --install build --prefix "$PWD/install"
./install/bin/unit_converter
```

这会安装到当前项目内的 `install/bin/`，不需要管理员权限。本例只安装静态链接后的程序，没有把转换库和头文件打包成可供其他 CMake 项目查找的软件包。

`RUNTIME` 通常对应可执行程序等运行文件；静态库安装通常使用 `ARCHIVE DESTINATION`，共享库的安装和运行时查找还有额外要求。

[install 官方说明](https://cmake.org/cmake/help/v3.22/command/install.html)



## 12. 常见错误的排查思路

| 阶段或提示 | 常见原因 | 排查方向 |
|---|---|---|
| 找不到 CMakeLists.txt | 源码目录选择错误，或文件名不对 | 检查 `-S` 指向的目录及文件名 |
| Cannot find source file | 源文件路径错误或文件不存在 | 相对于当前 CMakeLists 所在源码目录核对路径 |
| unitlib.h: No such file or directory | 头文件搜索路径缺失 | 检查 include 目录与 PUBLIC 配置 |
| undefined reference | 实现缺失、未链接库，或声明与定义不一致 | 检查库源码、链接关系和函数签名 |
| Could not find a package configuration file | 缺少开发包或配置文件不在搜索路径中 | 检查依赖安装及 CMAKE_PREFIX_PATH |
| CMake 版本低于要求 | 当前 CMake 不满足最低版本 | 查看 `cmake --version`，核对项目实际需要的版本 |
| 构建缓存记录了其他目录或生成器 | 移动了项目，或重复使用不兼容的构建目录 | 新建独立构建目录重新配置 |

排查时先判断出错阶段：配置阶段检查构建规则和环境，编译阶段检查源码和头文件，链接阶段检查实现和库依赖。

阅读输出中第一个关键错误，并用 `--verbose` 查看实际命令。



## 13. 总结

需要掌握的关系可以整理为：

1. `add_subdirectory()` 让 CMake 处理子目录的构建规则。
2. `add_library()` 和 `add_executable()` 创建不同类型的目标。
3. `target_include_directories()` 解决声明所在头文件的查找问题。
4. `target_link_libraries()` 描述库依赖，解决实现的链接问题。
5. `PUBLIC` 等关键字控制使用要求如何传播。
6. `cmake -S ... -B ...` 配置并生成，`cmake --build` 构建，程序路径用于运行。
