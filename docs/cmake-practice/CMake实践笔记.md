# CMake 实践笔记


## 1. 环境检查

```bash
cmake --version
g++ --version
make --version
uname -sr
```

```text
cmake version 3.22.1
g++ (Ubuntu 11.4.0-1ubuntu1~22.04.3) 11.4.0
GNU Make 4.3
Linux 6.8.0-117-generic
```

本次使用 CMake 3.22.1、G++ 11.4.0 和 C++17。




## 2. 建立目录并划分职责

项目位于 HelloCV 仓库的 UnitConverter 子目录。主体结构为：

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
└── build/                  # 本地构建产物，不提交
```


| 文件 | 职责 |
|---|---|
| [src/main.cpp](../../UnitConverter/src/main.cpp) | 菜单、读取选项和数值、调用函数、输出结果 |
| [unitlib.h](../../UnitConverter/unitlib/include/unitlib.h) | 声明六个转换函数 |
| [unitlib.cpp](../../UnitConverter/unitlib/src/unitlib.cpp) | 实现六个转换函数 |
| [根 CMakeLists](../../UnitConverter/CMakeLists.txt) | 项目设置、引入库、创建程序、链接、安装 |
| [库 CMakeLists](../../UnitConverter/unitlib/CMakeLists.txt) | 创建静态库、设置公开头文件目录 |

主程序负责交互，转换库负责计算，更换交互界面时，可以保留计算函数。



## 3. 编写根目录构建文件

```cmake
cmake_minimum_required(VERSION 3.16)
project(UnitConverter VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(unitlib)

add_executable(unit_converter src/main.cpp)
target_link_libraries(unit_converter PRIVATE unitlib)
target_compile_definitions(unit_converter PRIVATE UNIT_CONVERTER_VERSION="${PROJECT_VERSION}")
target_compile_options(unit_converter PRIVATE -Wall -Wextra -Wpedantic)

install(TARGETS unit_converter RUNTIME DESTINATION bin)
```


| 语句 | 解释 |
|---|---|
| cmake_minimum_required | 设置最低 CMake 版本 |
| project | 声明名称、版本和 C++ 语言 |
| set | 为后续目标设置 C++17 等要求 |
| add_subdirectory | 处理 unitlib 中的构建规则 |
| add_executable | 创建主程序目标 |
| target_link_libraries | 建立主程序使用转换库的关系 |
| target_compile_definitions | 把项目版本传成 C++ 编译宏，用于欢迎信息 |
| target_compile_options | 在本次 GCC/Clang 风格环境中启用编译警告 |
| install | 定义可执行程序的安装规则 |

`PROJECT_VERSION` 是 CMake 中的值。只有通过 `target_compile_definitions()` 等方式传给编译器，C++ 源码才会得到 `UNIT_CONVERTER_VERSION` 宏。[编译宏说明](https://cmake.org/cmake/help/latest/command/target_compile_definitions.html)



## 4. 编写库构建文件和函数接口

```cmake
add_library(unitlib STATIC src/unitlib.cpp)

target_include_directories(unitlib PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
target_compile_options(unitlib PRIVATE -Wall -Wextra -Wpedantic)
```

`STATIC` 创建静态库，本次生成 build/unitlib/libunitlib.a。

`CMAKE_CURRENT_SOURCE_DIR` 在处理库构建文件时指向 unitlib 的源码目录，所以拼接 /include 后得到 unitlib/include。

使用 PUBLIC，因为编译库和编译调用库的主程序都需要找到 unitlib.h。主程序链接库后继承这一公开要求，不需要重复设置相同头文件目录。

主程序链接语句中的 PRIVATE 不阻止主程序接收库公开的配置，它控制主程序是否继续向自己的使用者公开这项依赖。[头文件路径说明](https://cmake.org/cmake/help/latest/command/target_include_directories.html)、[链接依赖说明](https://cmake.org/cmake/help/latest/command/target_link_libraries.html)

`add_subdirectory()` 让 CMake 知道有哪些目标，`target_link_libraries()` 描述这些目标之间的使用关系。

unitlib.h 中的函数声明：

```cpp
#ifndef UNITLIB_H
#define UNITLIB_H

double celsiusToFahrenheit(double c);
double fahrenheitToCelsius(double f);
double metersToFeet(double m);
double feetToMeters(double ft);
double poundsToKilograms(double lb);
double kilogramsToPounds(double kg);

#endif
```



## 5. 实现六个转换函数

```text
华氏度 = 摄氏度 × 1.8 + 32
摄氏度 = (华氏度 - 32) ÷ 1.8
英尺 = 米 ÷ 0.3048
米 = 英尺 × 0.3048
千克 = 磅 × 0.45359237
磅 = 千克 ÷ 0.45359237
```




unitlib.cpp 的实现：

```cpp
#include"unitlib.h"
constexpr double metersPerFoot=0.3048;
constexpr double kilogramsPerPound=0.45359237;

double celsiusToFahrenheit(double c){return c*1.8+32;}
double fahrenheitToCelsius(double f){return (f-32)/1.8;}
double metersToFeet(double m){return m/metersPerFoot;}
double feetToMeters(double ft){return ft*metersPerFoot;}
double poundsToKilograms(double lb){return lb*kilogramsPerPound;}
double kilogramsToPounds(double kg){return kg/kilogramsPerPound;}
```



## 6. 实现菜单和单位换算

菜单在程序开始时显示一次。输入 1～6 选择转换方向，输入 0 退出；每次换算后重新提示选择。

main.cpp 的实现：

```cpp
#include<iostream>
#include<iomanip>
#include"unitlib.h"
using namespace std;

int main()
{
    int n;
    double x;
    cout<<fixed<<setprecision(6);
    cout<<"UnitConverter v"<<UNIT_CONVERTER_VERSION<<"\n"
        <<"1. 摄氏度 → 华氏度\n2. 华氏度 → 摄氏度\n"
        <<"3. 米 → 英尺\n4. 英尺 → 米\n"
        <<"5. 磅 → 千克\n6. 千克 → 磅\n0. 退出\n请选择：";
    while(cin>>n)
    {
        if(n==0)break;
        if(n<1||n>6)
        {
            cout<<"请选择0～6：";
            continue;
        }
        cout<<"请输入数值：";
        if(!(cin>>x))break;
        cout<<"转换结果：";
        if(n==1)cout<<celsiusToFahrenheit(x)<<" 华氏度\n";
        else if(n==2)cout<<fahrenheitToCelsius(x)<<" 摄氏度\n";
        else if(n==3)cout<<metersToFeet(x)<<" 英尺\n";
        else if(n==4)cout<<feetToMeters(x)<<" 米\n";
        else if(n==5)cout<<poundsToKilograms(x)<<" 千克\n";
        else if(n==6)cout<<kilogramsToPounds(x)<<" 磅\n";
        cout<<"请选择：";
    }
    cout<<"程序结束。\n";
    return 0;
}
```

输入结束或读取不到数字时结束程序。输出使用 fixed 和 setprecision(6)，保留六位小数。



## 7. 源码外配置与构建

在 UnitConverter 目录中实际执行：

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build --parallel 2
```

第一条配置并生成构建文件；第二条调用实际构建工具，编译源码、生成静态库并链接主程序。Debug 设置适用于本次使用的单配置生成器。


配置输出：

```text
-- Configuring done
-- Generating done
-- Build files have been written to: /home/xyzfrozen/Documents/联创/HelloCV/UnitConverter/build
```

构建输出：

```text
Consolidate compiler generated dependencies of target unitlib
[ 50%] Built target unitlib
Consolidate compiler generated dependencies of target unit_converter
[ 75%] Building CXX object CMakeFiles/unit_converter.dir/src/main.cpp.o
[100%] Linking CXX executable unit_converter
[100%] Built target unit_converter
```

主要产物：

```text
build/CMakeCache.txt
build/compile_commands.json
build/unitlib/libunitlib.a
build/unit_converter
```

compile_commands.json 记录源码编译命令，便于查看头文件路径、编译标准和宏。



## 8. 功能测试

| 转换方向 | 输入 | 实际显示结果 |
|---|---|---|
| 摄氏度 → 华氏度 | 0 | 32.000000 |
| 华氏度 → 摄氏度 | 212 | 100.000000 |
| 米 → 英尺 | 1 | 3.280840 |
| 英尺 → 米 | 1 | 0.304800 |
| 磅 → 千克 | 1 | 0.453592 |
| 千克 → 磅 | 1 | 2.204623 |
| 摄氏度 → 华氏度 | -40 | -40.000000 |
| 华氏度 → 摄氏度 | -40 | -40.000000 |
| 米 → 英尺 | 1.5 | 4.921260 |
| 千克 → 磅 | 1e2 | 220.462262 |



执行程序：

```bash
./build/unit_converter
```

依次输入三行：选择摄氏度转华氏度，输入 0 摄氏度，再选择退出。

```text
1
0
0
```

```text
UnitConverter v1.0
1. 摄氏度 → 华氏度
2. 华氏度 → 摄氏度
3. 米 → 英尺
4. 英尺 → 米
5. 磅 → 千克
6. 千克 → 磅
0. 退出
请选择：请输入数值：转换结果：32.000000 华氏度
请选择：程序结束。
```



## 9. 本地安装和 Git 忽略

构建完成后，本次把程序安装到项目自身的 install 目录，并再次运行验证：

```bash
cmake --install build --prefix "$PWD/install"
./install/bin/unit_converter
```

安装命令的输出：

```text
-- Install configuration: "Debug"
-- Installing: /home/xyzfrozen/Documents/联创/HelloCV/UnitConverter/install/bin/unit_converter
```

安装后运行程序，输入同样的 `1`、`0`、`0` 三行，结果为：

```text
转换结果：32.000000 华氏度
```

这里仅安装程序，没有把库和头文件发布成可供其他项目查找的 CMake 软件包。


仓库的 .gitignore 增加：

```gitignore
/UnitConverter/build*/
/UnitConverter/install/
```

原有 `/build/` 只匹配仓库根目录的 build 文件夹，不能忽略 UnitConverter 中的嵌套构建目录。通过 git check-ignore 检查了 CMakeCache.txt 和安装后的可执行文件，均被忽略。

在 HelloCV 仓库根目录执行：

```bash
git check-ignore UnitConverter/build/CMakeCache.txt UnitConverter/install/bin/unit_converter
```

输出：

```text
UnitConverter/build/CMakeCache.txt
UnitConverter/install/bin/unit_converter
```



## 10. 总结

1. 根目录的 CMakeLists 组织项目，库目录的 CMakeLists 管理转换库。
2. add_subdirectory 引入子目录的构建规则，target_link_libraries 建立目标之间的链接关系。
3. 头文件提供函数声明，转换库提供函数实现，两者分别参与编译和链接。
4. PUBLIC 让使用库的目标继承公开头文件搜索目录，减少重复配置。
