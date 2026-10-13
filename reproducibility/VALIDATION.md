# InFuse validation

Local validation environment:

| Component | Measured value |
| --- | --- |
| Date | 2026-10-13 (Asia/Shanghai) |
| OS | Windows 11 Pro x64, build 10.0.26200 |
| CPU | Intel Core i7-13700F |
| Installed memory | Approximately 64 GiB; this is the test host, not a minimum requirement |
| Compiler | MSVC 19.44.35223, Visual Studio 2022 17.14.37012.4 |
| CMake | 3.30.1 |
| Python | 3.12.4 |
| OpenCV | 4.4.0, x64 vc15 distribution |
| Clipper | Included 6.4.2 source |

The InFuse Release build passed. The earlier `pipeline_smoke` test passed both shell/fusion pipelines, batch equivalence, and error handling. The source-distribution audit now additionally checks that the default experiment uses the included bunny unchanged in single-cavity shell mode.

The default experiment was changed to the author-selected bunny outer model. A complete invocation of reproduce.ps1 succeeded in 336.689 seconds of application runtime and exported 5 patch OBJ meshes, G-code, numerical paths, a CSV table, JSON metrics, and an XZ path projection. The model is byte-identical to the supplied ASCII STL and has 44,644 triangles. Geometry, scale, and orientation were not changed.

| Bunny measurement | Value |
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

expected_metrics.json records the baseline from this successful bunny run. The saved outputs were checked against that baseline; this is not a claim of an independent second full bunny run. Subsequent parameter-free runs compare their newly generated data with the baseline. Numerical lengths are defined in OUTPUT_FORMATS.md and are not physical printer runtime/travel estimates. The exported OBJ meshes total approximately 360 MB; they are generated under results and are not included in the source archive. The supplied STL and compact reference table/projection are included.

The previous cylinder demonstration and its measurements are retained in reproducibility/synthetic_demo; it is now used only as a small regression example. The original full workspace sources and the supplied bunny file were not modified.

INSTALL_WINDOWS.txt specifies a separate, explicit fresh-system setup with CMake 3.31.6 and Python 3.12.10. Those installer steps have not been run on a clean virtual machine. GitHub Actions has not run remotely. No full paper benchmark, fabrication experiment, or complete GRSI compliance has been verified.

Reference measurements in expected_metrics.json describe the selected bunny experiment. Length comparisons use relative tolerance 1e-6 or absolute tolerance 1e-5 mm; counts must match exactly. Runtime and random OBJ colors are excluded from matching. The corresponding paper figure/table identifier has not yet been supplied.
