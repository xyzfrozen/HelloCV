# LLM 网关模块笔记

本笔记按四个模块说明功能，并将十个核心函数的解释放在各自模块中。最后附四个模块的完整代码。密钥由 config.py 从显式参数、环境变量或本地私有 .env 文件读取，实际值不写入源码。运行命令与验证结果另见 [LLM 网关测试验证记录](LLM网关测试验证记录.md)。

网关位于本地程序和 DeepSeek 官方 API 之间。本地程序提供本轮请求，网关负责向模型询问出拳，再把模型回复整理成固定格式的决策。

输入示例：

```json
{"time":"t1","history":["rock","paper"]}
```

输出示例：

```json
{"time":"t1","option":"scissor"}
```

| 字段 | 含义 |
|---|---|
| time | 本轮标识；正常流程以请求中的有效 time 为准 |
| history | 对手过去的出拳，可选；按从旧到新的顺序排列 |
| option | 本轮出拳，只允许 rock、scissor、paper |

输出只保留 time 和 option 两个键。对每个非空输入行，网关写出一行 JSON 决策；空白行会被跳过。

```text
本地请求
   ↓
app.py：接收并组织处理流程
   ↓
client.py：构造 HTTP 请求，调用模型，取回 content
   ↓
json_guard.py：从回复文本中恢复 Python 字典
   ↓
rules.py：校验字段、统一出拳、确定本轮时间
   ↓
app.py：出口检查，写出标准结果
```

模型无法提供可用结果时，由 rules.py 的本地规则提供出拳。网关负责决策，game.py 则负责本地对手、胜负判断和记分，后者属于仓库已经提供的脚手架。



## 1. json_guard.py

负责把模型回复整理成 Python 字典，集中处理围栏、对象提取和 JSON 文本修复。

**strip_code_fence(text)**

输入和输出都是字符串。模型可能把 JSON 放进 Markdown 代码块，这个函数负责取出代码块内部的文本。

````text
输入：
```json
{"option":"rock"}
```

返回：
{"option":"rock"}
````

实现时先用 `text.split("```")` 分割。分割结果中，下标 1、3、5……是围栏内部的片段，对应 Python 切片 `parts[1::2]`。

对每个片段去掉语言标记和首尾空白。有多个片段时，优先返回以 `{` 开头的对象片段；没有对象片段时返回第一个非空片段。原文没有围栏则直接返回原文。

这一步没有执行 JSON 解析，返回值仍是文本。

**find_first_json_object(text)**

从自由文本中找出一个完整的 `{...}`，返回字符串片段；找不到时返回 None。

```text
输入：好的，我出剪刀：{"option":"scissor"}，请继续。
返回：{"option":"scissor"}
```

实现逐字符扫描，维护三个状态：

| 状态 | 作用 |
|---|---|
| depth | 记录对象花括号的嵌套深度 |
| in_string | 当前是否处于双引号字符串中 |
| escaped | 当前字符是否受到前一个反斜杠转义 |

字符串外遇到 `{` 增加深度，遇到 `}` 减少深度，深度回到 0 时截取对象。字符串内部的括号不参与计数，转义双引号也不会结束字符串。

如果一个起点没有配平，会继续尝试后面的 `{`。这个函数寻找结构片段，实际语法是否合法仍由后续 JSON 解析确认。

**repair_json_text(text)**

输入字符串，返回尝试修复后的字符串。它负责恢复常见的 JSON 写法，不负责补造出拳含义。

| 处理内容 | 示例 |
|---|---|
| 删除字符串外的注释 | 删除 // 注释和块注释 |
| 统一引号 | 单引号、中文引号转为 JSON 双引号 |
| 转义裸换行 | 字符串中的实际换行转为转义序列 |
| 统一字面量 | None、True、False 转为 null、true、false |
| 删除尾随逗号 | `{"option":"rock",}` 转为合法对象文本 |
| 补齐结尾 | 对截断的字符串和容器尝试补齐结尾 |

处理时先识别并保留字符串内容。例如网址中的 `//`、文字中的 True、字符串里的逗号都可能是正常数据，不能一律替换。

实现先删除字符串外的注释，再调用 `_normalize_strings()`，随后只替换字符串外的字面量和尾随逗号，最后调用 `_close_unfinished()`。整个过程不执行模型提供的代码，不使用 eval。

修复属于尽力恢复；最终仍要通过 `json.loads()` 确认语法，不能保证任意文字都能变成 JSON。

三个函数由 loads_object 配合使用：收集原文及候选片段，尝试直接解析或修复后解析，最终取得字典。dumps_json 则将对象转成紧凑的 JSON 输出。



## 2. rules.py

负责判断整理后的数据是否符合出拳协议，统一出拳名称、确定时间并提供本地兜底。

**normalize_option(value)**

将同一种出拳的不同写法统一为标准值。

```text
"  ROCK " → "rock"
"Scissors" → "scissor"
"石头" → "rock"
"剪刀" → "scissor"
"布" → "paper"
```

先检查是否为字符串，再执行 `strip().lower()`，最后查询 ALIASES 别名字典。无法识别时返回 None，交给调用方处理。

本仓库的剪刀标准值是 scissor，不能自行改成 scissors。

**validate_decision(candidate, request)**

输入模型候选字典和本轮请求，返回 `(decision, notes)`。decision 是最终决策，notes 是说明列表。

处理顺序：

1. 候选不是字典，转入本地兜底。
2. 通过 normalize_option 统一出拳，无法识别则兜底。
3. 优先采用请求中有效的 time，丢弃模型自行给出的不同时间。
4. 单独调用本函数且请求没有有效 time 时，允许采用模型的有效 time；两者都没有时生成当前时间。
5. 通过 build_decision 重新构造对象，只保留 time 和 option。

```text
请求：{"time":"t1"}
模型候选：{"time":"other","option":"SCISSORS","reason":"说明"}
最终决策：{"time":"t1","option":"scissor"}
```

整条网关流程中，handle_request 会提前补齐或规范化请求的 time，所以正常进入校验时已经有本轮的可信时间。

**fallback_decision(request)**

本地兜底函数，不调用模型，返回 `(decision, notes)`。

有有效对手历史时，选择能赢过对手上一手的出拳；没有历史时选择 rock。时间取请求中的有效 time，否则生成当前时间。

```text
历史最后一手：rock    → 兜底出 paper
历史最后一手：scissor → 兜底出 rock
历史最后一手：paper   → 兜底出 scissor
没有历史             → 兜底出 rock
```

