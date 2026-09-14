# AGENTS.md

## 1. Project Goal

本仓库用于《计算机体系结构》课程的系统性能评价实验。

实验对象仅比较两颗 CPU：

- Apple M5（macOS / ARM64）
- Intel Core i5-12400F（Windows / x86-64）

本项目实现一套跨平台、可复现、可量化的 CPU Toy Benchmark 测试框架。

同一份 C++ 源代码应能够在 macOS 和 Windows 上分别编译运行，并自动识别当前平台，将实验结果保存到各自独立的结果目录中。

本项目只进行本次课程实验所需的 CPU 性能测试，不设计后续扩展实验。

本项目不测试 GPU，不调用 CUDA、Metal 或任何 GPU 加速接口。

最终目标：

1. 两个平台运行相同算法与相同 workload；
2. 两个平台尽可能使用一致的编译策略；
3. 尽量避免编译器将原始算法转换为明显不同的平台特化实现；
4. 测试结果具有可重复性；
5. 原始数据完整保存；
6. 自动完成数据汇总与绘图；
7. 测试代码能够体现 CPU 计算、多核并行、Cache/内存访问等体系结构特征；
8. 最终数据能够直接用于课程实验报告。

---

## 2. Core Principles

### 2.1 Same Source Code

macOS 和 Windows 必须使用同一套 Benchmark 源代码。

禁止维护：

- macOS 专用 Benchmark 核心算法；
- Windows 专用 Benchmark 核心算法；
- 针对 Apple M5 单独编写的性能优化算法；
- 针对 i5-12400F 单独编写的性能优化算法。

允许存在必要的平台检测与文件路径处理代码，但不得改变 Benchmark 的核心计算逻辑。

最终两个平台必须执行逻辑等价的算法。

---

### 2.2 CPU Only

所有 Toy Benchmark 只能运行在 CPU 上。

禁止使用：

- CUDA
- Metal
- OpenCL
- Apple Accelerate
- Intel MKL
- GPU Compute Shader
- GPU 计算
- 其他硬件专用高性能计算库

矩阵乘法、排序、数组访问等核心算法必须由项目自行实现。

---

### 2.3 Reproducibility

所有测试必须能够复现。

涉及随机数据时必须使用固定随机种子，例如：

```cpp
std::mt19937 rng(12345);
```

相同 Benchmark、相同 problem size 在两个平台上必须获得一致的输入数据。

每项 Benchmark 应支持：

- warmup 次数；
- repeat 次数；
- problem size；
- thread count。

默认：

```text
warmup = 1
repeat = 5
```

每一次运行的原始结果都必须保存。

禁止只保存最终平均值。

---

## 3. Compiler Consistency

### 3.1 General Principle

Apple M5 使用 ARM64，而 i5-12400F 使用 x86-64，因此两个平台最终生成的机器指令不可能完全相同。

本实验不要求二进制代码完全一致。

实验需要控制的是：

```text
Same Source Code
Same Algorithm
Same Input
Same Workload
Same C++ Standard
Comparable Compiler Strategy
```

目标是尽量保证两个平台测试的是同一个高级语言程序，而不是两个经过完全不同平台优化后的程序。

---

### 3.2 Compiler Optimization Control

Coding Agent 可以适当选择或禁用编译选项，以减少编译器对原始 Benchmark 代码结构造成过大的改变。

重点控制：

```text
Auto Vectorization
Loop Unrolling
Fast Math
Architecture-Specific Optimization
Link Time Optimization
Profile Guided Optimization
```

默认禁止：

```text
-march=native
-mcpu=native
-mtune=native
-flto
-ffast-math
PGO
```

Windows/MSVC 环境下也禁止主动启用等价选项，例如：

```text
/arch:AVX2
/GL
/LTCG
PGO
```

禁止针对不同平台分别开启：

```text
M5:
NEON-specific manual optimization

12400F:
AVX / AVX2-specific manual optimization
```

---

### 3.3 Baseline Compiler Profile

项目只需要一种正式实验使用的编译模式：

```text
baseline
```

不存在额外的 optimized 实验模式。

Baseline 的目标：

