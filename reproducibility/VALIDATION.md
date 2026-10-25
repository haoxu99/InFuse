# Validation and regression baseline

## Recorded environment

| Component | Value |
| --- | --- |
| Date | 2026-10-13 (Asia/Shanghai) |
| OS | Windows 11 Pro x64, build 10.0.26200 |
| CPU | Intel Core i7-13700F |
| Installed memory | Approximately 64 GiB; not a minimum requirement |
| Compiler | MSVC 19.44.35223, Visual Studio 2022 17.14.37012.4 |
| CMake | 3.30.1 |
| Python | 3.12.4 |
| OpenCV | 4.4.0, x64 vc15 distribution |
| Clipper | Included 6.4.2 source |

The Release build and `pipeline_smoke` test passed in this environment. Tests covered both shell/fusion pipelines, single/batch numerical equivalence, output isolation, and error handling. The source-distribution audit checks English filenames/text, local includes, and the bundled example's integrity.

## Bunny baseline

A complete parameter-free invocation of `reproduce.ps1` processed the bundled Bunny model in shell mode in 336.689 seconds of application runtime. It exported five patch OBJ meshes, G-code, numerical paths, a CSV table, JSON metrics, and an XZ projection. The input contains 44,644 triangles; geometry, scale, and orientation are unchanged.

| Measurement | Value |
| --- | --- |
| Mode | shell (single cavity) |
| Patch count | 5 |
| Nonempty polyline count | 10,762 |
| Point count | 588,934 |
| Layer records | 468 |
| Injection layer records | 4 |
| Solid layer records | 2 |
| Polyline length | 183,815.592697 mm |
| Inter-polyline geometric gap | 62,286.808384 mm |

[expected_metrics.json](expected_metrics.json) stores the baseline from that run. The 2026-10-13 check used saved outputs; the independent full rerun on 2026-10-25 is documented below. Subsequent runs compare newly generated outputs with the baseline.

Length comparisons use relative tolerance `1e-6` or absolute tolerance `1e-5` mm; counts must match exactly. Runtime and random OBJ colors are excluded. [OUTPUT_FORMATS.md](OUTPUT_FORMATS.md) defines the lengths as geometric path measurements rather than estimates of physical machine motion or fabrication time.

The generated OBJ meshes total approximately 360 MB and are not bundled. Compact reference outputs are stored in `reference_bunny`. The small cylinder has a separate regression baseline in `synthetic_demo`.

## Validation scope

This record describes the command-line implementation and its regression example. It does not establish numerical equivalence to every paper figure or benchmark, or validate physical fabrication. Linux and macOS have not been tested. The fresh-system installation sequence in [INSTALL_WINDOWS.txt](../INSTALL_WINDOWS.txt), which specifies CMake 3.31.6 and Python 3.12.10, has not been tested in a clean virtual machine.

The GitHub Actions workflow provides additional automated build/test/example execution; its live status is reported by GitHub rather than this dated local record.

## Documentation consistency checks

On 2026-10-14, both CTest tests passed after the public documentation and metadata revision (57.15 seconds total). The source audit additionally verifies local documentation links, JSON syntax, and agreement between the README BibTeX entries and REFERENCES.bib. The C++ implementation and bundled STL were unchanged by this revision.

## Figure 16 Bunny verification, 2026-10-25

The repository's submitted TechRxiv v1 PDF was inspected to identify the Bunny row of Figure 16 (PDF page 12, including its cover). This is the version-specific target recorded in [experiment.json](experiment.json). The illustrated sequence has five printing stages and four injection events.

A fresh invocation of the root `reproduce.ps1`, without parameters, completed in **336.574 seconds** of application runtime. The output directory was `results/reproduction/20261025T014831736175Z`. It computed new geometry and exported the five OBJ meshes, stage/event CSV files, `paper_result.json`, and `paper_figure.svg`. Both `reference_check` and `paper_result_check` in the new run manifest were `passed`. All numerical metrics matched the earlier Bunny baseline without changing that baseline, the input STL, or the C++ implementation.

The recorded [stage mapping](reference_bunny/paper_stages.csv), [injection coordinates](reference_bunny/injection_events.csv), and [reference preview](reference_bunny/figure16_bunny.png) come from that new computation. The SVG preview was rasterized locally for visual inspection; rendering this reference PNG is not an additional runtime dependency of the reproduction script.

All three CTest tests passed in **56.53 seconds**: `pipeline_smoke`, `source_distribution`, and `paper_result_export` (four exporter unit cases). The source audit was adjusted to respect the repository's removal of REFERENCES.bib, rather than requiring or restoring that file. A stale third-party documentation link was corrected.

The stage/event count check validates five printing stages and four injections, not pixel identity or every paper-image boundary. The distributed beam width is 500, while the preprint's benchmark setting is 7500; this run does not reproduce Tables 1 or 2. The target is the Bunny geometric/stage data, with standard OBJ/CSV outputs and an explanatory cumulative XZ preview. Other gallery models, physical fabrication, and injected-liquid volume rendering remain outside the validated scope.