历史通过 history_of 归一化、过滤，并最多保留最近 10 手。这是一条确定性的备用规则，不代表能够保证赢过对手当前这一手。

辅助函数 history_of 提取最近的对手历史，build_decision 构造两个字段的对象，is_valid_decision 检查最终格式，counter_to 查找能赢过指定出拳的选择。



## 3. client.py

负责与上游 API 通信，包括请求体、鉴权头、超时、重试和最终回复提取。

**build_body(request)**

将本地请求组织成 Chat Completions 的 HTTP 请求体，返回 Python 字典。

| 请求字段 | 来源或作用 |
|---|---|
| model | 配置中的模型名称 |
| messages | 系统规则提示词，以及携带本轮请求的用户消息 |
| stream | False，使用完整响应 |
| temperature | 配置中的采样参数 |
| max_tokens | 配置中的输出长度上限 |
| response_format | 启用 JSON 模式时设为 json_object |
| thinking | 按配置启用或关闭思考模式 |

本轮请求通过 json.dumps 序列化后加入用户消息。系统提示词说明游戏规则、JSON 格式和两个输出字段。

JSON 模式约束输出格式，出拳枚举和 time 是否正确仍由本地规则检查。API Key 放在 HTTP 请求头中，不放在提示词里。

**decide(request)**

发送一次决策请求，返回 `(content, meta)`。content 是模型最终回复文本，meta 是请求元信息。

主要过程：

1. 检查密钥和客户端状态。
2. 调用 build_body，并将请求体编码为 UTF-8。
3. 向 endpoint 发起 POST，设置连接超时、读取超时和鉴权头。
4. 成功时通过 extract_content 取出最终回复。
5. 临时 HTTP 状态、网络问题或不可用的回复在次数限制内重试；鉴权和其他不可重试状态立即返回异常给上层。

最多尝试次数为 `max(1, max_retries + 1)`。例如 max_retries 为 2 时，总共最多尝试 3 次。

重试前使用 retry_delay 计算等待时间，包括指数退避、少量随机抖动，以及对 Retry-After 的处理。Session 的 POST 操作通过锁串行保护。

返回元信息包含 attempt、http_status、latency_s、has_reasoning 和 finish_reason。其中 latency_s 是本次发送流程的总耗时，包括重试等待。

**extract_content(response)**

从 HTTP 响应中提取最终回复，返回：

```text
(content, has_reasoning, finish_reason)
```

读取位置是：

```python
data["choices"][0]["message"]["content"]
```

函数检查响应对象、choices、message 和 content 的结构。content 必须是非空字符串；被长度限制截断的回复视为未完成。

reasoning_content 不作为出拳答案使用。函数只记录它是否存在，不将其内容返回为决策。

辅助方法 endpoint 拼接接口地址，build_headers 构造鉴权头，retry_delay 计算等待时间，close 释放 HTTP 会话。



## 4. app.py

负责组织整条处理流程，把模型调用、JSON 整理、规则校验和本地兜底连接起来。

**handle_request(request)**

这是网关的核心编排入口，返回 `(decision, notes)`。

```text
复制本轮请求，补齐或规范化 time
   ↓
在线模式：decide → loads_object → validate_decision
   ↓
尚无可用决策时：fallback_decision
   ↓
is_valid_decision 出口检查
   ↓
返回标准决策和说明
```

复制请求是为了避免修改调用方持有的原始字典。提前确定 time，可以让模型调用、校验和兜底使用同一个本轮标识。

离线模式没有模型客户端，直接使用本地规则。在线流程中的整理或调用异常会被接住，并记录说明，然后转入兜底。出口检查再次确认字段和枚举，必要时生成标准 rock 决策。

handle_line 解析单行请求后调用 handle_request；run 逐行读取、写出并统计；Stats 汇总直接可用、修正、兜底及输入处理数量。

---



**四个模块的完整代码**

以下代码与 skeleton/gateway 中的实际文件一致，包含辅助函数。运行项目还需要仓库中的 config.py、prompts.py 和入口脚本等配套文件。

**json_guard.py**

````python
"""JSON 容错层：把模型返回的**任意文本**整理成 Python 字典。

这是本练习最核心的模块，也是你主要要写的地方。

模型（即使开了 JSON 模式）经常返回这些东西：

* ````` ```json {...} ``` ```` —— 外面套了 markdown 代码块
* ``好的，我出剪刀：{"option": "scissor"}`` —— 前后夹带自然语言
* ``{"option": "rock",}`` —— 尾随逗号
* ``{'option': 'rock'}`` —— 单引号
* ``{"option": None}`` —— Python 的 None / True / False
* ``{"option": "rock"`` —— 少了一个右括号
* 直接摆烂：``抱歉，我无法回答。``

``loads_object`` 的容错阶梯（它调用下面三个核心函数）：

1. 直接 ``json.loads``
2. :func:`strip_code_fence` 剥掉 markdown 围栏
3. :func:`find_first_json_object` 从自由文本里抠出配平的 ``{...}``
4. :func:`repair_json_text` 修复文本，再解析一次
5. 顶层是数组就取第一个对象
6. 全部失败 —— 抛 :class:`JsonGuardError`，由上层走兜底逻辑

**重要原则：容错不等于瞎猜。** 这里只负责「把明显是 JSON 的文本救回来」，
不做语义修补；字段缺失、取值非法是 :mod:`gateway.rules` 的事。

本模块实现的三个核心函数::

    strip_code_fence(text)          -> str          # 剥 markdown 围栏
    find_first_json_object(text)    -> str | None   # 字符串感知的括号配对
    repair_json_text(text)          -> str          # 文本级修复

对应测试：``tests/test_json_guard.py``，补充测试见 ``tests/test_json_guard_extra.py``。
"""

from __future__ import annotations

import json
import re
from typing import Any, Dict, List, Optional

__all__ = [
    "JsonGuardError",
    "strip_code_fence",
    "find_first_json_object",
    "repair_json_text",
    "loads_object",
    "loads_object_or_none",
    "dumps_json",
]


class JsonGuardError(ValueError):
    """无法从给定文本里整理出 JSON 对象。"""