```text
保持正常编译性能
+
减少明显改变原始算法结构的自动优化
+
避免针对具体 CPU 的平台特化
```

对于 Clang / GCC，可以考虑：

```text
-O1 或 -O2
-fno-vectorize
-fno-slp-vectorize
-fno-unroll-loops
-fno-fast-math
```

Coding Agent 应根据实际编译器支持情况确定最终参数。

macOS 和 Windows 必须尽可能采用语义一致的编译策略。

禁止出现：

```text
M5: -O3
12400F: -O0
```

之类明显不可比的配置。

如果 Windows 能够方便使用 LLVM Clang，则优先使用：

```text
macOS: Apple Clang
Windows: LLVM Clang
```

以减少不同编译器实现造成的差异。

如果 Windows 环境只能方便使用 MSVC，也允许使用 MSVC，但需要：

1. 使用与 macOS 尽可能等价的优化策略；
2. 记录具体编译器和编译选项；
3. 在实验报告中注明编译环境差异。

---

### 3.4 Record Compiler Configuration

每次实验必须保存：

```text
Compiler
Compiler Version
C++ Standard
Build Type
Exact Compiler Flags
```

例如：

```text
Compiler: Apple Clang
C++ Standard: C++17
Flags:
-O2
-fno-vectorize
-fno-slp-vectorize
-fno-unroll-loops
-fno-fast-math
```

这样实验结果才能被复现和解释。

---

## 4. Dependency Management

### 4.1 General Rule

Coding Agent 在开发和运行过程中，如果发现缺少必要依赖，应根据当前操作系统自行检查并安装所需依赖。

流程：

```text
Detect OS
↓
Check Dependencies
↓
Install Missing Dependencies
↓
Verify Installation
↓
Continue
```

不要因为缺少基础开发工具就直接停止任务。

---

### 4.2 Required Dependencies

项目可能需要：

```text
Git
CMake
C++ Compiler
Python 3
pandas
matplotlib
```

只安装本项目真正需要的依赖。

禁止引入大量与实验无关的软件包。

---

### 4.3 macOS

首先检查：

```bash
git --version
cmake --version
clang++ --version
python3 --version
```

如果缺少 Apple Command Line Tools：

```bash
xcode-select --install
```

如果系统已经安装 Homebrew，可根据需要使用：

```bash
brew install cmake
brew install python
```

禁止重复安装已经满足要求的软件。

Python 推荐使用虚拟环境：

```bash
python3 -m venv .venv
source .venv/bin/activate
python -m pip install -r requirements.txt
```

---

### 4.4 Windows

首先检查：

```powershell
git --version
cmake --version
clang++ --version
python --version
```

如果系统存在 `winget`，可以优先使用系统包管理器安装缺失组件。

可能需要：

```text
Git
CMake
LLVM
Python
```

如果已经存在：

```text
Visual Studio Build Tools
MSVC
LLVM
CMake
Python
```

则直接复用。

不要重复安装。

Python 推荐：

```powershell
python -m venv .venv
```

然后：

```powershell
python -m pip install -r requirements.txt
```

---

### 4.5 requirements.txt

仓库提供：

```text
requirements.txt
```

至少包含：

```text
pandas
matplotlib
```

C++ Benchmark 核心原则上只依赖：

```text
C++ Standard Library
```

不要引入 Boost 或其他大型 C++ 第三方库。

---

### 4.6 Dependency Safety

依赖安装遵循：

```text
Minimum Dependencies
Reuse Existing Environment
Use Mainstream Package Managers
Avoid Unnecessary System Changes
```

如果安装操作：

```text
需要管理员权限
会覆盖已有 Compiler
会修改重要系统设置
```

优先使用影响更小的方案。

不要破坏用户已有开发环境。

---

## 5. Technology Stack

### 5.1 C++

统一使用：

```text
C++17
```

优先使用标准库。

计时统一使用：

```cpp
std::chrono::steady_clock
```

禁止两个平台分别采用不同的系统计时 API。

---

### 5.2 Build System

使用：

```text
CMake
```

基本构建方式：

```bash
cmake -S . -B build
cmake --build build --config Release
```

必须支持：

```text
macOS + Apple Clang
Windows + LLVM Clang 或 MSVC
```

