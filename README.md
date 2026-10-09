# HelloCV

第一次任务的学习与实践材料汇总，包含 Ubuntu/Linux 环境搭建、Git 学习和分支开发计算器。

## 材料入口

| 内容 | 链接 |
|---|---|
| Linux 学习与实践记录 | [语雀文档](https://www.yuque.com/xyzfrozen/mcnuvk/ka6agsl342sammss) |
| Git 学习笔记 | [语雀文档](https://www.yuque.com/xyzfrozen/mcnuvk/gggicdc7nbrhrkpt) |
| Git 计算器实践记录 | [语雀文档](https://www.yuque.com/xyzfrozen/mcnuvk/utcsciwdap9kb6qg) |
| Gitee仓库 | [Gitee：git_training](https://gitee.com/xyzfrozen/git_training) |


## 仓库结构

```text
HelloCV/
├── README.md     # 环境配置、材料入口和运行方法
├── NOTES.md      # Linux 与 Git 语雀笔记链接汇总
└── .gitignore    # 编译产物与本地编辑器文件忽略规则
```

计算器源码保存在独立的 `git_training` 仓库中，本仓库作为任务提交入口。

## 开发环境

使用原生 Ubuntu 22.04.5 LTS。环境验证记录中的主要版本如下：

| 工具 | 版本 |
|---|---|
| GCC / G++ | 11.4.0 |
| Git | 2.34.1 |
| Make / CMake | 4.3 / 3.22.1 |
| VS Code | 1.122.0 |
| ROS2 | Humble |
| Vim / tmux | 8.2 / 3.2a |
| Node.js / npm / PM2 | 22.16.0 / 10.9.2 / 7.0.4 |

### Ubuntu 基础工具

```bash
sudo apt update
sudo apt install build-essential gcc g++ make cmake git nmap \
  ssh openssh-client openssh-server vim flex bison tmux wget
```

SSH 服务端可通过以下命令启动并设置开机启用：

```bash
sudo systemctl enable --now ssh
systemctl is-active ssh
```

### VS Code、ROS2 和 Node.js

可使用 FishROS 安装入口，在菜单中分别选择 VS Code、ROS/ROS2 和 Node.js 环境；ROS2 选择 Humble。

```bash
wget http://fishros.com/install -O fishros && . fishros
```

安装 ROS2 后加载环境：

```bash
source /opt/ros/humble/setup.bash
```

Node.js 环境准备好后，将 PM2 安装到用户目录：

```bash
npm install -g --prefix "$HOME/.local" pm2
export PATH="$HOME/.local/bin:$PATH"
```

当前中文输入使用 Fcitx 与搜狗拼音。详细安装入口、配置、运行截图和问题记录见 Linux 语雀文档。

### 环境检查

```bash
g++ --version
git --version
cmake --version
code --version
vim --version
tmux -V
ssh -V
node --version
npm --version
pm2 --version
ros2 --help
```

## 计算器编译与使用

### 获取源码

```bash
git clone https://gitee.com/xyzfrozen/git_training.git
cd git_training
```

源码结构：

```text
git_training/
├── main.cpp     # 读取输入并选择运算
├── addsub.h     # 加法与减法实现
└── muldiv.h     # 乘法与除法实现
```

### 编译运行

输入顺序是“第一个整数 第二个整数 运算符”，使用空格分隔。例如：

```text
输入：8 2 +
输出：10
```

| 输入 | 输出 |
|---|---|
| `8 2 +` | `10` |
| `8 2 -` | `6` |
| `8 2 *` | `16` |
| `8 2 /` | `4` |
| `7 2 /` | `3` |

当前程序使用整数运算，因此除法结果会截去小数部分。除数不能为 0，运算符需使用 `+`、`-`、`*`、`/`；当前版本没有完整的非法输入处理。

## Git 开发过程

计算器在 `feature/add-sub` 和 `feature/mul-div` 分支中分别实现加减法与乘除法，再合并到 `main`。可在源码仓库查看提交关系：

```bash
git log --oneline --graph --decorate --all
```

命令操作、stash 使用、分支合并和运行截图见 Git 实践语雀文档。