# --------------------------------------------------------------------------- #
# 第 1 步：剥掉 markdown 围栏
# --------------------------------------------------------------------------- #
def strip_code_fence(text: str) -> str:
    """如果文本里有 ```` ``` ```` 围栏，返回围栏**内部**的内容；否则原样返回。

    提示：

    * ``text.split("```")`` 之后，下标 1、3、5… 的片段就是围栏内部；
    * 语言标记行（``json`` / ``JSON`` / ``js``…）要丢掉：判断第一行很短且不含 ``{``；
    * 没写结束围栏（模型被截断）时，取到文本结尾即可（``split`` 天然就是这个效果）；
    * 围栏可能出现在自然语言中间（``好的：```json {...}``` ``），别只看开头；
    * 有多个围栏时，优先返回「第一个以 ``{`` 开头的」，没有就返回第一个非空片段。

    目标：``tests/test_json_guard.py`` 里的 5 个 ``test_strip_code_fence_*`` 通过。
    """
    parts = text.split("```")
    if len(parts) == 1:
        return text

    first = ""
    for block in parts[1::2]:
        # 去掉 json、js 等语言标记，兼容换行和紧接 JSON 的写法。
        block = re.sub(
            r"^[A-Za-z][\w+-]*(?:[ \t]*\r?\n|[ \t]+(?=[{\[]))",
            "",
            block.lstrip(),
        ).strip()
        if not block:
            continue
        if not first:
            first = block
        if block.startswith("{"):
            return block
    return first


# --------------------------------------------------------------------------- #
# 第 2 步：字符串感知的括号配对
# --------------------------------------------------------------------------- #
def find_first_json_object(text: str) -> Optional[str]:
    """从自由文本中找出第一个**括号配平**的 ``{...}``，找不到返回 ``None``。

    提示：

    * 逐字符扫描，维护 ``depth``（花括号深度）、``in_string``、``escaped`` 三个状态；
    * 字符串内部的 ``{`` ``}`` 不参与计数；``\\"`` 不算字符串结束；
    * 从每个 ``{`` 开始尝试，``depth`` 回到 0 就返回 ``text[start:index + 1]``；
    * 某个 ``{`` 一直没配平，就换下一个 ``{`` 继续找；
    * 找不到任何配平对象时返回 ``None``（上层会去走修复或兜底）。

    目标：``tests/test_json_guard.py`` 里的 6 个 ``test_find_first_json_object_*`` 通过。
    """
    for start, char in enumerate(text):
        if char != "{":
            continue
        depth = 0
        in_string = False
        escaped = False
        for index in range(start, len(text)):
            char = text[index]
            if in_string:
                if escaped:
                    escaped = False
                elif char == "\\":
                    escaped = True
                elif char == '"':
                    in_string = False
            elif char == '"':
                in_string = True
            elif char == "{":
                depth += 1
            elif char == "}":
                depth -= 1
                if depth == 0:
                    return text[start:index + 1]
    return None


# --------------------------------------------------------------------------- #
# 已经写好的小工具（修复时用得上，不用改）
# --------------------------------------------------------------------------- #
_PY_LITERAL_RE = re.compile(r"(?<![\w\"'])(None|True|False)(?![\w\"'])")
_PY_LITERALS = {"None": "null", "True": "true", "False": "false"}
_OPEN_TO_CLOSE = {"\u201c": "\u201d", "\u2018": "\u2019"}  # “ -> ” ，‘ -> ’
_ANY_QUOTE = ('"', "'", "\u201c", "\u201d", "\u2018", "\u2019")


def _strip_comments(text: str) -> str:
    """去掉 ``//`` 与 ``/* */`` 注释（字符串内部的斜杠要保留，例如 https://）。"""
    out: List[str] = []
    in_string = False
    escaped = False
    index = 0
    length = len(text)
    while index < length:
        char = text[index]
        if in_string:
            out.append(char)
            if escaped:
                escaped = False
            elif char == "\\":
                escaped = True
            elif char == '"':
                in_string = False
            index += 1
        elif char == '"':
            in_string = True
            out.append(char)
            index += 1
        elif char == "/" and index + 1 < length and text[index + 1] == "/":
            while index < length and text[index] != "\n":
                index += 1
        elif char == "/" and index + 1 < length and text[index + 1] == "*":
            index += 2
            while index + 1 < length and not (text[index] == "*" and text[index + 1] == "/"):
                index += 1
            index += 2
        else:
            out.append(char)
            index += 1
    return "".join(out)


def _normalize_strings(text: str) -> str:
    """把「像字符串」的引号统一成 ``"``，并把字符串里的裸换行转义掉。

    * 不在字符串里遇到 ``'`` / ``“`` / ``”`` 时，都当成字符串开头，输出 ``"``；
    * 字符串内部的中文引号按普通字符保留（否则会把 ``"他说“好”"`` 改坏）；
    * 字符串内部的裸换行 / 制表符转义成 ``\\n`` / ``\\t``。
    """
    out: List[str] = []
    closers: List[str] = []
    escaped = False
    for char in text:
        if closers:  # 在字符串内部
            if escaped:
                out.append(char)
                escaped = False
            elif char == "\\":
                out.append(char)
                escaped = True
            elif char == "\n":
                out.append("\\n")
            elif char == "\r":
                out.append("\\r")
            elif char == "\t":
                out.append("\\t")
            elif char == closers[-1]:
                out.append('"')
                closers.pop()
            else:
                out.append(char)
        else:
            if char in _ANY_QUOTE:
                closers.append(_OPEN_TO_CLOSE.get(char, char))
                out.append('"')
            else:
                out.append(char)
    return "".join(out)


def _close_unfinished(text: str) -> str:
    """补齐缺失的右引号与右括号，并丢掉结尾多余的逗号。"""
    stripped = text.rstrip()
    if stripped.endswith(","):
        stripped = stripped[:-1]
    stack: List[str] = []
    in_string = False
    escaped = False
    for char in stripped:
        if in_string:
            if escaped:
                escaped = False
            elif char == "\\":
                escaped = True
            elif char == '"':
                in_string = False
        elif char == '"':
            in_string = True
        elif char in "{[":
            stack.append(char)
        elif char in "}]":
            if stack and ((char == "}" and stack[-1] == "{") or (char == "]" and stack[-1] == "[")):
                stack.pop()
    suffix = '"' if in_string else ""
    while stack:
        suffix += "}" if stack.pop() == "{" else "]"
    return stripped + suffix