用户不应通过修改源代码切换平台。

---

### 5.3 Python

Python 只负责：

```text
CSV 数据读取
数据汇总
Mean
Standard Deviation
Speedup
Parallel Efficiency
Plotting
```

Python 不参与 Benchmark 核心性能测试。

使用：

```text
pandas
matplotlib
```

---

## 6. Repository Structure

仓库建议：

```text
cpu-benchmark/
├── AGENTS.md
├── README.md
├── CMakeLists.txt
├── requirements.txt
├── .gitignore
│
├── src/
│   ├── main.cpp
│   ├── platform.cpp
│   ├── benchmark_runner.cpp
│   │
│   └── benchmarks/
│       ├── sieve.cpp
│       ├── quicksort.cpp
│       ├── matrix_mul.cpp
│       └── memory_access.cpp
│
├── include/
│   ├── platform.h
│   ├── benchmark.h
│   └── benchmark_runner.h
│
├── scripts/
│   ├── analyze.py
│   └── plot.py
│
├── results/
│   ├── macos-arm64/
│   │   ├── raw/
│   │   └── summary/
│   │
│   ├── windows-x86_64/
│   │   ├── raw/
│   │   └── summary/
│   │
│   └── comparison/
│       ├── summary/
│       └── figures/
│
└── docs/
```

禁止提交：

```text
build/
.venv/
```

---

## 7. Automatic Platform Detection

程序启动后自动识别：

```text
Operating System
CPU Architecture
```

至少区分：

```text
macos-arm64
windows-x86_64
```

Apple M5 输出：

```text
results/macos-arm64/raw/
```

i5-12400F 输出：

```text
results/windows-x86_64/raw/
```

程序自动创建目录。

禁止通过 CPU 名称硬编码判断平台。

---

## 8. Benchmark Design

本项目实现以下 Toy Benchmark。

---

### 8.1 Sieve of Eratosthenes

自行实现埃拉托斯特尼筛法。

测试侧重：

```text
Integer Operations
Loops
Array Access
Basic CPU Execution
```

Problem Size：

```text
1,000,000
5,000,000
10,000,000
50,000,000
```

记录：

```text
benchmark
problem_size
run
execution_time_ms
checksum
```

checksum 可以使用：

```text
prime count
```

---

### 8.2 Quicksort

自行实现 Quicksort。

禁止直接使用：

```cpp
std::sort
```

输入使用固定随机数种子：

```cpp
std::mt19937 rng(12345);
```

主要观察：

```text
Integer Comparison
Branch
Swap
Control Flow
Cache / Memory Access
```

Problem Size：

```text
100,000
1,000,000
5,000,000
10,000,000
```

测试完成：

```text
Validate Sorted Result
Calculate Checksum
```

---

### 8.3 Matrix Multiplication

自行实现普通矩阵乘法。

固定基础算法：

```cpp
for (int i = 0; i < N; ++i)
    for (int j = 0; j < N; ++j)
        for (int k = 0; k < N; ++k)
            C[i * N + j] +=
                A[i * N + k] * B[k * N + j];
```

禁止使用：

```text
SIMD Intrinsic
NEON
AVX
AVX2
BLAS
Apple Accelerate
Intel MKL
CUDA
Metal
```

测试：

```text
128
256
512
1024
```

记录：

```text
N
execution_time_ms
GFLOPS
checksum
```

GFLOPS：

```text
GFLOPS
=
2 * N^3
/
execution_time_seconds
/
10^9
```

---

## 9. Multi-Core Benchmark

矩阵乘法增加多线程版本。

使用：

```cpp
std::thread
```

核心算法仍必须保持跨平台一致。

测试：

```text
1 thread
2 threads
4 threads
...
```

程序可以读取：

```cpp
std::thread::hardware_concurrency()
```

但实际线程数量必须记录。

计算：

```text
Speedup(N)
=
T(1) / T(N)
```

以及：

```text
Parallel Efficiency
=
Speedup / N
```

Apple M5 存在异构核心时：

```text
不要手工绑定 P-Core / E-Core
```

统一交给操作系统调度。

---

## 10. Memory / Cache Benchmark

实现：

