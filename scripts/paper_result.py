"""Export paper-figure data from freshly computed InFuse paths, not screenshots."""
import csv
import html
import json


COLORS = ('#63B7A4', '#B8C99E', '#DDC66E', '#CD7779', '#A88F79')


def injection_events(path):
    """Read event coordinates from postprocessed paths in the OBJ coordinate frame."""
    from reproduce import read_paths
    return [(layer, points[0])
            for layer, paths in enumerate(read_paths(path))
            for points in paths if len(points) == 1]


def write_csv(path, rows):
    with path.open('w', newline='', encoding='utf-8') as stream:
        writer = csv.DictWriter(stream, fieldnames=list(rows[0]))
        writer.writeheader()
        writer.writerows(rows)


def stage_svg(patches, events, target):
    """Use shared vector groups to show cumulative printing, without duplicating data."""
    all_points = [point for patch in patches for path in patch for point in path]
    if not all_points:
        raise ValueError('The paper result contains no printing paths.')
    minx, maxx = min(p[0] for p in all_points), max(p[0] for p in all_points)
    minz, maxz = min(p[2] for p in all_points), max(p[2] for p in all_points)
    panel_width, height = 300, 380
    scale = min(240 / max(maxx-minx, 1e-9), 240 / max(maxz-minz, 1e-9))

    def project(point):
        return (150+(point[0]-(minx+maxx)/2)*scale,
                190-(point[2]-(minz+maxz)/2)*scale)

    width = panel_width * len(patches)
    out = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" viewBox="0 0 {width} {height}">',
           f'<rect width="{width}" height="{height}" fill="white"/>',
           f'<text x="{width/2}" y="28" text-anchor="middle" font-family="sans-serif" font-size="20">{html.escape(target["figure"])}: Bunny printing stages and injection events</text>',
           '<defs>']
    for index, patch in enumerate(patches):
        out.append(f'<g id="P{index+1}" stroke="{COLORS[index % len(COLORS)]}" fill="none" stroke-width="0.65">')
        for path in patch:
            if len(path) > 1:
                xy = ' '.join(f'{x:.3f},{z:.3f}' for x, z in map(project, path))
                out.append(f'<polyline points="{xy}"/>')
        out.append('</g>')
    out.append('</defs>')
    for index in range(len(patches)):
        out.append(f'<g transform="translate({index*panel_width} 0)">')
        for previous in range(index+1):
            out.append(f'<use href="#P{previous+1}"/>')
        if index < len(events):
            x, z = project(events[index][1])
            out.extend((f'<circle cx="{x:.3f}" cy="{z:.3f}" r="4" fill="#7F3C8D" stroke="white"/>',
                        f'<text x="{x+7:.3f}" y="{z-7:.3f}" font-family="sans-serif" font-size="13" fill="#7F3C8D">I{index+1}</text>'))
        prefix = 'P1' if index == 0 else f'P1-P{index+1}'
        label = f'{prefix}; then I{index+1}' if index < len(events) else f'{prefix}; final sealing'
        out.append(f'<text x="150" y="330" text-anchor="middle" font-family="sans-serif" font-size="16">{label}</text></g>')
    out.append(f'<text x="{width/2}" y="367" text-anchor="middle" font-family="sans-serif" font-size="13">Cumulative XZ projections of newly computed paths; dots mark injection centers. Not a photorealistic rendering.</text></svg>')
    return '\n'.join(out)+'\n'


def export_paper_result(output, model_folder, stem, patches, manifest):
    target = manifest.get('paper_result')
    if target is None:
        return None
    events = injection_events(model_folder/'work'/(stem+'_paths.txt'))
    if len(patches) != target['printing_stage_count'] or len(events) != target['injection_event_count']:
        raise ValueError('Computed printing/injection counts do not match the selected paper panel.')
    if len(patches) != len(events)+1:
        raise ValueError('Expected one final printing stage after the injection events.')
    rows, mesh_paths = [], []
    for index, patch in enumerate(patches):
        mesh = model_folder/'patches'/f'patch_{index}.obj'
        if not mesh.is_file() or not mesh.stat().st_size:
            raise ValueError('Missing paper-stage mesh: '+str(mesh))
        mesh_paths.append(mesh.relative_to(output).as_posix())
        rows.append({'printing_stage': f'P{index+1}',
                     'incremental_obj': mesh_paths[-1],
                     'cumulative_objs': ';'.join(mesh_paths),
                     'next_event': f'I{index+1}' if index < len(events) else 'complete',
                     'printing_polylines': sum(len(path)>1 for path in patch)})
    write_csv(output/'paper_stages.csv', rows)
    write_csv(output/'injection_events.csv', [
        {'event': f'I{index+1}', 'after_printing_stage': f'P{index+1}',
         'layer_record_index': layer, 'x_mm': point[0], 'y_mm': point[1], 'z_mm': point[2]}
        for index, (layer, point) in enumerate(events)])
    report = {'paper_result': target,
              'generated_printing_stages': len(patches),
              'generated_injection_events': len(events),
              'stage_event_count_check': 'passed',
              'coordinate_frame': 'Printing platform; same as patch OBJ and G-code; millimeters',
              'stages': rows,
              'scope': 'Bunny geometry and printing/injection stage data; no other gallery models, benchmark tables, hardware execution, or pixel-identical rendering'}
    (output/'paper_result.json').write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')
    (output/'paper_figure.svg').write_text(stage_svg(patches, events, target), encoding='utf-8')
    print(f'Paper target: {target["figure"]}, Bunny; {len(patches)} printing stages, {len(events)} injection events.', flush=True)
    return report
