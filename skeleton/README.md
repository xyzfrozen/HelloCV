# 石头剪刀布决策网关

基于 [u5343/traning_repo](https://gitee.com/u5343/traning_repo) 的 skeleton 完成。四个核心模块、十个待实现函数均已补全，当前共 104 个自动测试。

每行输入一个 JSON 对象，例如 `{"time":"t1","history":["rock"]}`；每行输出只有 `time` 和 `option` 的 JSON 对象，例如 `{"time":"t1","option":"paper"}`。`history` 可选，`option` 只允许 `rock`、`scissor`、`paper`。

## 安装与检查

在本目录执行，Python ≥ 3.9：

```bash
python3 -m venv .venv
source .venv/bin/activate
python -m pip install -r requirements.txt
python selfcheck.py
```

## 运行

```bash
printf '%s\n' '{"time":"t1"}' | python run_gateway.py --offline --quiet
python play_game.py --offline --rounds 6 --strategy cycle
```

本地模拟服务使用两个终端，分别执行：

```bash
python mock_llm_server.py --port 8801
```

```bash
python run_gateway.py --api-base-url http://127.0.0.1:8801 --api-key test --input samples/requests.jsonl --quiet
```

真实 API 在运行程序的终端配置自己的密钥：

```bash
read -rsp 'DeepSeek API Key: ' DEEPSEEK_API_KEY
printf '\n'
export DEEPSEEK_API_KEY
python play_game.py --rounds 3 --strategy cycle --api-base-url https://api.deepseek.com --model deepseek-flash
```

也可使用本目录或 HelloCV 根目录中被 Git 忽略的 `.env` 文件。配置读取顺序为显式参数、环境变量、本目录 `.env`、HelloCV 根目录 `.env`。

## 核心模块

| 文件 | 职责 |
|---|---|
| `gateway/json_guard.py` | 剥代码围栏、提取对象、修复 JSON 文本 |
| `gateway/rules.py` | 枚举归一化、校验、兜底 |
| `gateway/client.py` | 构造请求、HTTP 调用与重试、提取 content |
| `gateway/app.py` | 编排逐行输入、上游、容错和输出 |

详细解释和四模块完整代码见[模块笔记](../docs/llm/LLM网关模块笔记.md)，运行结果见[测试验证记录](../docs/llm/LLM网关测试验证记录.md)。完整入口见[仓库 README](../README.md)。