```text
Sequential Access
Random Access
```

---

### 10.1 Sequential Access

例如：

```text
a[0]
a[1]
a[2]
...
```

用于分析：

```text
Spatial Locality
Cache
Memory Hierarchy
```

---

### 10.2 Random Access

随机访问索引提前生成。

必须使用固定 seed。

随机序列生成过程不得包含在性能计时中。

---

### 10.3 Working Set Size

测试：

```text
32 KB
64 KB
256 KB
1 MB
4 MB
16 MB
64 MB
256 MB
```

根据实际运行时间可以对规模作合理微调，但两个平台最终必须使用相同测试规模。

记录：

```text
access_pattern
working_set_bytes
execution_time_ms
throughput
checksum
```

如果能够严格定义有效带宽，可以输出 bandwidth。

如果无法保证指标含义准确，则不要强行输出 bandwidth。

---

## 11. Benchmark Timing Rules

初始化过程不能包含在核心 execution time 中。

Matrix：

```text
Initialize
↓
Start Timer
↓
Matrix Multiplication
↓
Stop Timer
↓
Checksum
```

Quicksort：

```text
Generate Original Input
↓
Copy Input
↓
Start Timer
↓
Quicksort
↓
Stop Timer
↓
Validation
↓
Checksum
```

Random Access：

```text
Generate Access Sequence
↓
Start Timer
↓
Memory Access
↓
Stop Timer
↓
Checksum
```

不得计入：

```text
File IO
CSV Writing
Random Data Generation
Logging
Metadata Collection
Python Analysis
Plotting
```

---

## 12. Prevent Compiler Removing Workload

必须防止编译器发现结果未使用后删除 Benchmark 的计算。

每个 Benchmark 必须产生：

```text
checksum
```

同时对适合验证的算法进行：

```text
correctness validation
```

不要大量使用：

```cpp
volatile
```

因为这会显著改变 Cache 和 Memory 行为。

优先使用：

```text
Actual Result
+
Checksum
+
Validation
```

保证 workload 对程序而言具有可观察结果。

---

## 13. Raw Data Format

统一使用 CSV。

示例：

```csv
timestamp,platform,arch,benchmark,variant,size,threads,run,time_ms,metric,checksum
2026-09-14T10:00:00,macos,arm64,matrix,serial,512,1,1,512.31,1.02,123456
```

每一次运行单独保存一行。

每次完整 Benchmark 生成独立文件。

macOS：

```text
results/macos-arm64/raw/run_20260914_100000.csv
```

Windows：

```text
results/windows-x86_64/raw/run_20260914_103000.csv
```

禁止覆盖旧数据。

文件名必须同时兼容：

```text
macOS
Windows
```

不要使用 `:` 等 Windows 文件名非法字符。

---

## 14. System Metadata

每次正式实验同时保存：

```text
OS
Architecture
CPU Model
Compiler
Compiler Version
C++ Standard
CMake Build Type
Exact Compiler Flags
Hardware Concurrency
Git Commit
Benchmark Version
```

其中：

```text
Exact Compiler Flags
```

必须记录。

CPU Model 如果能够方便可靠读取则自动记录。

如果跨平台自动获取过于复杂，可以允许配置或人工填写：

```text
Apple M5
Intel Core i5-12400F
```

不要为了读取 CPU 型号引入大型第三方库。

---

## 15. Data Analysis

`scripts/analyze.py` 读取：

```text
results/macos-arm64/raw/
results/windows-x86_64/raw/
```

对于相同：

```text
benchmark
variant
size
threads
compiler configuration
```

计算：

```text
mean
standard deviation
min
max
sample count
```

禁止删除或修改原始 CSV。

Summary 必须能够追溯到 Raw Data。

---

## 16. Cross-Platform Comparison

M5 相对于 i5-12400F 的相对性能：

```text
Speedup_M5
=
T_12400F / T_M5
```

如果：

```text
Speedup_M5 > 1
```

表示：

```text
M5 faster
```

如果：

```text
Speedup_M5 < 1
```

表示：

```text
i5-12400F faster
```

例如：

```text
M5 = 2.0 s
12400F = 3.0 s

Speedup_M5 = 3.0 / 2.0 = 1.5
```

