"""Run the documented InFuse experiment with no command-line arguments."""
from pathlib import Path
from datetime import datetime, timezone
import csv
import hashlib
import html
import json
import math
import os
import platform
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[1]

def local_path(value):
    path = (ROOT/value).resolve()
    if not path.is_relative_to(ROOT):
        raise ValueError('Experiment paths must stay inside the repository.')
    return path

def read_paths(path):
    tokens = iter(path.read_text(encoding='utf-8').split())
    patches = []
    try:
        for _ in range(int(next(tokens))):
            paths = []
            for _ in range(int(next(tokens))):
                points = [tuple(float(next(tokens)) for _ in range(3)) for _ in range(int(next(tokens)))]
                if any(not math.isfinite(v) for point in points for v in point):
                    raise ValueError('Nonfinite path coordinate.')
                paths.append(points)
            patches.append(paths)
        if next(tokens, None) is not None:
            raise ValueError('Unexpected trailing path data.')
    except StopIteration as error:
        raise ValueError('Truncated path data: '+str(path)) from error
    return patches

def injection_counts(path):
    tokens = iter(path.read_text(encoding='utf-8').split())
    count = int(next(tokens))
    types = []
    for _ in range(count):
        layer_type = int(next(tokens))
        types.append(layer_type)
        if layer_type == 1:
            for _ in range(3):
                float(next(tokens))
    if next(tokens, None) is not None:
        raise ValueError('Unexpected trailing injection-order data.')
    return count, types.count(1), types.count(2)

def summarize(folder, stem, mode):
    patches = read_paths(folder/'work'/(stem+'_paths_step.txt'))
    paths = [points for patch in patches for points in patch if points]
    length = sum(math.dist(a,b) for points in paths for a,b in zip(points,points[1:]))
    gaps = sum(math.dist(a[-1],b[0]) for a,b in zip(paths,paths[1:]))
    layers,injection,solid = injection_counts(folder/'work'/'Injection_Order.txt')
    gcode = folder/'Blocks_files'/(stem+'_path.gcode')
    meshes = list((folder/'patches').glob('patch_*.obj'))
    if not meshes or not gcode.is_file() or not gcode.stat().st_size:
        raise ValueError('Required mesh or G-code outputs are missing.')
    record = {'mode':mode,'patch_count':len(patches),'polyline_count':len(paths),
              'point_count':sum(map(len,paths)),'layer_record_count':layers,
              'injection_layer_count':injection,'solid_layer_count':solid,
              'polyline_length_mm':round(length,6),'inter_polyline_gap_mm':round(gaps,6)}
    return record,patches

def projection_svg(records, patch_sets, manifest):
    colors = ('#2474B5','#E27633','#49A078','#A063BC')
    width = max(1,len(records))*500
    axes = manifest.get('projection_axes',[0,1])
    if len(axes)!=2 or len(set(axes))!=2 or any(axis not in (0,1,2) for axis in axes):
        raise ValueError('projection_axes must contain two different coordinate indices in [0, 1, 2].')
    horizontal,vertical = axes
    title = html.escape(manifest.get('display_title','InFuse toolpath projection'))
    out = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="560" viewBox="0 0 {width} 560">',
           f'<rect width="{width}" height="560" fill="white"/>',
           f'<text x="{width/2}" y="32" text-anchor="middle" font-family="sans-serif" font-size="20">{title}</text>']
    for column,(record,patches) in enumerate(zip(records,patch_sets)):
        paths = [p for patch in patches for p in patch if p]
        points = [p for path in paths for p in path]
        minx,maxx = min(p[horizontal] for p in points),max(p[horizontal] for p in points)
        miny,maxy = min(p[vertical] for p in points),max(p[vertical] for p in points)
        scale = min(390/max(maxx-minx,1e-9),350/max(maxy-miny,1e-9))
        centerx,centery = column*500+250,260
        project = lambda p:(centerx+(p[horizontal]-(minx+maxx)/2)*scale,centery-(p[vertical]-(miny+maxy)/2)*scale)
        out.append(f'<text x="{centerx}" y="68" text-anchor="middle" font-family="sans-serif" font-size="18">{html.escape(record["mode"])}</text>')
        for patch_index,patch in enumerate(patches):
            for path in patch:
                if len(path)>1:
                    xy = ' '.join(f'{x:.3f},{y:.3f}' for x,y in map(project,path))
                    out.append(f'<polyline points="{xy}" fill="none" stroke="{colors[patch_index%len(colors)]}" stroke-width="1.5" opacity="0.65"/>')
                elif path:
                    x,y = project(path[0])
                    out.append(f'<circle cx="{x:.3f}" cy="{y:.3f}" r="4" fill="#D02B31"/>')
        caption = f'{record["patch_count"]} patches; {record["polyline_count"]} polylines; length {record["polyline_length_mm"]:.3f} mm'
        out.append(f'<text x="{centerx}" y="480" text-anchor="middle" font-family="sans-serif" font-size="13">{html.escape(caption)}</text>')
    axis_names = ''.join('XYZ'[axis] for axis in axes)
    out.append(f'<text x="{width/2}" y="530" text-anchor="middle" font-family="sans-serif" font-size="12">{axis_names} projection; colors distinguish patches.</text></svg>')
    return '\n'.join(out)+'\n'

