# LLM 网关测试验证记录

记录日期：2026-10-17。离线与本地 HTTP 记录保留自 2026-10-15，完整自检和官方 API 验证已于本次更新。

本文件只记录验证方式、实际命令和运行结果。四个模块的职责、十个核心函数的解释和完整代码另见 [LLM 网关模块笔记](LLM网关模块笔记.md)。

## 1. 验证环境

```text
Python 3.10.12
pytest 6.2.5
requests 2.25.1
```

以下命令均在 skeleton 目录中执行。从 HelloCV 仓库根目录进入：

```bash
cd skeleton
```

## 2. 完整自检

执行：

```bash
python3 selfcheck.py
```

实际输出节选：

```text
所有 TODO 都实现完了

开始跑测试 ...

104 passed in 4.58s

测试全绿，恭喜！
```

104 个用例包括仓库原有的 93 个用例、json_guard 模块补充的 7 个用例，以及本地密钥加载的 4 个用例。覆盖 JSON 整理、字段规则、HTTP 请求与重试、网关逐行输出和游戏对局。完整测试中的 HTTP 服务运行在本机回环地址，不消耗 DeepSeek API 额度。

需要单独查看某个模块时可以执行：

```bash
python3 -m pytest tests/test_json_guard.py tests/test_json_guard_extra.py -q
python3 -m pytest tests/test_rules.py -q
python3 -m pytest tests/test_client.py -q
python3 -m pytest tests/test_app.py -q
python3 -m pytest tests/test_config_local.py -q
```

上面五条是按模块或配置功能复查的命令；本次通过结果来自前述完整自检。

## 3. 离线逐行输出

输入两条请求，第二条带有对手上一手 rock：

```bash
printf '%s\n' '{"time":"t1"}' '{"time":"t2","history":["rock"]}' | python3 run_gateway.py --offline --quiet
```

实际标准输出：

```text
{"time":"t1","option":"rock"}
{"time":"t2","option":"paper"}
```

这两行分别是独立 JSON 对象，整体属于 JSON Lines 数据。每个对象只有 time 与 option，time 与输入保持一致。没有历史时出 rock，有历史时出能够赢过上一手的 paper。

标准错误中的处理汇总：

```text
处理 2 行：直接可用 0 / 已修正 0 / 本地兜底 2 / 输入非法 0
```

## 4. 离线六轮对局

执行：

```bash
python3 play_game.py --offline --rounds 6 --strategy cycle
```

实际输出：

```text
对局开始：6 轮，对手策略=cycle，模式=离线兜底

第  1 轮  模型=rock  对手=rock(cycle)  平局  | 本地兜底; 兜底:rock
第  2 轮  模型=paper  对手=scissor(cycle)  模型输  | 本地兜底; 兜底:counter(rock)
第  3 轮  模型=rock  对手=paper(cycle)  模型输  | 本地兜底; 兜底:counter(scissor)
第  4 轮  模型=scissor  对手=rock(cycle)  模型输  | 本地兜底; 兜底:counter(paper)
第  5 轮  模型=paper  对手=scissor(cycle)  模型输  | 本地兜底; 兜底:counter(rock)
第  6 轮  模型=rock  对手=paper(cycle)  模型输  | 本地兜底; 兜底:counter(scissor)

共 6 轮：模型赢 0 / 平 1 / 输 5（其中本地兜底 6 轮）
```

该模式完全使用本地兜底规则。对手按 rock、scissor、paper 循环出拳，备用规则参考的是上一手，因此本次结果不代表对模型能力的评价。

## 5. 本地 HTTP 六轮对局

本次通过真正的本地 HTTP 请求验证 client、json_guard、rules 和 app 的衔接。假上游固定返回合法的 paper 决策，对手仍使用 cycle 策略。

复现命令：

```bash
python3 - <<'PY'
import subprocess
import sys
from mock_llm_server import MockLlmServer

with MockLlmServer(scenarios=['{"time":"{time}","option":"paper"}']) as server:
    subprocess.run([
        sys.executable, "play_game.py",
        "--rounds", "6",
        "--strategy", "cycle",
        "--api-base-url", server.base_url,
        "--api-key", "local-test-key",
    ], check=True)
    print("本地 HTTP 请求数：", len(server.requests))
PY
```

local-test-key 仅用于本机假上游，不是真实 API Key。

实际对局输出：

```text
对局开始：6 轮，对手策略=cycle，模式=deepseek-flash

第  1 轮  模型=paper  对手=rock(cycle)  模型赢  
第  2 轮  模型=paper  对手=scissor(cycle)  模型输  
第  3 轮  模型=paper  对手=paper(cycle)  平局  
第  4 轮  模型=paper  对手=rock(cycle)  模型赢  
第  5 轮  模型=paper  对手=scissor(cycle)  模型输  
第  6 轮  模型=paper  对手=paper(cycle)  平局  

共 6 轮：模型赢 2 / 平 2 / 输 2（其中本地兜底 0 轮）
```

实际 HTTP 请求数为 6。六轮出拳都来自假上游回复，汇总显示本地兜底为 0 轮。输出中的 deepseek-flash 是配置的模型名称，本次实际请求地址为本机回环服务。

## 6. DeepSeek 官方 API 验证：已通过

本次实际调用 DeepSeek 官方接口完成三轮对局，未使用本地兜底。

实际验证时间：2026-10-17T11:12:12.755590+08:00。

原始验证环境的密钥保存在本机原练习项目的私有 .env 文件，权限为 600，并已被 Git 忽略。该文件未随代码发布；在 HelloCV 中复现时需自行设置环境变量或本地 .env。config.py 会按“显式参数 → 环境变量 → skeleton/.env → 项目根目录 .env”的优先级读取密钥，因此在本项目中不需要每次重新 export。

本地文件只支持简单的 KEY=value 配置，不执行 Shell 代码。真实密钥不写入笔记或源码。

本次通过 JsonGateway 和 play_rounds 运行三轮对局，可用下面的命令复现相同流程：

```bash
python3 - <<'PY'
from gateway.app import JsonGateway
from gateway.config import GatewayConfig
from gateway.game import play_rounds

config = GatewayConfig(
    api_base_url="https://api.deepseek.com",
    model="deepseek-flash",
    max_retries=1,
    request_timeout=30,
)
gateway = JsonGateway(config)
try:
    print("DeepSeek 官方 API：三轮实际对局，strategy=cycle")
    result = play_rounds(gateway, rounds=3, strategy="cycle", log=print)
    print(result.summary())
finally:
    gateway.client.close()
PY
```

实际输出：

```text
DeepSeek 官方 API：三轮实际对局，strategy=cycle
第  1 轮  模型=rock  对手=rock(cycle)  平局  
第  2 轮  模型=paper  对手=scissor(cycle)  模型输  
第  3 轮  模型=paper  对手=paper(cycle)  平局  
共 3 轮：模型赢 0 / 平 2 / 输 1（其中本地兜底 0 轮）
```

三轮均产生了官方模型的有效回复，本地兜底为 0 轮。胜、平、负是游戏结果，接口连通性以实际调用成功和没有触发兜底为准。后续对局结果不要求与本次完全相同。

也可以直接使用命令行入口：

```bash
python3 play_game.py --rounds 3 --strategy cycle --api-base-url https://api.deepseek.com --model deepseek-flash
```

本次原始结构化结果保存在 [LLM真实API验证数据.json](LLM真实API验证数据.json)，其中不包含密钥或鉴权头。
