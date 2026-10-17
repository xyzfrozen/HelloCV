# HelloCV

哈尔滨工业大学（威海）创新平台任务的学习与实践记录：Ubuntu/Linux、Git、CMake 单位转换器，以及 LLM 石头剪刀布决策网关。

## 学习笔记

| 内容 | 语雀笔记 | 仓库材料 |
|---|---|---|
| CMake 学习内容 | [Cmake-学习内容](https://www.yuque.com/g/xyzfrozen/mcnuvk/dkpc9875qgm26wek/collaborator/join?token=Z9hHADDYexzDoUHo&source=doc_collaborator) | [Markdown 笔记](docs/cmake-practice/CMake学习笔记.md) |
| CMake 实践任务 | [Cmake-实践任务](https://www.yuque.com/g/xyzfrozen/mcnuvk/rknl54skmrhtx821/collaborator/join?token=8h0XgVdFNgMwRyEM&source=doc_collaborator) | [实践笔记](docs/cmake-practice/CMake实践笔记.md)、[转换器源码](UnitConverter/) |
| LLM 任务 | [LLM任务](https://www.yuque.com/g/xyzfrozen/mcnuvk/gvg0nz3rwzxmsuzp/collaborator/join?token=WwoBG4GmdzOmcVd6&source=doc_collaborator) | [模块笔记](docs/llm/LLM网关模块笔记.md)、[测试验证记录](docs/llm/LLM网关测试验证记录.md)、[网关源码](skeleton/) |

以上三篇语雀文档使用作者提供的完整只读协作邀请链接，协作权限为“可阅读”；访问时按语雀页面提示登录。仓库内同时保留 Markdown 笔记，模块讲解与测试验证分别成文，命令和输出使用代码块记录。

此前的 [Linux 学习与实践](https://www.yuque.com/xyzfrozen/mcnuvk/ka6agsl342sammss)、[Git 学习](https://www.yuque.com/xyzfrozen/mcnuvk/gggicdc7nbrhrkpt)、[Git 实践](https://www.yuque.com/xyzfrozen/mcnuvk/utcsciwdap9kb6qg)及[计算器源码](https://gitee.com/xyzfrozen/git_training)仍保留，完整入口见 [NOTES.md](NOTES.md)。

## 目录结构

```text
HelloCV/
├── README.md
├── NOTES.md
├── UnitConverter/              # C++17 主程序和 unitlib 静态库
│   ├── CMakeLists.txt
│   ├── src/main.cpp
│   └── unitlib/
│       ├── CMakeLists.txt
│       ├── include/unitlib.h
│       └── src/unitlib.cpp
├── skeleton/                   # 完成后的 Python LLM 网关
│   ├── gateway/                # 四个核心模块及配置、提示词、游戏逻辑
│   ├── tests/                  # 104 个自动测试
│   ├── samples/requests.jsonl
│   ├── run_gateway.py
│   ├── play_game.py
│   ├── mock_llm_server.py
│   ├── selfcheck.py
│   └── requirements.txt
└── docs/
    ├── cmake-practice/          # CMake 学习、实践笔记和验证脚本
    └── llm/                     # 模块笔记、独立测试记录和 API 验证数据
```

## CMake：单位转换器

支持摄氏度与华氏度、米与英尺、磅与千克六个方向的换算。`unitlib` 负责公式，`main.cpp` 使用 `while(cin>>n)`、`if` 和 `else if` 完成交互。选择 `0` 或结束输入即可退出。

需要 CMake ≥ 3.16、支持 C++17 的编译器以及对应构建工具。以下命令从仓库根目录执行：

```bash
cmake -S UnitConverter -B UnitConverter/build -DCMAKE_BUILD_TYPE=Debug
cmake --build UnitConverter/build --parallel 2
./UnitConverter/build/unit_converter
```

例如依次输入 `1`、`0`、`0`，先将 0 摄氏度换算为 32 华氏度，再退出。

运行 18 个命令行用例，并验证构建、本地安装与产物忽略规则：

```bash
python3 docs/cmake-practice/scripts/verify.py
```

也可单独安装到本地目录：

```bash
cmake --install UnitConverter/build --prefix "$PWD/UnitConverter/install"
./UnitConverter/install/bin/unit_converter
```

## LLM：石头剪刀布决策网关

网关每次读取一行 JSON，调用兼容 Chat Completions 的上游接口，再输出只有 `time` 和 `option` 的一行 JSON。输出时间以请求为准，出拳只能是 `rock`、`scissor` 或 `paper`。上游失败或回复不符合协议时使用本地规则兜底。

| 核心模块 | 功能 |
|---|---|
| `json_guard.py` | 去除代码围栏、提取配平对象、修复常见 JSON 文本问题 |
| `rules.py` | 出拳归一化、字段校验和本地兜底 |
| `client.py` | 请求体构造、HTTP 调用、分类重试和回复提取 |
| `app.py` | 串联请求解析、上游调用、JSON 整理、校验与最终输出 |

### 安装与自检

需要 Python ≥ 3.9。从仓库根目录执行：

```bash
cd skeleton
python3 -m venv .venv
source .venv/bin/activate
python -m pip install -r requirements.txt
python selfcheck.py
```

当前 10 个核心函数已完成，104 个测试通过，包括原有 93 个测试、7 个 JSON 容错补充测试和 4 个本地配置测试。测试会在本机启动 HTTP 服务，不需要真实 API Key。

### 离线运行

以下命令均在 `skeleton/` 中执行：

```bash
printf '%s\n' '{"time":"t1"}' '{"time":"t2","history":["rock"]}' | python run_gateway.py --offline --quiet
python play_game.py --offline --rounds 6 --strategy cycle
```

### 本地模拟上游

一个终端启动服务：

```bash
python mock_llm_server.py --port 8801
```

另一个终端进入 `skeleton/`、激活同一虚拟环境后执行：

```bash
python run_gateway.py --api-base-url http://127.0.0.1:8801 --api-key test --input samples/requests.jsonl --quiet
python play_game.py --rounds 6 --api-base-url http://127.0.0.1:8801 --api-key test
```

### DeepSeek API

在运行程序的同一个终端中读取并导出自己的密钥：

```bash
read -rsp 'DeepSeek API Key: ' DEEPSEEK_API_KEY
printf '\n'
export DEEPSEEK_API_KEY
python play_game.py --rounds 3 --strategy cycle --api-base-url https://api.deepseek.com --model deepseek-flash
```

也支持在 `skeleton/.env` 或仓库根目录 `.env` 中配置 `DEEPSEEK_API_KEY`。读取优先级为显式参数 → 环境变量 → `skeleton/.env` → 仓库根目录 `.env`。私有配置已被 Git 忽略，仓库不包含真实密钥。

2026-10-17 的真实 API 验证完成 3 轮对局，本地兜底 0 轮；结果与命令见[测试验证记录](docs/llm/LLM网关测试验证记录.md)。

## 来源与运行环境

LLM 练习基于 [u5343/traning_repo](https://gitee.com/u5343/traning_repo) 的 `skeleton` 脚手架完成，保留测试与命令行入口，补全四个模块及相关配置支持。原练习的 `example/` 参考目录不在本仓库中。

本次验证环境为 Ubuntu 22.04、CMake 3.22.1、G++ 11.4.0 和 Python 3.10.12。更完整的工具安装记录见 Linux 笔记。
