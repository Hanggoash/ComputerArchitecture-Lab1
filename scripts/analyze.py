#!/usr/bin/env python3
"""Summarize immutable raw benchmark CSV files and create report-ready comparisons."""

from __future__ import annotations

import json
from pathlib import Path

import pandas as pd


ROOT = Path(__file__).resolve().parents[1]
PLATFORM_KEYS = ("macos-arm64", "windows-x86_64")
REQUIRED_COLUMNS = {
    "timestamp",
    "platform",
    "arch",
    "benchmark",
    "variant",
    "size",
    "threads",
    "run",
    "time_ms",
    "metric",
    "metric_name",
    "checksum",
}
SUMMARY_COLUMNS = [
    "platform",
    "arch",
    "benchmark",
    "variant",
    "size",
    "threads",
    "metric_name",
    "compiler_config",
]


def metadata_for(raw_file: Path) -> dict[str, object]:
    """Load metadata written with a raw CSV; tolerate manually supplied CSVs."""
    suffix = raw_file.stem[4:] if raw_file.stem.startswith("run_") else raw_file.stem
    metadata_file = raw_file.with_name(f"metadata_{suffix}.json")
    if not metadata_file.exists():
        return {}
    try:
        with metadata_file.open(encoding="utf-8") as handle:
            return json.load(handle)
    except (OSError, json.JSONDecodeError) as error:
        print(f"Warning: could not read {metadata_file}: {error}")
        return {}


def config_label(metadata: dict[str, object]) -> str:
    compiler = str(metadata.get("compiler", "unknown"))
    version = str(metadata.get("compiler_version", "unknown"))
    standard = str(metadata.get("cxx_standard", "unknown"))
    build_type = str(metadata.get("build_type", "unknown"))
    flags = str(metadata.get("compiler_flags", "unknown"))
    return f"{compiler} {version}; {standard}; {build_type}; {flags}"


def load_raw(platform_key: str) -> pd.DataFrame:
    raw_dir = ROOT / "results" / platform_key / "raw"
    frames: list[pd.DataFrame] = []
    for raw_file in sorted(raw_dir.glob("run_*.csv")):
        try:
            frame = pd.read_csv(raw_file)
        except (OSError, pd.errors.ParserError) as error:
            print(f"Warning: skipping unreadable {raw_file}: {error}")
            continue
        missing = REQUIRED_COLUMNS.difference(frame.columns)
        if missing:
            print(f"Warning: skipping {raw_file}; missing columns: {sorted(missing)}")
            continue
        metadata = metadata_for(raw_file)
        frame["compiler_config"] = config_label(metadata)
        frame["compiler_flags"] = str(metadata.get("compiler_flags", "unknown"))
        frame["raw_file"] = raw_file.name
        frames.append(frame)
    if not frames:
        return pd.DataFrame(columns=list(REQUIRED_COLUMNS) + ["compiler_config", "compiler_flags", "raw_file"])
    return pd.concat(frames, ignore_index=True)


def add_scaling_metrics(summary: pd.DataFrame) -> pd.DataFrame:
    """Attach T(1)/T(N) and efficiency to matrix rows when a baseline exists."""
    if summary.empty:
        summary["speedup"] = pd.Series(dtype=float)
        summary["parallel_efficiency"] = pd.Series(dtype=float)
        return summary

    matrix = summary.loc[summary["benchmark"] == "matrix"].copy()
    baseline_columns = ["platform", "arch", "size", "compiler_config"]
    baselines = matrix.loc[(matrix["variant"] == "serial") & (matrix["threads"] == 1), baseline_columns + ["time_mean_ms"]]
    baselines = baselines.rename(columns={"time_mean_ms": "serial_time_mean_ms"})
    result = summary.merge(baselines, on=baseline_columns, how="left")
    is_matrix = result["benchmark"] == "matrix"
    result["speedup"] = pd.NA
    result.loc[is_matrix, "speedup"] = (
        result.loc[is_matrix, "serial_time_mean_ms"] / result.loc[is_matrix, "time_mean_ms"]
    )
    result["parallel_efficiency"] = pd.NA
    result.loc[is_matrix, "parallel_efficiency"] = (
        result.loc[is_matrix, "speedup"] / result.loc[is_matrix, "threads"]
    )
    return result.drop(columns=["serial_time_mean_ms"])


def summarize(raw: pd.DataFrame) -> pd.DataFrame:
    if raw.empty:
        empty = pd.DataFrame(columns=SUMMARY_COLUMNS + [
            "time_mean_ms", "time_std_ms", "time_min_ms", "time_max_ms", "sample_count", "metric_mean", "metric_std"
        ])
        return add_scaling_metrics(empty)

    grouped = raw.groupby(SUMMARY_COLUMNS, dropna=False, as_index=False).agg(
        time_mean_ms=("time_ms", "mean"),
        time_std_ms=("time_ms", "std"),
        time_min_ms=("time_ms", "min"),
        time_max_ms=("time_ms", "max"),
        sample_count=("time_ms", "count"),
        metric_mean=("metric", "mean"),
        metric_std=("metric", "std"),
    )
    return add_scaling_metrics(grouped)


