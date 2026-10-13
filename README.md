# InFuse

InFuse is a research implementation of extrusion and injection toolpath planning. It processes ASCII STL models through slicing, injection-layer detection, graph fusion and patch merging, travel-order optimization, interlayer gap filling, and G-code/OBJ export.

The project builds on the original [ACAP implementation](https://github.com/fanchao98/acap). InFuse is the name of this project; upstream attribution is preserved in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) and [REFERENCES.bib](REFERENCES.bib).

## Install and build

The supported review platform is **Windows 11 x64**. Follow [INSTALL_WINDOWS.txt](INSTALL_WINDOWS.txt) for complete installation steps on a fresh system, including fixed absolute paths, Visual Studio 2022 Community, Python, CMake, and OpenCV.

After placing the repository at `C:\InFuse` and installing the compiler, Python, and CMake as documented:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File C:\InFuse\scripts\build_windows.ps1
```

The build script fetches and checksums OpenCV 4.4.0, compiles InFuse, and runs tests. Clipper 6.4.2 is included as source. There are no additional Python packages. CGAL, Boost, Eigen, libigl, GMP, and MPFR are not required.

The executable is `C:\InFuse\build\Release\infuse.exe`. The generated Visual Studio solution is `C:\InFuse\build\InFuse.sln`. OpenCV and runtime resources are copied beside the executable. Existing tool installations can be selected with the build script's absolute-path options described in INSTALL_WINDOWS.txt.

## Generate reproducible data

Run the parameter-free workflow:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File C:\InFuse\reproduce.ps1
```

It runs the selected bunny outer model through the **single-cavity workflow (`--mode shell`)**, exports a table and XZ path projection, and compares numerical metrics with the recorded bunny reference. The STL is included at `C:\InFuse\examples\models\bunny_outer.stl`. Its geometry and scale are unchanged from the model supplied by the author. Every run uses a new output directory. `C:\InFuse\results\reproduction\latest_run.txt` identifies it.

- `table.csv` and `metrics.json`: path counts, layer types, lengths, and geometric gaps.
- `toolpaths.svg`: XZ visualization derived from the generated bunny paths.
- `shell/bunny_outer/patches`: patch OBJ meshes.
- `run_manifest.json`: data/configuration hashes, commands, environment, timings, and reference-check status.

The workflow uses [reproducibility/experiment.json](reproducibility/experiment.json). The bunny and single-cavity workflow have been selected by the author. The paper's figure/table number and publication metadata still need to be filled in; this package does not assume which paper figure the exported result corresponds to. Model format, checksum, and provenance are documented in [examples/models/README.md](examples/models/README.md). Custom output formats and metric definitions are documented in [OUTPUT_FORMATS.md](reproducibility/OUTPUT_FORMATS.md).

The measured bunny [table](reproducibility/reference_bunny/table.csv) and [path projection](reproducibility/reference_bunny/toolpaths.svg) are included for inspection:

![InFuse bunny single-cavity toolpaths](reproducibility/reference_bunny/toolpaths.svg)

The small cylinder at `examples/demo.stl` remains a quick regression-test input; it is no longer the default reproduction model. Its earlier measurements are retained under `reproducibility/synthetic_demo` and are separate from the bunny reference.

The measured bunny run took approximately 5 minutes 37 seconds on the documented test computer and produced 5 patch meshes. Runtime depends on the machine. The detailed OBJ outputs total approximately 360 MB and are regenerated under `results`; they are not bundled in the GitHub source package. The source archive includes the unchanged STL and reference table/projection.

## Run your own models

```powershell
& C:\InFuse\build\Release\infuse.exe --input C:\InFuse\examples\models\bunny_outer.stl --output C:\InFuse\results\bunny --mode shell
& C:\InFuse\build\Release\infuse.exe --input C:\InFuse\examples\demo.stl --output C:\InFuse\results\fusion --mode fusion
& C:\InFuse\build\Release\infuse.exe --input C:\InFuse\input_models --output C:\InFuse\results\batch --mode shell
```

Folder input scans direct children in sorted order; `.stl` and `.STL` are accepted. Input must be **ASCII STL**, in millimeters. Existing per-model output folders are rejected to prevent stale block data from contaminating a run. Choose a new output root to repeat it.

| Option | Meaning |
| --- | --- |
| `--input` | Required ASCII STL file or folder |
| `--output` | Output root; defaults to `results` |
| `--mode shell` | Single-cavity shell pipeline; default and bunny experiment mode |
| `--mode fusion` | Outer/inner slicing and fusion pipeline |
| `--config` | Slicing configuration; default current/executable folder `config.ini` |
| `--templates` | G-code template folder; default current/executable folder `slice` |
| `--help` | Usage information |

Each model produces `Blocks_files` (contours and G-code), `patches` (OBJ meshes), and `work` (slice layers, injection order, paths, and diagnostics). The return code is zero only when every model succeeds.

## Validation and limitations

The tests exercise both pipelines, single/batch numerical equivalence, output isolation, and invalid inputs. A second audit checks English-only filenames/text, InFuse naming, and local includes. [VALIDATION.md](reproducibility/VALIDATION.md) records the tested environment and results. A GitHub Actions Windows workflow is included; its remote run is pending publication.

The retained graph has a 10,000-node capacity. Large or non-manifold models may violate assumptions of the research algorithms. Layer spacing is 0.2 mm and extrusion width is 0.4 mm in `opp/helpers.h`; keep `config.ini` consistent. Nozzle geometry is in `opp/infuse_main.cpp`. Mixing ratio, gap optimization, merging strength, and beam width are in `opp/helpers.h`.

The startup/shutdown templates and injection calibration are machine-specific. Software replication requires no printer or GPU and does not execute physical motion. Inspect and adapt G-code before using a printer. Linux and macOS have not been validated. Full paper-dataset regression and clean-VM installation remain pending.

## GitHub and GRSI preparation

Upload the contents of this InFuse folder as the repository root, including dotfiles and `.github`. Generated builds, SDKs, and result folders are ignored. The source archive includes no compiled binaries or private datasets. [GITHUB_RELEASE.md](reproducibility/GITHUB_RELEASE.md) describes publication and archival steps.

Preparation is tracked against the [GRSI requirements](https://www.replicabilitystamp.org/requirements.html) in [GRSI_CHECKLIST.md](reproducibility/GRSI_CHECKLIST.md). The repository includes [submission.txt](submission.txt), precise Windows installation instructions, the parameter-free experiment, documented outputs, and an unsigned review-form preparation record.

The paper title/authors and figure/table mapping, signed review permission, and code/model release permissions are still author actions. No stamp approval or complete submission compliance is claimed.

## License and provenance

No project-wide release license has been assigned. The upstream ACAP README restricts use to research and does not explicitly grant derivative redistribution/relicensing. Confirm the permissions applicable to your copy before publishing; see [LICENSE_STATUS.md](LICENSE_STATUS.md). A citation does not replace permission. Original Clipper and slicer notices are preserved.

Cite the original ACAP method when using it. Add the InFuse paper's title, authors, DOI, and citation once supplied. [CLEANUP.md](CLEANUP.md) describes the retained algorithms and removed legacy modules.
