# InFuse output formats

Coordinates are expressed in millimeters. Text files use UTF-8 and whitespace-separated numbers. Counts describe the immediately following records.

`*_paths_step.txt`, `*_paths.txt`, `*_paths_jump.txt`, and `*_paths_interface.txt` use this hierarchy:

```text
patch_count
  polyline_count_in_patch
    point_count_in_polyline
      x y z
      ...
    ...
  ...
```

The indentation above is explanatory; files contain one count or coordinate triple per line. A one-point polyline represents an injection position rather than an extrusion segment. Empty polylines contain a zero point count.

`Injection_Order.txt` begins with the number of layer records. Each record has a layer type: `-1` outer model, `0` normal layer, `1` injection layer, or `2` solid layer. A type-1 record is followed by its center coordinates `x y z`. Other types have no coordinate triple in this file. The record order follows the planned patch sequence.

`slice_layers*.txt` begins with a layer count. Each layer has a contour count. Each contour has a point count followed by `x y z` triples. `Blocks_files/*_blockN.txt` begins with a layer count, then each layer's point count and `x y z layer_height` records.

OBJ and STL files follow their standard text mesh formats. OBJ vertex records may include an additional RGB triple after XYZ. G-code is a machine-specific motion program; replication evaluates exported numerical data and meshes without running a printer.

The reproduction script exports:

| File | Content |
| --- | --- |
| `paper_figure.svg` | Five cumulative XZ printing-stage views for the final manuscript's Figure 14 Bunny target, with injection centers |
| `paper_stages.csv` | P1-P5 mapping to incremental OBJ meshes, cumulative mesh sets, and the next event |
| `injection_events.csv` | I1-I4 in execution order, layer-record index, and XYZ in the same frame as the OBJ meshes |
| `paper_result.json` | Final manuscript title, DOI, figure/page reference, computed printing/event counts, coordinate frame, and count-check status |
| `table.csv`, `metrics.json` | Mode, patch/polyline/point counts, layer classifications, and lengths |
| `toolpaths.svg` | Projection of the generated paths using experiment.json's projection_axes, with patch colors; the bunny uses XZ |
| `run_manifest.json` | Input/configuration/binary hashes, executed commands, environment, timings, and reference comparison |
| `<mode>.log` | Captured output for each selected mode; the bunny experiment produces shell.log |
| `latest_run.txt` | Relative directory of the latest successful run |

`polyline_length_mm` sums consecutive-point distances inside nonempty polylines. `inter_polyline_gap_mm` sums the distance from the last point of each nonempty polyline to the first point of the next one, in exported order. It is a geometric gap measure, not an estimate of actual machine travel or printing time. Runtime is recorded separately and is not part of deterministic reference matching. SVG projects onto the two selected coordinate axes (XZ for the bunny), collapsing the remaining coordinate; it is a diagnostic view, not a rendering of fabricated hardware.

The paper target and field-by-field figure-data specification are also supplied in [PAPER_RESULT.txt](PAPER_RESULT.txt), a plain UTF-8 text file. The five OBJ meshes correspond to incremental printing stages P1-P5, not five independent complete Bunny models. Their cumulative unions reconstruct the successive printed states. The injection CSV uses postprocessed platform coordinates; the raw `Injection_Order.txt` centers have not undergone that platform translation.
