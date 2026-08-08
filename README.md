# 🎵 AI Music Player

![GitHub stars](https://img.shields.io/github/stars/ui14411/musicPlayer)
![GitHub license](https://img.shields.io/github/license/ui14411/musicPlayer)

[![Qt](https://img.shields.io/badge/Qt-6.7.3-blue.svg)](https://www.qt.io/)
[![C++](https://img.shields.io/badge/C++-17-orange.svg)](https://en.cppreference.com/)
[![ONNX Runtime](https://img.shields.io/badge/ONNX_Runtime-1.18-green.svg)](https://onnxruntime.ai/)

基于 **Qt/QML + C++ + AI 音频处理技术** 开发的多功能音乐播放器。

支持 AI 人声分离、HRTF 三维环绕声、双耳分听、歌词同步、频谱显示等功能。

这是我的第一个大型 C++ 项目。

---

# ✨ 功能特性

## 🎵 基础播放器

- 支持 MP3 / WAV / FLAC 等音频格式
- 本地音乐扫描
- 音乐列表管理
- 播放 / 暂停 / 上一首 / 下一首
- 播放进度控制
- 音量调节


## 🤖 AI 音频处理

### AI 人声分离

基于：

- ONNX Runtime
- UVR-MDX-NET 模型


支持：

- 提取人声
- 提取伴奏


功能流程：

```
Audio
 |
 | FFmpeg Decode
 |
 | STFT
 |
 | ONNX Runtime Inference
 |
 | ISTFT
 |
Output Audio
```


---

## 🎧 HRTF 环绕声

实现：

- 普通立体声转 3D 环绕声
- 空间定位效果
- HRTF 滤波处理


支持生成：

```
Stereo Audio

      ↓

HRTF Processing

      ↓

3D Surround Audio
```


---

## 👂 双耳分听

支持：

- 左耳独立播放
- 右耳独立播放
- 左右声道独立控制
- 双耳不同步播放测试


---

## 🎤 歌词系统

支持：

- LRC歌词解析
- 当前歌词高亮
- 歌词自动匹配


---

## 🖼️ 其他功能

- 专辑封面自动获取
- 频谱可视化
- 自定义背景图片
- 视频背景播放
- 文件拖放导入


---

# 📦 下载运行

推荐直接下载 Release：

[Download Releases](https://github.com/ui14411/musicPlayer/releases)


下载：

```
MusicPlayer_v1.x.x.zip
```


解压后：

```
nusicVideoQml.exe
```

直接运行即可。


无需安装：

- Qt
- FFmpeg
- ONNX Runtime

Release 已包含运行环境。


---

# 🔧 从源码编译


## 环境要求


| 工具 | 版本 |
|-|-|
| Qt | 6.7.3 |
| CMake | 3.16+ |
| MSVC | Visual Studio 2022 |
| C++ | C++17 |
| ONNX Runtime | 1.18.0 |


---

# 📚 第三方依赖


## Qt

用于：

- GUI
- QML
- Multimedia


官网：

https://www.qt.io/


许可证：

LGPL


---

## FFmpeg

用于：

- 音频解码
- 音频转换


官网：

https://ffmpeg.org/


许可证：

LGPL


---

## ONNX Runtime

用于：

- AI模型推理


官网：

https://onnxruntime.ai/


许可证：

MIT


---

## KissFFT

用于：

- STFT
- ISTFT


许可证：

BSD-3-Clause


---

# 📁 项目结构


```
musicPlayer/

│
├── CPPFile/
│   └── C++ source files
│
├── HeaderFile/
│   └── C++ header files
│
├── QmlFile/
│   └── Qt Quick UI
│
├── FFmpeg/
│   └── FFmpeg development files
│
├── kissFFT/
│   └── FFT library
│
├── ONNXRuntime/
│   └── include/
│
├── CMakeLists.txt
│
├── main.cpp
│
├── LICENSE
│
└── README.md

```


---

# 🤖 AI模型


AI人声分离功能需要：

```
UVR-MDX-NET-Inst_HQ_3.onnx
```


模型来源：

开源社区提供的 UVR 模型。


本项目：

- 不训练模型
- 不修改模型
- 仅实现模型加载、推理和音频处理流程


模型文件不会上传到 GitHub。


放置位置：

```
musicPlayer/

├── nusicVideoQml.exe
│
└── model/
    └── UVR-MDX-NET-Inst_HQ_3.onnx

```


---

# ⚙️ 源码编译


## 1. 下载 ONNX Runtime


从 Releases 下载：

```
ONNXRuntime_Lib.zip
```


解压：

```
ONNXRuntime/

├── include/
│
└── lib/
    └── onnxruntime.lib

```


---

## 2. 配置 CMake


示例：

```bash
mkdir build

cd build


cmake .. ^
-DCMAKE_PREFIX_PATH="C:/Qt/6.7.3/msvc2022_64"

```


---

## 3. 编译


Release:

```bash
cmake --build . --config Release
```


---

# 📝 License


This project is licensed under the MIT License.


See:

```
LICENSE
```


---

# 🤝 Contributing


欢迎提交 Issue 和 Pull Request。


流程：

1. Fork 项目

2. 创建分支


```bash
git checkout -b feature/new-feature
```


3. 提交修改


```bash
git commit -m "Add new feature"
```


4. Push


```bash
git push origin feature/new-feature
```


5. 创建 Pull Request


---

# 👤 Author


GitHub:

https://github.com/ui14411


Email:

497476530 [at] qq.com


---

如果这个项目对你有帮助，欢迎 Star ⭐

