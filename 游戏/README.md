# Flappy Bird C++ 游戏

这是一个使用C++和SFML库开发的Flappy Bird游戏。

## 功能特点

- 经典Flappy Bird游戏玩法
- 流畅的物理引擎
- 随机生成的管道障碍
- 分数系统
- 游戏结束和重新开始功能
- 简洁的图形界面

## 系统要求

- Windows 10/11
- Visual Studio 2019或更新版本（或MinGW）
- CMake 3.10或更新版本
- SFML 2.5库

## 安装和运行

### 1. 安装SFML库

#### 使用vcpkg（推荐）
```bash
# 安装vcpkg（如果还没有安装）
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat

# 安装SFML
.\vcpkg install sfml:x64-windows
```

#### 手动安装
1. 从 https://www.sfml-dev.org/download.php 下载SFML
2. 解压到合适的位置
3. 设置环境变量或在CMakeLists.txt中指定路径

### 2. 编译游戏

```bash
# 创建构建目录
mkdir build
cd build

# 配置项目
cmake ..

# 编译
cmake --build . --config Release
```

### 3. 运行游戏

```bash
# 在build/bin目录下运行
./FlappyBird.exe
```

## 游戏控制

- **空格键**: 让小鸟跳跃
- **空格键**: 游戏结束后重新开始

## 游戏规则

1. 按空格键让小鸟跳跃
2. 避开绿色的管道障碍
3. 每通过一个管道得1分
4. 撞到管道、地面或天花板游戏结束
5. 游戏结束后按空格键重新开始

## 项目结构

```
FlappyBird/
├── main.cpp          # 主游戏代码
├── CMakeLists.txt    # CMake构建文件
└── README.md         # 说明文档
```

## 技术实现

- **图形渲染**: SFML Graphics模块
- **窗口管理**: SFML Window模块
- **物理引擎**: 自定义重力系统
- **碰撞检测**: SFML边界检测
- **随机生成**: C++11随机数生成器

## 开发环境

- 编程语言: C++17
- 图形库: SFML 2.5
- 构建系统: CMake
- 编译器: MSVC 2019+ 或 MinGW

## 故障排除

### 常见问题

1. **找不到SFML库**
   - 确保SFML已正确安装
   - 检查CMakeLists.txt中的路径设置

2. **编译错误**
   - 确保使用C++17标准
   - 检查SFML版本兼容性

3. **运行时错误**
   - 确保SFML DLL文件在可执行文件目录中
   - 检查字体文件是否存在

## 扩展功能

可以考虑添加的功能：
- 音效和背景音乐
- 更复杂的障碍物
- 难度递增
- 最高分记录
- 更精美的图形