即：

```text
在该 workload 下，M5 的性能约为 i5-12400F 的 1.5 倍。
```

禁止将单位不同的指标直接混合求平均。

---

## 17. Plotting

使用：

```text
matplotlib
```

图片保存：

```text
results/comparison/figures/
```

---

### 17.1 Matrix Execution Time

折线图：

```text
X = Matrix Size
Y = Mean Execution Time
Series = M5 / i5-12400F
```

显示：

```text
Mean ± Standard Deviation
```

---

### 17.2 Matrix GFLOPS

```text
X = Matrix Size
Y = GFLOPS
Series = M5 / i5-12400F
```

---

### 17.3 Multi-Core Scaling

```text
X = Thread Count
Y = Speedup
```

可以同时绘制：

```text
Ideal Speedup: y = x
```

用于和实际结果比较。

---

### 17.4 Parallel Efficiency

```text
X = Thread Count
Y = Parallel Efficiency
```

其中：

```text
Parallel Efficiency = Speedup / Thread Count
```

---

### 17.5 Memory Performance

```text
X = Working Set Size
Y = Execution Time 或 Throughput
```

分别绘制：

```text
Sequential
Random
```

Working Set Size 推荐采用：

```text
log scale
```

---

### 17.6 Overall Comparison

不同 Benchmark 如果需要统一比较，优先使用：

```text
Relative Performance
Speedup
```

禁止把：

```text
seconds
GFLOPS
throughput
Cinebench score
```

等不同单位数据直接放入同一坐标轴。

---

## 18. Output Directory

统一：

```text
results/
├── macos-arm64/
│   ├── raw/
│   └── summary/
│
├── windows-x86_64/
│   ├── raw/
│   └── summary/
│
└── comparison/
    ├── summary/
    └── figures/
```

C++ 负责：

```text
raw/
```

Python 负责：

```text
summary/
comparison/
figures/
```

---

## 19. Command Line Interface

默认运行全部 Toy Benchmark：

```bash
./cpu_benchmark
```

单独运行：

```bash
./cpu_benchmark --benchmark sieve
./cpu_benchmark --benchmark quicksort
./cpu_benchmark --benchmark matrix
./cpu_benchmark --benchmark memory
```

指定重复次数：

```bash
./cpu_benchmark --repeat 5
```

矩阵：

```bash
./cpu_benchmark --benchmark matrix --size 512 --threads 1
```

Windows：

```powershell
.\cpu_benchmark.exe
```

CLI 保持简单，不需要复杂子命令框架。

---

## 20. README Requirements

README 必须说明：

```text
Project Purpose
Hardware Under Test
Dependencies
macOS Setup
Windows Setup
Compiler Configuration
Build
Benchmark Usage
Analysis
Plotting
Output Directory
Git Workflow
Experimental Fairness
```

---

### 20.1 Experimental Fairness

README 明确说明：

```text
Same Source Code
Same Algorithm
Same Input
Same Random Seed
Same Workload
Same C++ Standard
Comparable Compiler Strategy
No CPU-Specific Manual Optimization
Multiple Repetitions
CPU Only
```

必须注明：

```text
ARM64 与 x86-64 最终执行的机器指令必然不同。
```

因此实验比较的是：

```text
相同高级语言 workload 在 Apple M5 与 Intel Core i5-12400F
实际平台上的最终 CPU 性能表现。
```

而不是试图构造完全相同的机器指令。

---

## 21. Git / GitHub Rules

项目通过 GitHub 管理。

`.gitignore` 至少包含：

```text
build/
.venv/
.vscode/
.idea/
.DS_Store
*.exe
*.obj
*.o
__pycache__/
```

不要忽略：

```text
results/
```

因为实验数据需要：

```text
真实
可追溯
可复现
```

每次测试使用独立 CSV，避免两个平台修改相同结果文件产生 Git 冲突。

---

## 22. Coding Style

目标：

```text
simple
readable
cross-platform
reproducible
```

避免：

```text
复杂模板元编程
不必要设计模式
大型第三方库
过度工程化
平台专用核心算法
复杂 GUI
复杂配置系统
```

