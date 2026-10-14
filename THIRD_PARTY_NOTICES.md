# Third-party acknowledgments and notices

InFuse builds on the [ACAP framework](https://github.com/fanchao98/acap) by Fanchao Zhong, Yonglai Xu, Haisen Zhao, and Lin Lu, extending its fabrication-planning implementation for in-situ printing and material injection. The upstream paper is cited in [REFERENCES.bib](REFERENCES.bib).

| Component | Role | Applicable notice |
| --- | --- | --- |
| [ACAP](https://github.com/fanchao98/acap) | Upstream fabrication-planning framework | The upstream README reserves program rights to its authors and restricts use to research purposes; see the [upstream terms](https://github.com/fanchao98/acap#license). |
| [Katana](https://github.com/Kandepet/katana) / [Schlizzer](https://github.com/dronus/Schlizzer) | Slicer ancestry | Original attribution is retained in `slice/katana.cpp`. No additional license for these inherited files is granted here. |
| Clipper 6.4.2, Angus Johnson | Polygon clipping and offsetting | Boost Software License 1.0; original notices remain in `slice/clipper.cpp` and `slice/clipper.hpp`. The license text is included in [licenses/BSL-1.0.txt](licenses/BSL-1.0.txt). |
| [OpenCV](https://opencv.org/) 4.4.0 | External geometry-processing dependency | BSD 3-Clause and the component notices accompanying the distribution. The OpenCV SDK is downloaded separately and is not bundled in this repository. |

Third-party notices apply to their respective components. Attribution and paper citations do not grant additional rights to inherited code. No project-wide license is supplied for the combined implementation. Example-data provenance is documented separately in [examples/models/README.md](examples/models/README.md).
