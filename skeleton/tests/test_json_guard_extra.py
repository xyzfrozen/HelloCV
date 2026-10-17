"""补充验证：清理格式时保留字符串内容，以及多个候选对象的处理。"""

import json

import pytest

from gateway.json_guard import find_first_json_object, repair_json_text, strip_code_fence


def test_fences_prefer_object_over_an_explanation():
    text = '```text\n说明\n```\n```JSON\n{"option":"paper"}\n```'
    assert strip_code_fence(text) == '{"option":"paper"}'


def test_fence_with_inline_language_tag():
    assert strip_code_fence('好的：```json {"option":"rock"}```') == '{"option":"rock"}'


def test_find_object_after_unfinished_prefix():
    assert find_first_json_object('前文 { 未结束，后文 {"option":"rock"}') == '{"option":"rock"}'


@pytest.mark.parametrize("text, expected", [
    ('{"reason":"None True False,}","option":"rock",}',
     {"reason": "None True False,}", "option": "rock"}),
    ("{'url':'https://example.com/a/*b*/','option':'paper',}",
     {"url": "https://example.com/a/*b*/", "option": "paper"}),
    ('{"a":1, /* comment with \' and \" */ "b":True,}', {"a": 1, "b": True}),
    ('{"reason":"None True False', {"reason": "None True False"}),
])
def test_repair_keeps_content_inside_strings(text, expected):
    assert json.loads(repair_json_text(text)) == expected