Benchmark 必须能够让实验报告清楚解释：

```text
程序进行了什么操作
它主要反映 CPU 的什么特性
测试变量是什么
最终指标是什么
```

---

## 23. Experimental Theory Mapping

### 23.1 Performance

```text
Performance ∝ 1 / Execution Time
```

相对性能：

```text
Performance(X) / Performance(Y)
=
ExecutionTime(Y) / ExecutionTime(X)
```

---

### 23.2 CPU Performance Equation

```text
CPU Time
=
Instruction Count × CPI × Clock Cycle Time
```

等价：

```text
CPU Time
=
Instruction Count × CPI / Clock Rate
```

本项目不要求直接测量：

```text
Instruction Count
CPI
```

实验报告可以使用该公式解释：

```text
为什么 CPU 主频不是决定性能的唯一因素。
```

---

### 23.3 Amdahl's Law

```text
Speedup
=
1 / ((1 - P) + P / N)
```

用于分析：

```text
Multi-Thread Matrix Multiplication
```

重点解释：

```text
线程数增加后为什么性能不能无限线性提升。
```

---

### 23.4 Locality

Memory Benchmark 对应：

```text
Temporal Locality
Spatial Locality
```

重点比较：

```text
Sequential Access
Random Access
```

以及 Working Set Size 增大后的性能变化。

---

## 24. Experimental Conclusion Constraint

禁止根据实验结果直接得出：

```text
ARM > x86
```

或：

```text
x86 > ARM
```

也禁止得出：

```text
Apple M5 架构天然优于 i5-12400F
```

等过度泛化结论。

实验能够支持的结论应限定为：

```text
在本实验规定的源代码、算法、输入数据、编译器配置、
操作系统和 workload 下，Apple M5 与 Intel Core i5-12400F
表现出相应的性能差异。
```

---

## 25. Required Deliverables

本项目最终只需要完成以下内容：

```text
[1] 跨平台 CMake 工程

[2] macOS / Windows 自动平台检测

[3] 自动结果目录分流

[4] 必要依赖检查与安装

[5] Baseline Compiler Profile

[6] Compiler Metadata 记录

[7] Sieve Benchmark

[8] Quicksort Benchmark

[9] Single-Thread Matrix Multiplication

[10] Multi-Thread Matrix Multiplication

[11] Sequential Memory Access

[12] Random Memory Access

[13] Working Set Size Sweep

[14] CSV Raw Data

[15] System Metadata

[16] Mean

[17] Standard Deviation

[18] Min / Max

[19] Speedup

[20] Parallel Efficiency

[21] matplotlib 绘图

[22] Cross-Platform Comparison

[23] README

[24] GitHub 数据管理
```

本项目到此为止。

不要添加额外实验。

---

## 26. Explicitly Out of Scope

禁止加入：

```text
GPU Benchmark
Disk Benchmark
Network Benchmark

CUDA
Metal
OpenCL

NEON Manual Optimization
AVX Manual Optimization
AVX2 Manual Optimization

Apple Accelerate
Intel MKL
BLAS

perf
Hardware Performance Counters

Energy Benchmark
Power Benchmark

Compiler Optimization Comparison Experiment
-O3 vs -O2 Experiment
Vectorization Comparison Experiment
ISA Optimization Experiment

LTO Experiment
PGO Experiment

GUI
Web Frontend
Database

Additional Hardware Platforms

Additional CPU Models

ARM vs x86 General Architecture Benchmark
```

不要为这些功能预留复杂架构。

不要实现“以后可能会用”的模块。

只完成当前课程实验所需要的功能。

---

## 27. Implementation Order

### Step 1

检查当前 OS：

```text
macOS
Windows
```

检查：

```text
Git
CMake
Compiler
Python
```

缺少必要依赖时根据 OS 安装。

安装后验证。

---

### Step 2

建立：

```text
CMakeLists.txt
src/
include/
scripts/
results/
requirements.txt
README.md
.gitignore
```

---

### Step 3

实现：

```text
OS Detection
Architecture Detection
Automatic Results Directory
```

正确识别：

```text
macos-arm64
windows-x86_64
```

---

### Step 4

确定正式实验唯一使用的：

