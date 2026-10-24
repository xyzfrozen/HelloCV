# ROS2 Humble C++ 学习示例

代码对应[ROS2 学习笔记](../docs/ros2/ROS2.md)，从笔记中的八份完整 C++ 示例整理而来，保留变量名、节点名、话题名和讲解之间的对应关系。

## 环境与依赖

- Ubuntu 22.04、ROS2 Humble。
- C++17 编译器、CMake、colcon，以及 ROS2 的 `ament_cmake`。
- 编译依赖：`rclcpp`、`std_msgs`、`std_srvs`、`rcl_interfaces`、`sensor_msgs`、`visualization_msgs`。
- 运行工具：`launch`、`launch_ros`、`ament_index_python`、`tf2_ros`、`tf2_tools`、`rviz2`。

包清单 `src/hello_cpp/package.xml` 声明了所需依赖。若已配置 rosdep，可在本工作空间目录检查、安装缺少的依赖：

```bash
source /opt/ros/humble/setup.bash
rosdep install --from-paths src --ignore-src --rosdistro humble -y
```

rosdep 安装系统软件包时可能调用 sudo。已具备全部依赖的环境可直接构建。

## 构建

以下第一条 `cd` 从 HelloCV 仓库根目录执行：

```bash
cd ros2_ws
source /opt/ros/humble/setup.bash
colcon build --packages-select hello_cpp --symlink-install
source install/setup.bash
```

修改 C++ 后需要重新构建。`build/`、`install/` 和 `log/` 为本地产物，已通过仓库忽略规则排除。

每个运行终端都需要加载系统和工作空间环境；以下命令中的 `source install/setup.bash` 在本工作空间目录执行：

```bash
source /opt/ros/humble/setup.bash
source install/setup.bash
```

## 单独运行

| 可执行文件 | 节点名 | 作用 |
|---|---|---|
| `hello_node` | `/hello_cv` | 打印 HelloCV 日志并保持节点运行 |
| `heartbeat` | `/heartbeat` | 约每秒打印计数，观察定时器回调 |
| `status_publisher` | `/status_publisher` | 向 `/vision/status` 约每秒发布两次模拟状态 |
| `status_listener` | `/status_listener` | 订阅 `/vision/status` 并打印内容 |
| `detection_server` | `/detection_server` | 提供 `/set_detection`，类型为 `std_srvs/srv/SetBool` |
| `detection_client` | `/detection_client` | 等待服务并发送开启请求，打印响应后退出 |
| `threshold_node` | `/threshold_node` | 使用可动态设置的 `threshold` 参数过滤固定模拟置信度 |
| `visual_demo` | `/visual_demo` | 发布 `/demo/image` 合成图像和 `/demo/target` 目标 Marker |

运行格式：

```bash
ros2 run hello_cpp hello_node
```

Topic 示例在两个终端分别启动 `status_publisher` 和 `status_listener`；Service 示例先启动 `detection_server`，再运行 `detection_client`。其他节点的参数和观察命令见学习笔记。

## Launch 与参数配置

```bash
ros2 launch hello_cpp learning.launch.py
```

启动发布者、订阅者、服务端和参数节点。发布与订阅双方的话题均重映射为 `/vision/demo_status`，参数节点从 `config/detector.yaml` 加载阈值 0.65。

另开已加载环境的终端：

```bash
ros2 node list
ros2 topic echo /vision/demo_status --once
ros2 param get /threshold_node threshold
ros2 param set /threshold_node threshold 0.80
ros2 service call /set_detection std_srvs/srv/SetBool "{data: false}"
```

参数节点使用固定置信度 0.70，便于对照阈值改变后的日志。服务开关只改变服务端自己的示例状态，没有自动控制其他独立节点。启动 Launch 前，先结束之前手动运行的同名节点。

## TF、图像与 Marker

按笔记第 7.2 节的命令，在独立终端发布以下静态 TF：

```text
world → base_link → camera_link → camera_optical_frame
```

之后分别运行：

```bash
ros2 run hello_cpp visual_demo
```

```bash
ros2 run rviz2 rviz2
```

在 RViz 中设置 `Fixed Frame=world`，添加 `TF`、话题 `/demo/image` 的 `Image`，以及话题 `/demo/target` 的 `Marker`。合成图像与示意目标仅用于学习可视化数据流，没有接入真实相机或识别模型。

生成 TF 图时，在本工作空间选择输出目录：

```bash
mkdir -p notes/tf
cd notes/tf
ros2 run tf2_tools view_frames
```

截图和实际通信结果应由后续实操补充；现有验证范围见[构建验证记录](../docs/ros2/构建验证记录.md)。

## 目录结构

```text
ros2_ws/
└── src/hello_cpp/
    ├── CMakeLists.txt
    ├── package.xml
    ├── LICENSE
    ├── src/          # 八个完整 C++ 示例
    ├── config/      # detector.yaml
    └── launch/      # learning.launch.py
```
