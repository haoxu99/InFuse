# InFuse

InFuse is a computational fabrication framework for in-situ 3D printing and material injection. It plans interleaved printing and injection operations under printing, injection, air-evacuation, and nozzle-accessibility constraints.

This repository contains the C++ command-line implementation, Windows build instructions, and a reproducible single-cavity Bunny example. The implementation builds on and extends the [ACAP framework](https://github.com/fanchao98/acap) with injection-aware sequence planning and fabrication processing. Upstream attribution is listed in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

**Paper:** [InFuse: In-Situ Material Injection for 3D Printing](https://doi.org/10.36227/techrxiv.175329147.74597084/v1), TechRxiv preprint, 2025. [PDF](https://www.techrxiv.org/doi/pdf/10.36227/techrxiv.175329147.74597084/v1).

## Installation

The supported platform is **Windows 11 x64**. [INSTALL_WINDOWS.txt](INSTALL_WINDOWS.txt) documents Visual Studio 2022, Python, CMake, and OpenCV installation, including a complete setup using fixed absolute paths.

With the source at `C:\InFuse` and the build tools installed as documented:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File C:\InFuse\scripts\build_windows.ps1
```

The script downloads and verifies OpenCV 4.4.0, builds InFuse, and runs the tests. Clipper 6.4.2 is included as source. No third-party Python packages are required. CGAL, Boost, Eigen, libigl, GMP, and MPFR are not build dependencies of this command-line implementation.

The executable is `C:\InFuse\build\Release\infuse.exe`; the generated Visual Studio solution is `C:\InFuse\build\InFuse.sln`. OpenCV runtime files, configuration, and G-code templates are copied beside the executable. Existing tool installations can be selected using the build script options documented in [INSTALL_WINDOWS.txt](INSTALL_WINDOWS.txt).

## Single-cavity Bunny example

The parameter-free workflow runs the bundled Bunny model using the single-cavity shell pipeline:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File C:\InFuse\reproduce.ps1
```

The experiment is defined in [reproducibility/experiment.json](reproducibility/experiment.json), with input `examples/models/bunny_outer.stl`. Each invocation creates an isolated output directory; `C:\InFuse\results\reproduction\latest_run.txt` identifies the latest successful run.

| Output | Description |
| --- | --- |
| `table.csv`, `metrics.json` | Path counts, layer classifications, lengths, and geometric gaps |
| `toolpaths.svg` | XZ projection of the generated paths, colored by patch |
| `shell/bunny_outer/patches` | Patch OBJ meshes |
| `shell/bunny_outer/Blocks_files` | Contour blocks and G-code |
| `run_manifest.json` | Input/configuration hashes, commands, environment, timings, and reference comparison |

Numerical outputs are compared with the recorded Bunny regression baseline. [OUTPUT_FORMATS.md](reproducibility/OUTPUT_FORMATS.md) defines the formats and metrics; [VALIDATION.md](reproducibility/VALIDATION.md) documents the measured environment, baseline, and validation scope. These metrics describe the exported paths and are not physical fabrication-time estimates or a reproduction of all paper benchmark tables.

The [reference table](reproducibility/reference_bunny/table.csv) and [path projection](reproducibility/reference_bunny/toolpaths.svg) are included:

![InFuse Bunny single-cavity toolpaths](reproducibility/reference_bunny/toolpaths.svg)

The recorded run took approximately 5 minutes 37 seconds and produced five patch meshes. Runtime depends on the machine. Detailed OBJ outputs total approximately 360 MB and are generated locally rather than bundled. Model properties and provenance are documented in [examples/models/README.md](examples/models/README.md).

The small cylinder in `examples/demo.stl` is a quick regression input; its separate baseline is stored in `reproducibility/synthetic_demo`.

## Command-line usage

```powershell
& C:\InFuse\build\Release\infuse.exe --input C:\InFuse\examples\models\bunny_outer.stl --output C:\InFuse\results\bunny --mode shell
& C:\InFuse\build\Release\infuse.exe --input C:\InFuse\examples\demo.stl --output C:\InFuse\results\fusion --mode fusion
& C:\InFuse\build\Release\infuse.exe --input C:\InFuse\input_models --output C:\InFuse\results\batch --mode shell
```

Input must be **ASCII STL**, with coordinates in millimeters. Folder input processes direct children in sorted order and accepts `.stl` and `.STL`. Existing per-model output directories are rejected to prevent stale data from contaminating results; repeated runs require a new output location.

| Option | Description |
| --- | --- |
| `--input` | Required ASCII STL file or directory |
| `--output` | Output root; defaults to `results` |
| `--mode shell` | Single-cavity shell pipeline; default mode |
| `--mode fusion` | Outer/inner slicing and fusion pipeline |
| `--config` | Configuration file; defaults to `config.ini` in the current or executable directory |
| `--templates` | G-code template directory; defaults to `slice` in the current or executable directory |
| `--help` | Usage information |

Each model produces `Blocks_files`, `patches`, and `work` directories. The process returns zero only when all requested models succeed.

## Testing and limitations

The included tests cover both pipelines, single/batch numerical equivalence, output isolation, invalid inputs, source consistency, and example-model integrity. A GitHub Actions workflow automates Windows builds, tests, and the Bunny example.

The dependency graph has a 10,000-node capacity. Large or non-manifold models may violate algorithmic assumptions. Layer spacing is 0.2 mm and extrusion width is 0.4 mm in `opp/helpers.h`; `config.ini` must remain consistent. Nozzle geometry is defined in `opp/infuse_main.cpp`; mixing ratio, gap optimization, merging strength, and beam width are defined in `opp/helpers.h`.

Software execution requires no printer or GPU and does not perform physical motion. G-code startup/shutdown templates and injection calibration are machine-specific and must be adapted before physical fabrication. Linux and macOS support has not been validated. The command-line package does not include the interactive interface or hardware-control software.

## Citation

If this code is used for research, please cite the InFuse preprint:

```bibtex
@article{xu2025infuse,
  title   = {{InFuse}: In-Situ Material Injection for {3D} Printing},
  author  = {Xu, Hao and Feng, Shuai and Xiong, Yi and He, Liang and Xu, Wenpeng and Zhao, Haisen},
  journal = {TechRxiv},
  year    = {2025},
  doi     = {10.36227/techrxiv.175329147.74597084/v1},
  url     = {https://www.techrxiv.org/doi/pdf/10.36227/techrxiv.175329147.74597084/v1},
  note    = {Preprint, version 1}
}
```

The implementation also builds on ACAP. Please acknowledge the upstream framework and cite its paper:

```bibtex
@article{zhong2023continuous,
  title={As-continuous-as-possible Extrusion-based Fabrication of Surface Models},
  author={Zhong, Fanchao and Xu, Yonglai and Zhao, Haisen and Lu, Lin},
  journal={ACM Transactions on Graphics},
  volume={42},
  number={3},
  pages={1--16},
  year={2023},
  publisher={ACM New York, NY}
}
```

Both entries are available in [REFERENCES.bib](REFERENCES.bib). The InFuse entry refers specifically to the linked preprint version.

## Third-party terms

Third-party components retain their respective notices and terms; see [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md). This repository does not grant an additional project-wide license.