```text
Baseline Compiler Profile
```

要求：

```text
No Architecture-Specific Optimization
No -march=native
No LTO
No PGO
No Fast Math
Control Automatic Vectorization
Control Automatic Loop Unrolling
```

保存实际 Compiler Flags。

---

### Step 5

实现统一 Benchmark Runner：

```text
Timer
Warmup
Repeat
CLI
CSV
Checksum
Metadata
```

---

### Step 6

实现：

```text
Sieve
Quicksort
```

确保：

```text
Same Input
Correctness Validation
Checksum
```

---

### Step 7

实现：

```text
Single-Thread Matrix Multiplication
```

记录：

```text
Execution Time
GFLOPS
Checksum
```

---

### Step 8

实现：

```text
Multi-Thread Matrix Multiplication
```

记录：

```text
Execution Time
Speedup
Parallel Efficiency
```

---

### Step 9

实现：

```text
Sequential Memory Access
Random Memory Access
Working Set Size Sweep
```

随机索引必须提前生成。

---

### Step 10

检查：

```text
Checksum
Correctness
Compiler Optimization Protection
Timing Range
Input Consistency
```

---

### Step 11

实现：

```text
scripts/analyze.py
```

输出：

```text
Mean
Standard Deviation
Min
Max
Sample Count
Speedup
Parallel Efficiency
```

---

### Step 12

实现：

```text
scripts/plot.py
```

生成：

```text
Matrix Size vs Execution Time
Matrix Size vs GFLOPS
Thread Count vs Speedup
Thread Count vs Parallel Efficiency
Working Set Size vs Memory Performance
M5 vs i5-12400F Relative Performance
```

---

### Step 13

完善 README：

```text
Dependencies
macOS Setup
Windows Setup
Build
Compiler Configuration
Run
Analysis
Plotting
Git Workflow
Experimental Fairness
```

---

### Step 14

验证当前能够访问的平台。

如果当前处于 macOS：

```text
实际完成并运行 macOS 测试。
```

如果当前处于 Windows：

```text
实际完成并运行 Windows 测试。
```

如果 Coding Agent 当前无法访问另一个 OS：

```text
检查跨平台代码和 CMake 逻辑，
但禁止伪造另一个平台的测试结果。
```

两个平台的最终实测结果必须来自对应真实设备。

---

## 28. Final Workflow

### macOS / Apple M5

```text
git clone / git pull
↓
Check Dependencies
↓
Install Missing Dependencies If Necessary
↓
CMake Configure
↓
CMake Build
↓
Run Benchmark
↓
results/macos-arm64/raw/
↓
git add
↓
git commit
↓
git push
```

---

### Windows / i5-12400F

```text
git pull
↓
Check Dependencies
↓
Install Missing Dependencies If Necessary
↓
CMake Configure
↓
CMake Build
↓
Run Benchmark
↓
results/windows-x86_64/raw/
↓
git add
↓
git commit
↓
git push
```

---

### Final Analysis

当 GitHub 已包含两个平台的原始测试数据后：

```text
git pull
↓
python scripts/analyze.py
↓
python scripts/plot.py
```

生成：

```text
results/comparison/summary/
results/comparison/figures/
```

最终工作流：

```text
Same GitHub Repository
↓
Same C++ Source Code
↓
Compile Separately on ARM64 and x86-64
↓
Run Same Workloads
↓
Automatically Save Raw Results by Platform
↓
Push Results to GitHub
↓
Combine Data
↓
Python Statistical Analysis
↓
Python Plotting
↓
Use Results in Experiment Report
```

---

## 29. Priority

优先级：

```text
1. Benchmark Fairness

2. Same Source Code

3. Same Algorithm

4. Same Workload

5. Compiler Consistency

6. Reproducibility

7. Cross-Platform Compatibility

8. Raw Data Integrity

9. Correctness

10. Statistical Analysis

11. Plot Quality

12. Engineering Appearance
```

发生冲突时：

```text
Fairness > Maximum Performance
```

```text
Reproducibility > Convenience
```

```text
Simple and Explainable Code > Complex Engineering
```

```text
Current Experiment Requirements > Future Extensibility
```

本项目不考虑未来扩展需求。