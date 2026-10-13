# InFuse cleanup scope

This is a separate source snapshot. The original working project is retained outside this folder.

Both the default shell workflow and the outer/inner fusion workflow are retained, including injection detection, collision constraints, merging, topological beam search, Clipper offsetting, gap filling, G-code generation, and per-patch OBJ export. The fusion workflow is exposed as a command-line option instead of requiring an edit to a local Boolean in main.cpp.

Removed modules:

- `opp/opp_graph.*`: old OPP implementation, not compiled by the current Visual Studio project and not used by the current entry point.
- `opp/edge.cpp`: empty translation unit; the Edge class remains in edge.h.
- `opp/resampling.*`: resampling API with no active call from either retained pipeline.
- `slice/curve-to-stl.*`, `generateClosedTube.*`, `Point3f.*`: disconnected curve/tube utilities and their point type.
- `slice/sample_on_ball.*`: used only by the removed multidirection slicing workflow.
- `slice/Vector2.*`: no references from the retained application.
- `slice/infill.*`: its hatch calls were already disabled; the active zigzag filler in katana.cpp is retained.
- `slice/ball.txt`: used only by the removed point-ball visualization.

Removed functions include multidirection slicing, support-scoring and flatten-area utilities, old dependency-graph visualization, unused polygon drawing, STL saving, the segment-mapping overload, segment offsetting, obsolete render exporters, unused curve/triangle helpers, and hollow/cylinder mesh generators. The mesh insertion overload used by patch OBJ export is retained. Declarations for abandoned search/clustering experiments and commented-out implementations were also removed.

Changes for reproducible use:

- Chinese comments and messages were translated where useful; damaged or redundant comments were removed. Application text is UTF-8 and English.
- Hardcoded model/output paths were replaced by CLI options and isolated per-model working folders.
- CMake builds Clipper directly from source and links the active OpenCV dependency.
- Configuration and input failures now produce errors instead of continuing with invalid data.
- Both modes and batch isolation are exercised by the included tests.
- Temporary graph matrices use automatic storage management, avoiding leaks between models. Fusion remapping arrays are sized to the actual node count.
- Travel search reports empty/cyclic constraint graphs and avoids division by zero in its normalized score. Fusion injection traversal stops when no predecessor exists.
- Disconnected normal/flatten-area tracking, unused constants, string-configuration storage, and obsolete geometric helper functions were removed.
- Stale model statistics and an embedded preview image were removed from the startup template; its machine commands were retained.

Shared vector operators and utilities were retained where removal could affect overload resolution or core geometry. This cleanup does not claim a complete semantic proof that every remaining utility is required by every possible model. It does not redesign the core research algorithms.

Local cleanup audit files and build helpers live outside this folder under `open_source/`; they are for the owner's review and are not part of the publishable source package.