# --------------------------------------------------------------------------- #
# 第 3 步：文本级修复
# --------------------------------------------------------------------------- #
def repair_json_text(text: str) -> str:
    """对「明显是 JSON、但有点小毛病」的文本做修复（不保证一定成功）。

    处理顺序：

    1. 将字符串作为整体保留，只删除字符串外的注释；
    2. ``_normalize_strings`` 统一引号并转义裸换行；
    3. 只在字符串外替换 ``None/True/False``，并删除尾随逗号；
    4. ``_close_unfinished`` 补齐缺失的右引号 / 右括号。

    字符串里的网址、True 和逗号属于内容，不能被当成语法修复。

    目标：``tests/test_json_guard.py`` 里的 9 个 ``test_repair_*`` 通过。
    """
    # 优先匹配整个字符串，防止其中的 //、/* 被误认为注释。
    quoted = (
        r'"(?:\\.|[^"\\])*(?:"|$)'
        r"|'(?:\\.|[^'\\])*(?:'|$)"
        r'|“(?:\\.|[^”\\])*(?:”|$)'
        r'|‘(?:\\.|[^’\\])*(?:’|$)'
    )
    text = re.sub(
        quoted + r"|//[^\n]*|/\*[\s\S]*?(?:\*/|$)",
        lambda match: match.group() if match.group()[0] in _ANY_QUOTE else " ",
        text,
        flags=re.DOTALL,
    )
    text = _normalize_strings(text).strip()

    def replace_token(match: re.Match) -> str:
        token = match.group()
        if token.startswith('"'):
            return token
        return _PY_LITERALS.get(token, "")

    text = re.sub(
        r'"(?:\\.|[^"\\])*(?:"|$)|' + _PY_LITERAL_RE.pattern + r"|,\s*(?=[}\]])",
        replace_token,
        text,
        flags=re.DOTALL,
    )
    return _close_unfinished(text.rstrip().rstrip(",").rstrip())


# --------------------------------------------------------------------------- #
# 编排：容错阶梯（已写好，不用改）
# --------------------------------------------------------------------------- #
def _candidate_texts(text: str) -> List[str]:
    """按「原文 -> 围栏内 -> 自由文本里抠出来的对象」收集候选片段。"""
    candidates: List[str] = [text.strip()]
    fenced = strip_code_fence(text).strip()
    if fenced and fenced not in candidates:
        candidates.append(fenced)
    found = find_first_json_object(fenced or text)
    if found and found not in candidates:
        candidates.append(found)
    found_raw = find_first_json_object(text)
    if found_raw and found_raw not in candidates:
        candidates.append(found_raw)
    return candidates


def _pick_object(parsed: Any) -> Optional[Dict[str, Any]]:
    """顶层是对象就返回；是数组就取第一个对象；其它返回 ``None``。"""
    if isinstance(parsed, dict):
        return parsed
    if isinstance(parsed, list):
        for item in parsed:
            if isinstance(item, dict):
                return item
    return None


def loads_object(text: Any) -> Dict[str, Any]:
    """把任意模型输出整理成 dict，失败抛 :class:`JsonGuardError`。"""
    if isinstance(text, dict):
        return text
    if text is None:
        raise JsonGuardError("模型返回内容为 None")
    if not isinstance(text, str):
        raise JsonGuardError("模型返回内容类型是 %s，期望 str" % type(text).__name__)
    if not text.strip():
        raise JsonGuardError("模型返回内容为空")

    seen: set = set()
    errors: List[str] = []
    for candidate in _candidate_texts(text):
        if not candidate:
            continue
        for variant in (candidate, repair_json_text(candidate)):
            if not variant or variant in seen:
                continue
            seen.add(variant)
            try:
                parsed = json.loads(variant)
            except Exception as exc:  # noqa: BLE001 - 逐级降级，失败就试下一个
                errors.append("%s: %s" % (type(exc).__name__, exc))
                continue
            picked = _pick_object(parsed)
            if picked is not None:
                return picked
            errors.append("顶层 JSON 不是对象")
    detail = errors[-1] if errors else "没找到 JSON 对象"
    raise JsonGuardError("无法从模型输出中整理出 JSON 对象（%s）" % detail)


def loads_object_or_none(text: Any) -> Optional[Dict[str, Any]]:
    """容错版 :func:`loads_object`：失败返回 ``None`` 而不是抛异常。"""
    try:
        return loads_object(text)
    except JsonGuardError:
        return None


def dumps_json(obj: Any) -> str:
    """网关统一出口：紧凑 JSON、保留中文、拒绝 NaN。"""
    return json.dumps(obj, ensure_ascii=False, separators=(",", ":"), allow_nan=False)
````

**rules.py**

````python
"""决策字段的解析、规范化与兜底。

约定的协议（一行进、一行出）::

    输入：{"time": "<本轮时间>", "history": ["rock", "paper", ...]}   # history 可选
    输出：{"time": "<与输入一致>", "option": "rock|scissor|paper"}    # 只允许这两个键

分层原则：

* :func:`parse_request` / :func:`normalize_time` —— 「读」：把外部输入变成可信的 Python 对象；
* :func:`normalize_option` —— 「整理」：把模型可能写歪的取值（大写、复数、中文）拉回枚举；
* :func:`validate_decision` / :func:`fallback_decision` —— 「兜底」：任何情况下都能给出合法决策；
* :func:`build_decision` / :func:`is_valid_decision` —— 「出口闸门」：只放行 {time, option}。

**网关是可信边界**：``time`` 一律以网关收到/生成的时间为准，不采信模型自己写的 time。

本模块实现的三个核心函数::

    normalize_option(value)                      -> str | None
    validate_decision(candidate, request)        -> (决策, 说明列表)
    fallback_decision(request)                   -> (决策, 说明列表)

对应测试：``tests/test_rules.py`` 的「整理」「校验与兜底」两节。
"""

from __future__ import annotations

import json
from datetime import datetime, timezone
from typing import Any, Dict, List, Optional, Tuple

__all__ = [
    "OPTIONS",
    "BEATS",
    "ALIASES",
    "now_iso",
    "normalize_time",
    "parse_request",
    "get_time",
    "history_of",
    "normalize_option",
    "build_decision",
    "is_valid_decision",
    "validate_decision",
    "fallback_decision",
    "judge",
    "counter_to",
]

#: 合法出拳（必须是这三个小写英文，顺序固定，方便测试）
OPTIONS: Tuple[str, ...] = ("rock", "scissor", "paper")

#: 谁赢谁：key 打败 value
BEATS: Dict[str, str] = {"rock": "scissor", "scissor": "paper", "paper": "rock"}

#: 模型可能写出来的同义写法 -> 标准值（已经给你了，直接用）
ALIASES: Dict[str, str] = {
    "rock": "rock",
    "rocks": "rock",
    "石头": "rock",
    "拳头": "rock",
    "锤子": "rock",
    "scissor": "scissor",
    "scissors": "scissor",
    "剪刀": "scissor",
    "paper": "paper",
    "papers": "paper",
    "布": "paper",
}

