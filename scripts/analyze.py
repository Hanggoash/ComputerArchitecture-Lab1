#!/usr/bin/env python3
"""Summarize immutable raw benchmark CSV files and compute comparison metrics."""

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
        "metric_mean": "m5_metric_mean",
        "compiler_config": "m5_compiler_config",
    })
    windows = windows.rename(columns={
        "time_mean_ms": "i5_time_mean_ms",
        "time_std_ms": "i5_time_std_ms",
        "metric_mean": "i5_metric_mean",
        "compiler_config": "i5_compiler_config",
    })
    mac = mac[key_columns + ["m5_time_mean_ms", "m5_time_std_ms", "m5_metric_mean", "m5_compiler_config"]]
    windows = windows[key_columns + ["i5_time_mean_ms", "i5_time_std_ms", "i5_metric_mean", "i5_compiler_config"]]
    comparison = mac.merge(windows, on=key_columns, how="inner")
    comparison["speedup_m5"] = comparison["i5_time_mean_ms"] / comparison["m5_time_mean_ms"]
    return comparison.sort_values(key_columns).reset_index(drop=True)


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
    comparison_dir = ROOT / "results" / "comparison" / "summary"
    comparison_dir.mkdir(parents=True, exist_ok=True)
    comparison_file = comparison_dir / "comparison.csv"
    comparison.to_csv(comparison_file, index=False)
    print(f"cross-platform comparison: {len(comparison)} matched rows: {comparison_file}")


if __name__ == "__main__":
    main()
