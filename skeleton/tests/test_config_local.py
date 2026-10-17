"""本地密钥配置测试：只使用临时目录和测试值。"""

import pytest

import gateway.config as config_module
from gateway.config import GatewayConfig


@pytest.fixture
def local_project(tmp_path, monkeypatch):
    root = tmp_path / "project"
    source = root / "skeleton/gateway/config.py"
    source.parent.mkdir(parents=True)
    monkeypatch.setattr(config_module, "__file__", str(source))
    monkeypatch.delenv("DEEPSEEK_API_KEY", raising=False)
    (root / ".env").write_text("DEEPSEEK_API_KEY=local-test-value\n")
    return root


def test_local_key_does_not_depend_on_working_directory(local_project, monkeypatch, tmp_path):
    monkeypatch.chdir(tmp_path)
    assert GatewayConfig().resolved_api_key() == "local-test-value"


def test_explicit_and_environment_values_take_priority(local_project, monkeypatch):
    monkeypatch.setenv("DEEPSEEK_API_KEY", "environment-test-value")
    assert GatewayConfig().resolved_api_key() == "environment-test-value"
    assert GatewayConfig(api_key="argument-test-value").resolved_api_key() == "argument-test-value"


def test_skeleton_env_takes_priority_over_project_env(local_project):
    (local_project / "skeleton/.env").write_text('export DEEPSEEK_API_KEY="skeleton-test-value"\n')
    assert GatewayConfig().resolved_api_key() == "skeleton-test-value"


def test_custom_variable_does_not_use_an_unrelated_key(local_project, monkeypatch):
    monkeypatch.delenv("TEST_ONLY_UNSET_KEY", raising=False)
    assert GatewayConfig(api_key_env="TEST_ONLY_UNSET_KEY").resolved_api_key() == ""