_MAX_HISTORY = 10


# --------------------------------------------------------------------------- #
# 读：输入解析（已写好，不用改）
# --------------------------------------------------------------------------- #
def now_iso() -> str:
    """当前时间（带时区的 ISO8601），输入缺 time 时由网关自己补。"""
    return datetime.now(timezone.utc).isoformat()


def normalize_time(value: Any) -> Optional[str]:
    """把 time 字段拉成非空字符串；无法识别返回 ``None``。"""
    if isinstance(value, bool) or value is None:
        return None
    if isinstance(value, str):
        cleaned = value.strip()
        if not cleaned or len(cleaned) > 64:
            return None
        return cleaned
    if isinstance(value, (int, float)):
        try:
            return datetime.fromtimestamp(float(value), tz=timezone.utc).isoformat()
        except (OverflowError, OSError, ValueError):
            return None
    return None


def parse_request(raw: Any) -> Dict[str, Any]:
    """解析一行请求：``str`` 走 ``json.loads``，``dict`` 直接透传，其余抛 ``ValueError``。"""
    if isinstance(raw, dict):
        return raw
    if not isinstance(raw, str) or not raw.strip():
        raise ValueError("请求为空")
    parsed = json.loads(raw)
    if not isinstance(parsed, dict):
        raise ValueError("请求顶层必须是 JSON 对象")
    return parsed


def get_time(request: Dict[str, Any]) -> Optional[str]:
    """取请求里的 time（缺失或非法返回 ``None``）。"""
    if not isinstance(request, dict):
        return None
    return normalize_time(request.get("time"))


def history_of(request: Dict[str, Any]) -> List[str]:
    """取对手历史出拳：只保留能识别的元素，最多保留最近 10 个。"""
    if not isinstance(request, dict):
        return []
    raw = request.get("history")
    if not isinstance(raw, list):
        return []
    history: List[str] = []
    for item in raw:
        option = normalize_option(item)
        if option is not None:
            history.append(option)
    return history[-_MAX_HISTORY:]


# --------------------------------------------------------------------------- #
# 第 4 步：枚举归一化
# --------------------------------------------------------------------------- #
def normalize_option(value: Any) -> Optional[str]:
    """把模型给的取值拉回 ``OPTIONS``；无法识别返回 ``None``。

    提示：

    * 非字符串直接返回 ``None``（``42`` / ``None`` / ``["rock"]`` 都不算有效出拳）；
    * ``value.strip().lower()`` 之后查 :data:`ALIASES`：
      这样 ``"  ROCK "`` -> ``rock``、``"Scissors"`` -> ``scissor``、``"石头"`` -> ``rock``。

    目标：``tests/test_rules.py`` 的 ``test_normalize_option_*`` 与 ``test_aliases_*`` 通过。
    """
    if not isinstance(value, str):
        return None
    return ALIASES.get(value.strip().lower())


# --------------------------------------------------------------------------- #
# 出口闸门（已写好，不用改）
# --------------------------------------------------------------------------- #
def build_decision(time_value: Any, option: Any) -> Dict[str, Any]:
    """按协议构造决策对象：**只有** time 与 option 两个键，多余字段一律不带出网关。"""
    return {"time": str(time_value), "option": str(option)}


def is_valid_decision(payload: Any) -> bool:
    """出口自检：必须是恰好含 time / option 两个键的 JSON 对象，且 option 合法。"""
    if not isinstance(payload, dict):
        return False
    if set(payload.keys()) != {"time", "option"}:
        return False
    if not isinstance(payload["time"], str) or not payload["time"]:
        return False
    return payload["option"] in OPTIONS


# --------------------------------------------------------------------------- #
# 第 5 步：校验与修正
# --------------------------------------------------------------------------- #
def validate_decision(candidate: Any, request: Dict[str, Any]) -> Tuple[Dict[str, Any], List[str]]:
    """把模型输出修正成合法决策，返回 ``(决策, 修正说明列表)``，**永不抛异常**。

    提示：

    1. ``candidate`` 不是 dict -> 直接 ``return fallback_decision(request)``，
       并在说明里加上 ``"非对象->兜底"``；
    2. ``normalize_option(candidate.get("option"))`` 得到 ``None`` -> 同样走兜底，
       说明里加上 ``"option 非法->兜底"``；
    3. time 以请求为准（网关是可信边界）：
       * 请求里有 time -> 用它；若模型也写了不同的 time，说明里加一句 "丢弃模型的 time"；
       * 请求里没有 -> 用模型给的 time（说明 "请求无 time->用模型的 time"）；
       * 都没有 -> ``now_iso()``（说明 "请求无 time->用当前时间"）；
    4. 最后 ``build_decision(time_value, option)`` 返回，保证只有两个键。

    注意 ``fallback_decision`` 返回的是 ``(决策, 说明)`` 元组，别把元组当决策返回。

    目标：``tests/test_rules.py`` 的 ``test_validate_decision_*`` 通过。
    """
    if not isinstance(candidate, dict):
        decision, notes = fallback_decision(request)
        return decision, ["非对象->兜底"] + notes

    option = normalize_option(candidate.get("option"))
    if option is None:
        decision, notes = fallback_decision(request)
        return decision, ["option 非法->兜底"] + notes

    notes: List[str] = []
    if candidate.get("option") != option:
        notes.append("option 已归一化")
    if set(candidate) - {"time", "option"}:
        notes.append("丢弃多余字段")

    time_value = get_time(request)
    model_time = normalize_time(candidate.get("time"))
    if time_value is not None:
        if "time" in candidate and model_time != time_value:
            notes.append("丢弃模型的 time")
    elif model_time is not None:
        time_value = model_time
        notes.append("请求无 time->用模型的 time")
    else:
        time_value = now_iso()
        notes.append("请求无 time->用当前时间")
    return build_decision(time_value, option), notes


# --------------------------------------------------------------------------- #
# 第 6 步：本地兜底
# --------------------------------------------------------------------------- #
def counter_to(option: str) -> str:
    """能赢过 ``option`` 的出拳（已写好，直接用）。"""
    for candidate, beaten in BEATS.items():
        if beaten == option:
            return candidate
    raise ValueError("未知出拳：%r" % (option,))


