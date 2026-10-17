"""网关配置：支持环境变量和本地私有 .env 文件。"""

from __future__ import annotations

import os
from dataclasses import dataclass, replace
from pathlib import Path
from typing import Any, Dict

__all__ = ["GatewayConfig"]


@dataclass
class GatewayConfig:
    """一次运行所需的全部配置。

    所有字段都有默认值，方便在测试里 ``GatewayConfig(offline=True)`` 一把构造。
    """

    # ---- 上游 API ----
    api_base_url: str = "https://api.deepseek.com"
    model: str = "deepseek-flash"
    api_key: str = ""
    api_key_env: str = "DEEPSEEK_API_KEY"

    # ---- 请求参数 ----
    enable_json_format: bool = True
    enable_thinking: bool = False
    temperature: float = 0.2
    max_tokens: int = 256

    # ---- 超时与重试 ----
    request_timeout: float = 15.0
    connect_timeout: float = 5.0
    max_retries: int = 2
    backoff_base: float = 0.5
    backoff_max: float = 4.0

    # ---- 其它 ----
    offline: bool = False  # True = 完全不调用 API，只用本地兜底出拳

    def resolved_api_key(self) -> str:
        """依次读取参数、环境变量、skeleton/.env、项目根目录 .env。"""
        if self.api_key and self.api_key.strip():
            return self.api_key.strip()
        value = (os.environ.get(self.api_key_env) or "").strip()
        if value:
            return value
        source = Path(__file__).resolve()
        for directory in (source.parents[1], source.parents[2]):
            try:
                lines = (directory / ".env").read_text(encoding="utf-8").splitlines()
            except OSError:
                continue
            for line in lines:
                name, separator, value = line.strip().removeprefix("export ").partition("=")
                if separator and name.strip() == self.api_key_env:
                    value = value.strip().strip("\"'")
                    if value:
                        return value
        return ""

    def with_overrides(self, **overrides: Any) -> "GatewayConfig":
        """返回一个覆盖了部分字段的新配置（``None`` 表示不改）。"""
        clean: Dict[str, Any] = {
            key: value for key, value in overrides.items() if value is not None and hasattr(self, key)
        }
        return replace(self, **clean) if clean else replace(self)
