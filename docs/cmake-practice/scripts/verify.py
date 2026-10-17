#!/usr/bin/env python3
"""Build the real project, check its CLI, and record reproducible evidence."""
from pathlib import Path
import datetime
import json
import math
import re
import shlex
import subprocess

ROOT=Path(__file__).resolve().parents[3]
PROJECT=ROOT/'UnitConverter'
EVIDENCE=ROOT/'docs/cmake-practice'
LOGS=EVIDENCE/'logs'
LOGS.mkdir(parents=True,exist_ok=True)

def run(args,cwd=PROJECT,stdin=None,success=True):
    result=subprocess.run(args,cwd=cwd,input=stdin,text=True,capture_output=True,timeout=30)
    record=f'工作目录：{cwd}\n命令：{shlex.join(args)}\n'
    if stdin is not None:
        record+='自动输入：\n'+repr(stdin)+'\n'
    record+='\n标准输出：\n'+result.stdout+'\n标准错误：\n'+result.stderr
    record+=f'\n退出码：{result.returncode}\n'
    if success and result.returncode!=0:
        raise RuntimeError(record)
    return result,record

environment='记录时间：'+datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat()+'\n'
for command in [['cmake','--version'],['g++','--version'],['make','--version'],['uname','-sr']]:
    _,record=run(command)
    environment+=record+'\n'
(LOGS/'01-environment.txt').write_text(environment)

_,configure=run(['cmake','-S','.','-B','build','-DCMAKE_BUILD_TYPE=Debug','-DCMAKE_EXPORT_COMPILE_COMMANDS=ON'])
(LOGS/'02-configure.txt').write_text(configure)
_,build=run(['cmake','--build','build','--parallel','2'])
(LOGS/'03-build.txt').write_text(build)
assert 'warning:' not in build
executable=PROJECT/'build/unit_converter'

cases=[]
def check(name,stdin,expected=None,contains=(),count=None):
    result,record=run([str(executable)],stdin=stdin)
    values=[float(value) for value in re.findall(r'转换结果：([^ ]+) ',result.stdout)]
    if expected is not None:
        assert len(values)==len(expected),(name,values,expected)
        for actual,reference in zip(values,expected):
            assert math.isclose(actual,reference,rel_tol=1e-9,abs_tol=0.0000011),(name,actual,reference)
    for fragment in contains:
        assert fragment in result.stdout,(name,fragment,result.stdout)
    if count is not None:
        assert len(values)==count,(name,values,count)
    item={'name':name,'input':stdin,'expected':expected,'actual':values,'status':'PASS','output':result.stdout}
    cases.append(item)
    (LOGS/f'case-{len(cases):02}.txt').write_text(record)

# Fixed reference values test all six directions independently of the C++ formulas.
numeric=[
    ('摄氏度→华氏度：零点',1,'0',32),
    ('华氏度→摄氏度：沸点',2,'212',100),
    ('米→英尺：1米',3,'1',3.280839895013123),
    ('英尺→米：1英尺',4,'1',0.3048),
    ('磅→千克：1磅',5,'1',0.45359237),
    ('千克→磅：1千克',6,'1',2.204622621848776),
    ('负温度：摄氏度',1,'-40',-40),
    ('负温度：华氏度',2,'-40',-40),
    ('小数长度：1.5米',3,'1.5',4.921259842519685),
    ('零长度',4,'0',0),
    ('零质量',5,'0',0),
    ('科学计数法：100千克',6,'1e2',220.4622621848776),
]
for name,choice,value,expected in numeric:
    check(name,f'{choice}\n{value}\n0\n',[expected],contains=('程序结束。',))

check('菜单输入结束','',[],contains=('程序结束。',))
check('数值输入结束','1\n',[],contains=('程序结束。',))
check('菜单范围检查','9\n1\n0\n0\n',[32],contains=('请选择0～6',))
check('非数字输入结束','abc\n',[],contains=('程序结束。',))
check('六方向连续转换','1\n0\n2\n212\n3\n1\n4\n1\n5\n1\n6\n1\n0\n',
      [32,100,3.280839895013123,0.3048,0.45359237,2.204622621848776],contains=('程序结束。',))
check('直接退出','0\n',[],contains=('程序结束。',))
(LOGS/'04-tests.json').write_text(json.dumps(cases,ensure_ascii=False,indent=2))
summary='全部 CLI 黑盒用例通过\n用例数量：'+str(len(cases))+'\n数值绝对容差：0.0000011\n\n'
summary+='\n'.join(f'{i:02}. PASS  {case["name"]}  实际结果={case["actual"]}' for i,case in enumerate(cases,1))
(LOGS/'04-tests-summary.txt').write_text(summary+'\n')

_,install=run(['cmake','--install','build','--prefix',str(PROJECT/'install')])
_,installed=run([str(PROJECT/'install/bin/unit_converter')],stdin='1\n0\n0\n')
assert '转换结果：32.000000 华氏度' in installed
(LOGS/'06-install.txt').write_text(install+'\n'+installed)
_,ignored=run(['git','check-ignore','UnitConverter/build/CMakeCache.txt','UnitConverter/install/bin/unit_converter'],cwd=ROOT)
(LOGS/'07-git-ignore.txt').write_text(ignored)
print(summary)
print('\n安装运行正常；构建和安装产物被 Git 忽略。')