def fallback_decision(request: Dict[str, Any]) -> Tuple[Dict[str, Any], List[str]]:
    """不依赖模型的本地兜底决策：任何输入都要给出合法决策。

    提示：

    * ``history = history_of(request)``；有历史就出 ``counter_to(history[-1])``
      （对手上一手是 X，就出能赢 X 的拳），说明里写 ``"兜底:counter(X)"``；
    * 没有历史就出 ``"rock"``，说明里写 ``"兜底:rock"``；
    * time 用 ``get_time(request) or now_iso()``；
    * 返回 ``(build_decision(time_value, option), [说明])``。

    目标：``tests/test_rules.py`` 的 ``test_fallback_decision_*`` 通过。
    """
    history = history_of(request)
    time_value = get_time(request) or now_iso()
    if history:
        option = counter_to(history[-1])
        note = "兜底:counter(%s)" % history[-1]
    else:
        option = "rock"
        note = "兜底:rock"
    return build_decision(time_value, option), [note]


# --------------------------------------------------------------------------- #
# 裁判（已写好，不用改）
# --------------------------------------------------------------------------- #
def judge(mine: str, theirs: str) -> int:
    """比较两手：``1`` 我赢，``-1`` 我输，``0`` 平局。"""
    if mine == theirs:
        return 0
    return 1 if BEATS.get(mine) == theirs else -1
````

**client.py**

````python
"""上游 API 转发层：构造请求、发送、重试、取出答案。

这一层要解决的真实问题：

* **鉴权**：``Authorization: Bearer <key>``，key 只从配置/环境变量来，绝不写死；
* **请求格式**：OpenAI 兼容的 ``/chat/completions``，开 ``response_format={"type":"json_object"}``
  让上游保证返回合法 JSON，同时默认关掉思考模式（``thinking={"type":"disabled"}``）省时间；
* **重试**：网络错误、429、5xx、**空回复**都值得退避重试；而 401/403/400（key 错、模型名错、
  参数非法）重试没有意义，要立刻失败；
* **只取 content**：思考模型还会返回 ``reasoning_content``（思维链），它**不是**答案，必须丢掉。

本模块实现的三个核心函数::

    build_body(request)         -> dict                    # 组装 Chat Completions 请求体
    extract_content(response)   -> (content, 有无思维链, finish_reason)
    decide(request)             -> (content, 元信息)        # 带重试的发送主流程

对应测试：``tests/test_client.py``。
"""

from __future__ import annotations

import json
import random
import threading
import time
from typing import Any, Callable, Dict, Optional, Tuple

import requests

from .config import GatewayConfig
from .prompts import SYSTEM_PROMPT, USER_PREFIX

__all__ = ["LlmError", "LlmAuthError", "LlmFatalError", "LlmClient"]

#: 这些 HTTP 状态码值得重试（服务端临时不可用 / 限流）
RETRY_STATUS = frozenset({408, 409, 425, 429, 500, 502, 503, 504, 522, 524})


class LlmError(RuntimeError):
    """调用上游失败（重试耗尽或响应结构异常）。"""


class LlmAuthError(LlmError):
    """API Key 缺失/无效（401/403）。"""


class LlmFatalError(LlmError):
    """重试也不会变好的错误（HTTP 400：模型名或参数不合法）。"""


