# CPU Toy Benchmark — Apple M5 vs. Intel Core i5-12400F

本项目是《计算机体系结构》课程的系统性能评价实验框架。它只比较两颗 CPU：Apple M5（macOS / ARM64）与 Intel Core i5-12400F（Windows / x86-64）。同一份 C++17 源代码在两台真实机器上分别编译和运行，保存每一次测试的原始结果，再用 Python 汇总和绘图。

测试范围严格限于 CPU：筛法、快速排序、普通矩阵乘法、多线程矩阵乘法，以及顺序/随机内存访问。项目不包含 GPU、CUDA、Metal、OpenCL、BLAS、MKL、Accelerate、SIMD intrinsic、性能计数器或能耗测试。

## 实验公平性

- 相同源代码、算法、输入、随机种子（`std::mt19937(12345)`）、workload 和 C++ 标准（C++17）。
- 矩阵乘法保持固定的 `i → j → k` 三重循环；排序为项目自身实现的 Hoare quicksort，不使用 `std::sort`。
- 只有一个正式编译配置：`baseline`。Clang/GCC 使用 `-O2`，禁用自动向量化、SLP 向量化、循环展开和 fast-math；未使用 `-march=native`、`-mcpu=native`、LTO 或 PGO。Windows 优先使用 LLVM Clang；MSVC 会使用最接近的 `/O2 /fp:precise /GL-` 配置，并把实际配置记入元数据。
- 每项测试默认预热 1 次、正式重复 5 次。初始化、随机数据生成、输入复制、校验、CSV 写入和日志均不计入核心计时。
- ARM64 与 x86-64 最终执行的机器指令必然不同。本实验比较的是在规定源代码、workload、编译策略和操作系统下，Apple M5 与 i5-12400F 的实际 CPU 表现；不能据此泛化为“ARM 优于 x86”或相反。

## 依赖

- Git（用于同步代码和记录提交版本）
- CMake 3.20+
- 支持 C++17 的编译器：macOS 使用 Apple Clang；Windows 优先 LLVM Clang，也支持 MSVC
- Python 3 与 `pandas`、`matplotlib`

本项目的 C++ 核心只依赖标准库。

### macOS 设置

先检查已有环境：

```bash
git --version
cmake --version
clang++ --version
python3 --version
```

若缺少 Command Line Tools，执行 `xcode-select --install`。若已安装 Homebrew，可按需执行 `brew install cmake python`。建议使用虚拟环境：

```bash
python3 -m venv .venv
source .venv/bin/activate
python -m pip install -r requirements.txt
```

### Windows 设置

在 PowerShell 中检查：

```powershell
git --version
cmake --version
clang++ --version
python --version
```

缺少组件时优先通过 `winget` 安装 Git、CMake、LLVM、Python，或复用已有 Visual Studio Build Tools。建议：

```powershell
python -m venv .venv
.\.venv\Scripts\Activate.ps1
python -m pip install -r requirements.txt
```

## 构建

在仓库根目录执行。macOS / Linux 单配置生成器：

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Windows（LLVM Clang 示例）：

```powershell
cmake -S . -B build -G Ninja -DCMAKE_CXX_COMPILER=clang++
cmake --build build --config Release
```

若使用 Visual Studio/MSVC，可省略 `-G Ninja -DCMAKE_CXX_COMPILER=clang++`。配置日志会显示唯一的 baseline flags，程序也会把编译器、版本、C++ 标准、构建类型和精确 flags 写入每次运行的 JSON 元数据。

## 运行基准

默认运行全部正式 workload：

```bash
./build/cpu_benchmark
```

Windows：

```powershell
.\build\Release\cpu_benchmark.exe
```

常用例子：

```bash
./build/cpu_benchmark --benchmark sieve --repeat 5
./build/cpu_benchmark --benchmark quicksort --size 1000000
./build/cpu_benchmark --benchmark matrix --size 512 --threads 1
./build/cpu_benchmark --benchmark matrix --size 512 --threads 4
./build/cpu_benchmark --benchmark memory --size 67108864
```

参数说明：