def main():
    if len(sys.argv)!=1:
        raise ValueError('Run this script without parameters; edit reproducibility/experiment.json to configure the experiment.')
    manifest = json.loads((ROOT/'reproducibility'/'experiment.json').read_text(encoding='utf-8'))
    binary = next((p for p in (ROOT/'build'/'Release'/'infuse.exe',ROOT/'build'/'infuse',ROOT/'build'/'infuse.exe') if p.is_file()),None)
    if binary is None:
        raise FileNotFoundError('Build InFuse first; see INSTALL_WINDOWS.txt.')
    model,config,templates = [local_path(manifest[key]) for key in ('input','configuration','templates')]
    if not model.is_file():
        raise FileNotFoundError('Missing experiment input: '+str(model))
    stamp = datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%S%fZ')
    output = ROOT/'results'/'reproduction'/stamp
    output.mkdir(parents=True,exist_ok=False)
    records,patch_sets,commands,times = [],[],[],{}
    for mode in manifest['modes']:
        if mode not in ('shell','fusion'):
            raise ValueError('Unsupported mode: '+mode)
        command = [str(binary),'--input',str(model),'--output',str(output/mode),'--mode',mode,'--config',str(config),'--templates',str(templates)]
        commands.append(command)
        start = time.perf_counter()
        with (output/(mode+'.log')).open('w',encoding='utf-8') as log:
            completed = subprocess.run(command,cwd=ROOT,stdout=log,stderr=subprocess.STDOUT,timeout=manifest['timeout_seconds_per_mode'])
        times[mode] = round(time.perf_counter()-start,3)
        if completed.returncode:
            raise RuntimeError(f'{mode} failed with exit {completed.returncode}; see {output/(mode+".log")}')
        record,patches = summarize(output/mode/model.stem,model.stem,mode)
        records.append(record)
        patch_sets.append(patches)
        print(f'Completed {mode} in {times[mode]} s.',flush=True)
    with (output/'table.csv').open('w',newline='',encoding='utf-8') as table:
        writer = csv.DictWriter(table,fieldnames=list(records[0]))
        writer.writeheader()
        writer.writerows(records)
    (output/'metrics.json').write_text(json.dumps(records,indent=2)+'\n',encoding='utf-8')
    (output/'toolpaths.svg').write_text(projection_svg(records,patch_sets,manifest),encoding='utf-8')
    run_record = {'experiment':manifest,'input_sha256':hashlib.sha256(model.read_bytes()).hexdigest(),
                  'configuration_sha256':hashlib.sha256(config.read_bytes()).hexdigest(),
                  'binary_sha256':hashlib.sha256(binary.read_bytes()).hexdigest(),'commands':commands,
                  'platform':platform.platform(),'python':sys.version.split()[0],'elapsed_seconds':times,
                  'git_commit':None,'reference_check':'not_available'}
    try:
        repository_root = subprocess.check_output(['git','-C',str(ROOT),'rev-parse','--show-toplevel'],stderr=subprocess.DEVNULL,text=True).strip()
        if Path(repository_root).resolve() == ROOT:
            run_record['git_commit'] = subprocess.check_output(['git','-C',str(ROOT),'rev-parse','HEAD'],stderr=subprocess.DEVNULL,text=True).strip()
    except (FileNotFoundError,subprocess.CalledProcessError):
        pass
    expected_file = ROOT/'reproducibility'/'expected_metrics.json'
    if expected_file.is_file():
        expected = json.loads(expected_file.read_text(encoding='utf-8'))
        if (expected['experiment_name'] == manifest['name']
                and expected['input_sha256'] == run_record['input_sha256']
                and expected['configuration_sha256'] == run_record['configuration_sha256']):
            for actual,reference in zip(records,expected['metrics'],strict=True):
                for key,value in reference.items():
                    if isinstance(value,float):
                        assert math.isclose(actual[key],value,rel_tol=1e-6,abs_tol=1e-5), f'Metric mismatch: {key}: {actual[key]} vs {value}'
                    else:
                        assert actual[key] == value, f'Metric mismatch: {key}: {actual[key]} vs {value}'
            run_record['reference_check'] = 'passed'
        else:
            raise ValueError('The experiment input or configuration differs from the recorded baseline. Generate and verify a matching baseline before comparison.')
    (output/'run_manifest.json').write_text(json.dumps(run_record,indent=2)+'\n',encoding='utf-8')
    latest = ROOT/'results'/'reproduction'/'latest_run.txt'
    latest.write_text(str(output.relative_to(ROOT)).replace(os.sep,'/')+'\n',encoding='utf-8')
    print('Results: '+str(output))
    print('Reference check: '+run_record['reference_check'])

if __name__ == '__main__':
    main()