class LlmClient:
    """OpenAI 兼容 Chat Completions 的最小客户端（线程安全）。"""

    def __init__(
        self,
        config: GatewayConfig,
        session: Optional[requests.Session] = None,
        sleep: Callable[[float], None] = time.sleep,
        log: Optional[Callable[[str], None]] = None,
    ) -> None:
        self.config = config
        self.api_key = config.resolved_api_key()
        self._session = session or requests.Session()
        self._sleep = sleep
        self._log = log or (lambda _message: None)
        self._lock = threading.Lock()
        self._closed = False

    # ------------------------------------------------------------------ #
    # 已经写好的部分（不用改）
    # ------------------------------------------------------------------ #
    @property
    def endpoint(self) -> str:
        """拼出完整 URL：``<base>/chat/completions``（base 已经带了就不重复拼）。"""
        base = (self.config.api_base_url or "").rstrip("/")
        if base.endswith("/chat/completions"):
            return base
        return base + "/chat/completions"

    def build_headers(self) -> Dict[str, str]:
        """鉴权与内容类型。"""
        return {
            "Authorization": "Bearer %s" % self.api_key,
            "Content-Type": "application/json",
            "Accept": "application/json",
        }

    def retry_delay(self, attempt: int, retry_after: Optional[str]) -> float:
        """指数退避 + 抖动；服务端给了 ``Retry-After`` 就听它的（不超过 backoff_max）。"""
        if retry_after:
            try:
                return max(0.0, min(float(retry_after), self.config.backoff_max))
            except (TypeError, ValueError):
                pass
        delay = min(self.config.backoff_base * (2 ** (attempt - 1)), self.config.backoff_max)
        return delay + random.uniform(0.0, self.config.backoff_base)

    def close(self) -> None:
        self._closed = True
        try:
            self._session.close()
        except Exception:  # noqa: BLE001
            pass

    # ------------------------------------------------------------------ #
    # 第 7 步：组装请求体
    # ------------------------------------------------------------------ #
    def build_body(self, request: Dict[str, Any]) -> Dict[str, Any]:
        """构造 Chat Completions 请求体。

    提示：

    * ``messages`` 两条：``{"role": "system", "content": SYSTEM_PROMPT}`` 和
      ``{"role": "user", "content": USER_PREFIX + json.dumps(request, ensure_ascii=False, separators=(",", ":"))}``；
    * 还需要 ``model`` / ``stream=False`` / ``temperature`` / ``max_tokens``（都从 ``self.config`` 取）；
    * ``self.config.enable_json_format`` 为真时加
      ``"response_format": {"type": "json_object"}``；
    * 思考开关：``"thinking": {"type": "enabled" if self.config.enable_thinking else "disabled"}``。

    目标：``tests/test_client.py`` 的 ``test_body_*`` 与 ``test_prompt_contains_json_word_for_json_mode`` 通过。
        """
        body = {
            "model": self.config.model,
            "messages": [
                {"role": "system", "content": SYSTEM_PROMPT},
                {"role": "user", "content": USER_PREFIX + json.dumps(
                    request, ensure_ascii=False, separators=(",", ":"), allow_nan=False,
                )},
            ],
            "stream": False,
            "temperature": self.config.temperature,
            "max_tokens": self.config.max_tokens,
            "thinking": {"type": "enabled" if self.config.enable_thinking else "disabled"},
        }
        if self.config.enable_json_format:
            body["response_format"] = {"type": "json_object"}
        return body

    # ------------------------------------------------------------------ #
    # 第 8 步：发送（带重试）
    # ------------------------------------------------------------------ #
    def decide(self, request: Dict[str, Any]) -> Tuple[str, Dict[str, Any]]:
        """请求一次出拳，返回 ``(content 文本, 元信息)``；重试耗尽抛 :class:`LlmError`。

    提示（骨架逻辑）：

    * 没有 key（``self.api_key`` 为空）直接抛 :class:`LlmAuthError`；
    * ``attempts = max(1, self.config.max_retries + 1)``，循环 ``attempt = 1..attempts``；
    * 每次 ``self._session.post(self.endpoint, data=payload, headers=..., timeout=(connect, read))``，
      其中 ``payload = json.dumps(body, ensure_ascii=False).encode("utf-8")``；
    * 状态码分支：
      - ``200`` -> ``content, has_reasoning, finish_reason = self.extract_content(response)``，
        返回 ``(content, {"attempt": ..., "http_status": 200, "latency_s": ..., "has_reasoning": ..., "finish_reason": ...})``；
      - ``401`` / ``403`` -> 抛 :class:`LlmAuthError`（不要重试）；
      - ``400`` -> 抛 :class:`LlmFatalError`（不要重试）；
      - 其它：状态码在 ``RETRY_STATUS`` 且还有重试机会 -> ``self._sleep(self.retry_delay(attempt, Retry-After))``
        后 ``continue``；否则抛 :class:`LlmError`；
    * ``LlmError``（比如空回复）和 ``requests.exceptions.RequestException``（网络错误）
      都当作可重试：还有机会就 sleep + 重试，否则把它抛出去 / 循环结束后抛 :class:`LlmError`；
    * 别忘了 ``except (LlmAuthError, LlmFatalError): raise`` 放在前面，避免它们被重试。

    目标：``tests/test_client.py`` 的 ``test_decide_*`` 通过。
        """
        if not self.api_key:
            raise LlmAuthError("缺少 API Key，请设置 %s" % self.config.api_key_env)
        if self._closed:
            raise LlmFatalError("客户端已经关闭")

        payload = json.dumps(self.build_body(request), ensure_ascii=False).encode("utf-8")
        attempts = max(1, self.config.max_retries + 1)
        started = time.perf_counter()
        for attempt in range(1, attempts + 1):
            retry_after = None
            try:
                with self._lock:
                    response = self._session.post(
                        self.endpoint,
                        data=payload,
                        headers=self.build_headers(),
                        timeout=(self.config.connect_timeout, self.config.request_timeout),
                    )
                status = response.status_code
                if status != 200:
                    detail = str(response.text).replace(self.api_key, "[REDACTED]")[:300]
                    message = "HTTP %s: %s" % (status, detail)
                    if status in (401, 403):
                        raise LlmAuthError(message)
                    if status not in RETRY_STATUS:
                        raise LlmFatalError(message)
                    retry_after = response.headers.get("Retry-After")
                    raise LlmError(message)

                content, has_reasoning, finish_reason = self.extract_content(response)
                return content, {
                    "attempt": attempt,
                    "http_status": status,
                    "latency_s": time.perf_counter() - started,
                    "has_reasoning": has_reasoning,
                    "finish_reason": finish_reason,
                }
            except (LlmAuthError, LlmFatalError):
                raise
            except (LlmError, requests.exceptions.RequestException) as exc:
                detail = str(exc).replace(self.api_key, "[REDACTED]")
                if attempt == attempts:
                    raise LlmError("请求失败，已尝试 %d 次: %s" % (attempt, detail)) from exc
                delay = self.retry_delay(attempt, retry_after)
                self._log("第 %d 次请求未完成，%.2f 秒后重试: %s" % (attempt, delay, detail))
                self._sleep(delay)
        raise LlmError("请求未能完成")

    # ------------------------------------------------------------------ #
    # 第 9 步：取出答案
    # ------------------------------------------------------------------ #
    @staticmethod
    def extract_content(response: Any) -> Tuple[str, bool, str]:
        """从 200 响应里取出 ``content``，返回 ``(content, 是否带思维链, finish_reason)``。

    提示：

    * ``response.json()`` 失败（不是 JSON）-> 抛 :class:`LlmError`；
    * ``data.get("choices")`` 为空 -> 抛 :class:`LlmError`；
    * 取 ``choices[0]["message"]`` 的 ``content`` 与 ``reasoning_content``；
    * ``content`` 不是非空字符串时都要抛 :class:`LlmError`（让上层重试），并且错误信息要能区分原因：
      - 有 ``reasoning_content``（思维链不是答案！）-> 提示"只返回了 reasoning_content"；
      - ``finish_reason == "length"`` -> 提示 max_tokens 不够用；
      - 其它 -> 提示 content 为空；
    * ``has_reasoning`` = ``reasoning_content`` 是非空字符串。

    目标：``tests/test_client.py`` 的 ``test_extract_content_*`` 通过。
        """
        try:
            data = response.json()
        except ValueError as exc:
            raise LlmError("上游响应不是合法 JSON") from exc
        if not isinstance(data, dict):
            raise LlmError("上游响应不是 JSON 对象")
        choices = data.get("choices")
        if not isinstance(choices, list) or not choices or not isinstance(choices[0], dict):
            raise LlmError("上游 choices 为空或格式不正确")
        message = choices[0].get("message")
        if not isinstance(message, dict):
            raise LlmError("上游 message 格式不正确")

        content = message.get("content")
        reasoning = message.get("reasoning_content")
        has_reasoning = isinstance(reasoning, str) and bool(reasoning.strip())
        finish_reason = choices[0].get("finish_reason") or ""
        if not isinstance(finish_reason, str):
            finish_reason = ""
        if not isinstance(content, str) or not content.strip():
            if has_reasoning:
                raise LlmError("只返回了 reasoning_content，没有最终 content")
            if finish_reason == "length":
                raise LlmError("回复被截断，请增大 max_tokens")
            raise LlmError("上游 content 为空或不是字符串")
        if finish_reason == "length":
            raise LlmError("回复被截断，请增大 max_tokens")
        return content, has_reasoning, finish_reason
````

**app.py**