def build_comparison(summaries: dict[str, pd.DataFrame]) -> pd.DataFrame:
    mac = summaries["macos-arm64"].copy()
    windows = summaries["windows-x86_64"].copy()
    if mac.empty or windows.empty:
        return pd.DataFrame(columns=[
            "benchmark", "variant", "size", "threads", "metric_name", "m5_time_mean_ms", "i5_time_mean_ms", "speedup_m5"
        ])

    key_columns = ["benchmark", "variant", "size", "threads", "metric_name"]
    mac = mac.rename(columns={
        "time_mean_ms": "m5_time_mean_ms",
        "time_std_ms": "m5_time_std_ms",
        "time_min_ms": "m5_time_min_ms",
        "time_max_ms": "m5_time_max_ms",
        "sample_count": "m5_sample_count",
        "metric_mean": "m5_metric_mean",
        "metric_std": "m5_metric_std",
        "speedup": "m5_parallel_speedup",
        "parallel_efficiency": "m5_parallel_efficiency",
        "compiler_config": "m5_compiler_config",
    })
    windows = windows.rename(columns={
        "time_mean_ms": "i5_time_mean_ms",
        "time_std_ms": "i5_time_std_ms",
        "time_min_ms": "i5_time_min_ms",
        "time_max_ms": "i5_time_max_ms",
        "sample_count": "i5_sample_count",
        "metric_mean": "i5_metric_mean",
        "metric_std": "i5_metric_std",
        "speedup": "i5_parallel_speedup",
        "parallel_efficiency": "i5_parallel_efficiency",
        "compiler_config": "i5_compiler_config",
    })
    platform_columns = [
        "time_mean_ms",
        "time_std_ms",
        "time_min_ms",
        "time_max_ms",
        "sample_count",
        "metric_mean",
        "metric_std",
        "parallel_speedup",
        "parallel_efficiency",
        "compiler_config",
    ]
    mac = mac[key_columns + [f"m5_{column}" for column in platform_columns]]
    windows = windows[key_columns + [f"i5_{column}" for column in platform_columns]]
    comparison = mac.merge(windows, on=key_columns, how="inner")
    comparison["speedup_m5"] = comparison["i5_time_mean_ms"] / comparison["m5_time_mean_ms"]
    return comparison.sort_values(key_columns).reset_index(drop=True)


def write_comparison_files(comparison: pd.DataFrame) -> dict[str, pd.DataFrame]:
    """Split the matched workloads into report-oriented comparison CSV files."""
    comparison_dir = ROOT / "results" / "comparison" / "summary"
    comparison_dir.mkdir(parents=True, exist_ok=True)
    outputs = {
        "sieve_comparison.csv": comparison.loc[comparison["benchmark"] == "sieve"],
        "quicksort_comparison.csv": comparison.loc[comparison["benchmark"] == "quicksort"],
        "matrix_single_comparison.csv": comparison.loc[
            (comparison["benchmark"] == "matrix") & (comparison["variant"] == "serial")
        ],
        "matrix_multi_comparison.csv": comparison.loc[
            (comparison["benchmark"] == "matrix") & (comparison["variant"] == "parallel")
        ],
        "memory_comparison.csv": comparison.loc[comparison["benchmark"] == "memory"],
        "overall_comparison.csv": comparison,
    }
    for filename, frame in outputs.items():
        output_file = comparison_dir / filename
        frame.to_csv(output_file, index=False)
        print(f"{filename}: {len(frame)} matched rows: {output_file}")

    # This was the pre-report-layout aggregate. `overall_comparison.csv` is its
    # explicit replacement; removing it avoids two files with identical scope.
    legacy_file = comparison_dir / "comparison.csv"
    if legacy_file.exists():
        legacy_file.unlink()
        print(f"Removed obsolete {legacy_file}")
    return outputs


def markdown_table(headers: list[str], rows: list[list[str]]) -> str:
    header = "| " + " | ".join(headers) + " |"
    separator = "| " + " | ".join("---" for _ in headers) + " |"
    body = ["| " + " | ".join(row) + " |" for row in rows]
    return "\n".join([header, separator, *body])


def cell_value(row: pd.Series | object, column: str) -> float:
    """Read a named field from either a Series or an itertuples row."""
    if isinstance(row, pd.Series):
        return float(row[column])
    return float(getattr(row, column))


def format_mean_std(
    row: pd.Series | object,
    mean_column: str,
    std_column: str,
    precision: int = 3,
) -> str:
    mean = cell_value(row, mean_column)
    std = cell_value(row, std_column)
    return f"{mean:.{precision}f} ± {std:.{precision}f}"


def format_time(row: pd.Series, prefix: str) -> str:
    return format_mean_std(row, f"{prefix}_time_mean_ms", f"{prefix}_time_std_ms")


def format_metric(row: pd.Series, prefix: str) -> str:
    return format_mean_std(row, f"{prefix}_metric_mean", f"{prefix}_metric_std")