- `--benchmark all|sieve|quicksort|matrix|memory`
- `--warmup N`，默认 `1`（可设为 `0` 以进行快速功能验证）
- `--repeat N`，默认 `5`
- `--size N`：sieve/quicksort 为元素数，matrix 为矩阵维度，memory 为字节数
- `--threads N`：矩阵线程数；未指定时，默认运行单线程尺寸扫描，并在 `N=512` 上进行 2、4、…、硬件并发数的多线程扫描

正式默认 workload 为：

| Benchmark | Workload |
| --- | --- |
| Sieve | 1M, 5M, 10M, 50M |
| Quicksort | 100K, 1M, 5M, 10M |
| Matrix | 128, 256, 512, 1024；多线程扩展在 512 |
| Memory | 32KB, 64KB, 256KB, 1MB, 4MB, 16MB, 64MB, 256MB；每个尺寸均测顺序与随机访问 |

矩阵输出 `GFLOPS = 2N³ / seconds / 10⁹`。内存访问输出的是固定计数访问的 `million_accesses_per_s`，不将它不严谨地称作硬件带宽。

## 输出与数据管理

程序自动根据操作系统和架构分流，且绝不覆盖旧数据：

```text
results/
├── macos-arm64/raw/run_YYYYMMDD_HHMMSS_mmm.csv
├── macos-arm64/raw/metadata_YYYYMMDD_HHMMSS_mmm.json
├── windows-x86_64/raw/run_YYYYMMDD_HHMMSS_mmm.csv
├── windows-x86_64/raw/metadata_YYYYMMDD_HHMMSS_mmm.json
├── macos-arm64/summary/summary.csv
├── windows-x86_64/summary/summary.csv
└── comparison/
    ├── summary/comparison.csv
    └── figures/
```

每个 raw CSV 的一行对应一次正式重复，字段包含时间、平台、架构、benchmark、variant、size、threads、run、time、metric、metric 名称和 checksum。相邻 JSON 同时记录 OS、CPU 型号、硬件并发数、编译器、版本、C++ 标准、build type、精确 flags、Git commit 与 benchmark version。结果目录不在 `.gitignore` 中，应提交真实 raw 数据与分析产物；不要提交 `build/` 或 `.venv/`。

macOS 和 Windows 应分别在对应的真实设备上运行并提交各自新增的 CSV/JSON。当前设备未运行的平台不得伪造数据。

## 分析与绘图

两平台原始数据都同步到仓库后运行：

```bash
.venv/bin/python scripts/analyze.py
.venv/bin/python scripts/plot.py
```

Windows 激活虚拟环境后可用：

```powershell
python scripts\analyze.py
python scripts\plot.py
```

`analyze.py` 只读取 raw CSV，不会修改或删除它们。它为相同平台、编译配置、benchmark、variant、size、threads 和 metric 汇总 mean、standard deviation、min、max、sample count 和平均 metric；对于矩阵还计算 `Speedup = T(1) / T(N)` 与 `Parallel Efficiency = Speedup / N`。

当两平台都有匹配 workload 时，`results/comparison/summary/comparison.csv` 计算：

```text
Speedup_M5 = T_i5-12400F / T_M5
```

大于 1 表示 M5 在该 workload 下更快，小于 1 表示 i5-12400F 更快。分析输出保留两边的编译配置，便于报告解释可能存在的工具链差异。

`plot.py` 会生成：矩阵时间（mean ± standard deviation）、矩阵 GFLOPS、多核 speedup、并行效率、顺序/随机内存性能（工作集对数横轴）与跨平台相对性能图。缺少某一平台的数据时，相关跨平台图会明确跳过，不会伪造结果。

## 推荐 Git 工作流

1. 两台机器均从同一提交 `git pull`。
2. 各自在真实硬件上构建和运行；只新增本平台的 raw CSV/JSON。
3. 提交并推送结果数据。
4. 汇总两边数据后运行分析和绘图，提交 summary 与 figures。
5. 报告中的结论限定在本实验的源码、算法、输入、编译配置、操作系统和 workload 范围内。