````python
"""把「读输入 -> 转发 -> JSON 容错 -> 校验 -> 兜底 -> 写输出」串成一条流水线。

对外只有一条硬性约定：**读进来一行，就写出去一行合法 JSON 决策**。
不管上游挂了、模型胡说、还是输入本身不是 JSON，都不允许把异常抛给调用方，
也不允许输出半截/非法 JSON —— 否则下游（真实系统里的消息总线）会直接报错。

本模块的核心编排函数::

    handle_request(request) -> (决策, 说明列表)

``handle_line`` / ``run`` / ``Stats`` 已经写好，不用改。
对应测试：``tests/test_app.py``。
"""

from __future__ import annotations

import sys
from dataclasses import dataclass
from typing import Any, Callable, Dict, Iterable, List, Optional, TextIO, Tuple

from .client import LlmClient
from .config import GatewayConfig
from .json_guard import JsonGuardError, dumps_json, loads_object
from .rules import (
    build_decision,
    fallback_decision,
    get_time,
    is_valid_decision,
    now_iso,
    parse_request,
    validate_decision,
)

__all__ = ["Stats", "JsonGateway"]


@dataclass
class Stats:
    """跑完一轮之后的统计，方便观察容错到底起了多少作用。"""

    lines: int = 0
    ok: int = 0        # 模型的答案直接可用
    repaired: int = 0  # 可用但被网关修正过（例如丢弃模型自报的 time）
    fallback: int = 0  # 走了本地兜底出拳
    bad_input: int = 0  # 输入行不是合法 JSON

    def as_dict(self) -> Dict[str, int]:
        return {
            "lines": self.lines,
            "ok": self.ok,
            "repaired": self.repaired,
            "fallback": self.fallback,
            "bad_input": self.bad_input,
        }


class JsonGateway:
    """一进一出的决策网关。"""

    def __init__(
        self,
        config: GatewayConfig,
        client: Optional[Any] = None,
        log: Optional[Callable[[str], None]] = None,
    ) -> None:
        self.config = config
        self.log = log or (lambda message: print("[gateway] %s" % message, file=sys.stderr, flush=True))
        if client is not None:
            self.client = client
        elif config.offline:
            self.client = None  # 离线模式：只用本地规则出拳
        else:
            self.client = LlmClient(config, log=lambda message: self.log("上游: %s" % message))

    # ------------------------------------------------------------------ #
    # 第 10 步：核心编排
    # ------------------------------------------------------------------ #
    def handle_request(self, request: Dict[str, Any]) -> Tuple[Dict[str, Any], List[str]]:
        """核心编排：调用上游 -> JSON 容错 -> 合法性校验 -> 兜底。

    返回 ``(合法决策, 说明列表)``，**永不抛异常**。提示：

    1. ``notes: List[str] = []``、``decision: Optional[Dict[str, Any]] = None``；
    2. 只有在 ``self.client is not None``（在线模式）时才调用上游：
       * ``content, meta = self.client.decide(request)`` 拿回原始文本；
       * ``candidate = loads_object(content)`` 把任意文本整理成 dict；
       * ``decision, notes = validate_decision(candidate, request)`` 校验/修正；
       * 可以顺手把 ``meta["latency_s"]`` 之类写进日志（可选）；
    3. ``JsonGuardError``（模型输出整理不出来）和**其它任何异常**（上游超时、500……）
       都要被接住：往 ``notes`` 里记一句原因，然后 ``decision`` 保持为 ``None``；
    4. ``decision is None`` 时用 ``fallback_decision(request)`` 本地出拳，
       ``notes`` 里加上 ``"本地兜底"`` 和兜底自己的说明；
    5. 出口用 ``is_valid_decision(decision)`` 再自检一次，
       万一不合法就退化成 ``build_decision(get_time(request) or now_iso(), "rock")``，
       ``notes`` 里加上 ``"出口自检失败->rock"``；
    6. 返回 ``(decision, notes)``。

    目标：``tests/test_app.py`` 的 ``test_handle_request_*`` 与 ``test_run_*`` 通过。
        """
        if not isinstance(request, dict):
            decision, notes = fallback_decision({})
            return decision, ["请求不是对象->本地兜底"] + notes

        # 固定本轮时间，不修改调用方的原始请求，也不让模型替换它。
        request = dict(request)
        request["time"] = get_time(request) or now_iso()
        notes: List[str] = []
        decision: Optional[Dict[str, Any]] = None
        if self.client is not None:
            try:
                content, _meta = self.client.decide(request)
                candidate = loads_object(content)
                decision, notes = validate_decision(candidate, request)
            except JsonGuardError as exc:
                notes.append("模型输出无法整理: %s" % exc)
            except Exception as exc:  # noqa: BLE001 - 不能让上游异常破坏逐行协议
                notes.append("上游异常: %s" % exc)

        if decision is None:
            decision, extra = fallback_decision(request)
            notes += ["本地兜底"] + extra
        if not is_valid_decision(decision):
            decision = build_decision(request["time"], "rock")
            notes.append("出口自检失败->rock")
        return decision, notes

    # ------------------------------------------------------------------ #
    # 已写好：输入解析与主循环（不用改）
    # ------------------------------------------------------------------ #
    def handle_line(self, line: str) -> Tuple[Dict[str, Any], List[str]]:
        """一行文本 -> ``(决策, 说明)``。

        输入不是 JSON 时**不转发给上游**（坏数据不值得花钱），直接用「当前时间 + 兜底出拳」回一行。
        """
        notes: List[str] = []
        text = line.strip()
        try:
            request = parse_request(text)
        except Exception as exc:  # noqa: BLE001 - 坏输入也要有输出
            notes.append("输入不是合法 JSON: %s" % exc)
            decision, extra = fallback_decision({})
            return decision, notes + extra
        decision, extra = self.handle_request(request)
        return decision, notes + extra

    def run(self, reader: Iterable[str], writer: TextIO) -> Stats:
        """逐行处理，写出 ``{"time": ..., "option": ...}``，返回统计。"""
        stats = Stats()
        for line in reader:
            if not line.strip():
                continue
            stats.lines += 1
            try:
                decision, notes = self.handle_line(line)
            except Exception as exc:  # noqa: BLE001 - 最后一道保险
                decision, notes = build_decision(now_iso(), "rock"), ["内部异常: %s" % exc]
            writer.write(dumps_json(decision) + "\n")
            writer.flush()
            if any("输入不是合法 JSON" in note for note in notes):
                stats.bad_input += 1
            if any("兜底" in note for note in notes):
                stats.fallback += 1
            elif notes:
                stats.repaired += 1
            else:
                stats.ok += 1
            self.log(
                "第 %d 行 -> %s %s"
                % (stats.lines, dumps_json(decision), ("| " + "; ".join(notes)) if notes else "")
            )
        return stats
````