def format_working_set(size_bytes: int) -> str:
    if size_bytes < 1024 * 1024:
        return f"{size_bytes // 1024} KiB"
    return f"{size_bytes // (1024 * 1024)} MiB"


def write_report_tables(outputs: dict[str, pd.DataFrame]) -> None:
    """Create a compact Markdown source for the course experiment report."""
    report_path = ROOT / "docs" / "report_tables.md"
    report_path.parent.mkdir(parents=True, exist_ok=True)

    def basic_rows(frame: pd.DataFrame) -> list[list[str]]:
        return [
            [
                f"{int(row.size):,}",
                format_time(row, "m5"),
                format_time(row, "i5"),
                f"{row.speedup_m5:.3f}×",
            ]
            for row in frame.itertuples(index=False)
        ]

    sieve = outputs["sieve_comparison.csv"]
    quicksort = outputs["quicksort_comparison.csv"]
    matrix_single = outputs["matrix_single_comparison.csv"]
    matrix_multi = outputs["matrix_multi_comparison.csv"]
    memory = outputs["memory_comparison.csv"]

    lines = [
        "# CPU Toy Benchmark 结果表",
        "",
        "正式测试均采用 1 次预热和 10 次计时重复。表内时间为平均值 ± 样本标准差；M5 相对性能 = T(i5-12400F) / T(M5)，大于 1 表示 M5 更快。",
        "",
        "原始数据、平台汇总与下列比较表均可追溯，且不修改 raw CSV。不要将不同 benchmark 或不同单位的数值直接平均。",
        "",
        "## 筛法",
        "",
        markdown_table(
            ["问题规模", "M5 时间（ms）", "i5-12400F 时间（ms）", "M5 相对性能"],
            basic_rows(sieve),
        ),
        "",
        "## 快速排序",
        "",
        markdown_table(
            ["元素数量", "M5 时间（ms）", "i5-12400F 时间（ms）", "M5 相对性能"],
            basic_rows(quicksort),
        ),
        "",
        "## 单线程矩阵乘法",
        "",
        markdown_table(
            ["矩阵维度 N", "M5 时间（ms）", "i5-12400F 时间（ms）", "M5 GFLOPS", "i5-12400F GFLOPS", "M5 相对性能"],
            [
                [
                    str(int(row.size)),
                    format_time(row, "m5"),
                    format_time(row, "i5"),
                    format_metric(row, "m5"),
                    format_metric(row, "i5"),
                    f"{row.speedup_m5:.3f}×",
                ]
                for row in matrix_single.itertuples(index=False)
            ],
        ),
        "",
        "## 多线程矩阵乘法（N = 512）",
        "",
        markdown_table(
            ["线程数", "M5 时间（ms）", "i5-12400F 时间（ms）", "M5 本机加速比", "i5 本机加速比", "M5 相对性能"],
            [
                [
                    str(int(row.threads)),
                    format_time(row, "m5"),
                    format_time(row, "i5"),
                    f"{row.m5_parallel_speedup:.3f}×",
                    f"{row.i5_parallel_speedup:.3f}×",
                    f"{row.speedup_m5:.3f}×",
                ]
                for row in matrix_multi.itertuples(index=False)
            ],
        ),
        "",
        "## 内存访问",
        "",
        markdown_table(
            ["模式", "工作集", "M5 吞吐量（百万次访问/秒）", "i5-12400F 吞吐量（百万次访问/秒）", "M5 相对性能"],
            [
                [
                    "顺序访问" if row.variant == "sequential" else "随机访问",
                    format_working_set(int(row.size)),
                    format_metric(row, "m5"),
                    format_metric(row, "i5"),
                    f"{row.speedup_m5:.3f}×",
                ]
                for row in memory.itertuples(index=False)
            ],
        ),
        "",
        "## 使用说明",
        "",
        "- 可将本文件中的表格直接作为实验报告的结果表。",
        "- 结论仅适用于本项目规定的源代码、输入、workload、编译配置和操作系统，不代表 ARM 与 x86 的普遍结论。",
        "- 精确的编译器配置、min/max、样本数与完整相对性能清单见 `results/*/summary/` 和 `results/comparison/summary/`。",
        "",
    ]
    report_path.write_text("\n".join(lines), encoding="utf-8")
    print(f"Wrote report tables: {report_path}")


def main() -> None:
    summaries: dict[str, pd.DataFrame] = {}
    for platform_key in PLATFORM_KEYS:
        raw = load_raw(platform_key)
        summary = summarize(raw)
        summaries[platform_key] = summary
        output_dir = ROOT / "results" / platform_key / "summary"
        output_dir.mkdir(parents=True, exist_ok=True)
        output_file = output_dir / "summary.csv"
        summary.to_csv(output_file, index=False)
        print(f"{platform_key}: {len(raw)} raw rows -> {len(summary)} summary rows: {output_file}")

    comparison = build_comparison(summaries)
    outputs = write_comparison_files(comparison)
    write_report_tables(outputs)


if __name__ == "__main__":
    main()
