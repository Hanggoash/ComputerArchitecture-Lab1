# 项目状态

更新时间：2026-09-14

## 当前完成情况

- 已从空目录建立跨平台 CMake / C++17 CPU Toy Benchmark 工程。
- 已实现并构建：埃拉托斯特尼筛法、项目自实现 Hoare Quicksort、单线程与 `std::thread` 多线程普通矩阵乘法、顺序与随机内存访问。
- 已实现 macOS / Windows 与 ARM64 / x86-64 自动识别，以及按平台写入独立结果目录。
- 已实现 CSV raw data、逐次运行 JSON metadata、checksum 与算法正确性检查。
- 已实现 `scripts/analyze.py`：均值、标准差、最小值、最大值、样本数、矩阵 speedup 与 parallel efficiency、跨平台 speedup。
- 已实现 `scripts/plot.py`：矩阵时间、GFLOPS、多核 speedup、并行效率、内存性能和跨平台相对性能图。
- 已完成 README、依赖清单与 `.gitignore`。

## 编译配置

唯一正式配置为 `baseline`：

```text
Apple Clang 21.0.0.21000101
C++17
Release
-O2
-fno-vectorize
-fno-slp-vectorize
-fno-unroll-loops
-fno-fast-math
```

未启用 `-march=native`、`-mcpu=native`、LTO、PGO、fast-math、BLAS、GPU 或 CPU 专用手写 SIMD 优化。

## 已验证的实测数据

设备自动识别结果：Apple M5、macOS / ARM64、10 个硬件线程。

- CMake 配置与 Release 构建成功。
- 四类 benchmark 已完成小规模功能验证。
- 默认正式 workload 已于 2026-09-14 完成，raw CSV 为：
  `results/macos-arm64/raw/run_20260914_102022_806.csv`
- 正式数据共 160 行：sieve 20、quicksort 20、matrix 40、memory 80。
- 每个 benchmark 配置均有 5 次正式重复，且同一配置 checksum 一致。
- 已从两平台正式数据生成 summary、comparison CSV 与全部 6 张 comparison 图。

另有 9 条小规模冒烟测试记录保留在 `results/macos-arm64/raw/smoke/`，用于追溯构建期验证。它们不参与正式数据汇总，不覆盖或篡改任何原始数据。

### Windows / i5-12400F 实测

- Windows / x86-64 正式数据为：`results/windows-x86_64/raw/run_20260914_120935_423.csv`。
- 正式数据共 160 行：sieve 20、quicksort 20、matrix 40、memory 80。
- 每个 benchmark 配置均有 5 次正式重复，且同一配置 checksum 一致。
- Windows 使用 GNU 16.1.0、C++17、Release，以及禁用自动向量化、循环展开和 fast-math 的 baseline 语义配置。
- metadata 的 CPU 自动识别字段为 `unavailable`；测试设备型号需在实验报告中按实际设备注明为 Intel Core i5-12400F。

### 当前分析产物

- 两边各 160 条正式 raw data 已成功汇总为各 32 个统计配置。
- 共有 31 个可直接匹配的跨平台配置；多线程矩阵仅比较共同线程数 2、4、8，因为 M5 的硬件并发数为 10、Windows 为 12。
- 跨平台汇总位于 `results/comparison/summary/comparison.csv`，图表位于 `results/comparison/figures/`。

## 本地依赖

- Apple Command Line Tools / Apple Clang：可用。
- Git：可用。
- CMake 4.4.3：已安装到用户目录；当前环境通过 `arch -arm64 /Users/hanggoash/Library/Python/3.9/bin/cmake` 调用。
- Python 3.9.6：可用。
- `.venv`：已安装 pandas 2.3.3 和 matplotlib 3.9.4。

## 后续事项

1. 将 comparison summary 和 figures 用于课程实验报告，并将结论限制在规定 workload、编译配置与操作系统范围内。
2. 在报告中注明工具链差异：macOS 为 Apple Clang，Windows 为 GNU，而非同一 Clang 版本。
3. 如需让 metadata 完整记录 Windows CPU 型号，可在 Windows 真机补充可靠的 CPU 型号读取方式后重新运行正式测试；不得修改既有 raw data。

## Git 状态

当前工作目录已初始化为 Git 仓库，默认分支为 `main`，远程仓库为 `origin`（`https://github.com/Hanggoash/Lab1.git`）。

macOS raw data 是在仓库初始化前生成的，因此其中的 `git_commit` 字段如实保留为 `unavailable`。后续重新 CMake 配置并运行 benchmark 时，metadata 会自动记录当时的 Git commit。
