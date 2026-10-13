"""Exercise both retained pipelines, batching, output isolation, and CLI failures."""
from pathlib import Path
import re
import subprocess
import sys
import tempfile

exe, source = Path(sys.argv[1]).resolve(), Path(sys.argv[2]).resolve()
sys.path.insert(0, str(source / 'examples'))
from generate_demo import cylinder

def run(args, cwd, success=True):
    try:
        result = subprocess.run([str(exe), *map(str,args)], cwd=cwd, capture_output=True, text=True, errors='replace', timeout=180)
    except subprocess.TimeoutExpired as error:
        raise AssertionError(f'Pipeline timed out: {args}\nLast output: {error.stdout!r}') from error
    if (result.returncode == 0) != success:
        raise AssertionError(f'Unexpected exit {result.returncode}: {args}\n{result.stdout}\n{result.stderr}')
    return result

def verify(output, stem):
    folder = output / stem
    gcode = folder / 'Blocks_files' / (stem + '_path.gcode')
    assert gcode.is_file() and gcode.stat().st_size > 0, f'Missing G-code: {gcode}'
    text = gcode.read_text(encoding='utf-8')
    assert re.search(r'^G1\s', text, re.M), 'No toolpath motion commands'
    assert not re.search(r'\b(?:nan|inf)\b',text,re.I), 'Nonfinite G-code coordinate'
    patches = list((folder / 'patches').glob('patch_*.obj'))
    assert patches and all(p.stat().st_size>0 for p in patches), 'Missing patch meshes'
    return (folder / 'work' / (stem + '_paths_step.txt')).read_bytes(), (folder / 'work' / 'Injection_Order.txt').read_bytes()

with tempfile.TemporaryDirectory(prefix='infuse-smoke-') as temp:
    base = Path(temp)
    input_dir = base / 'input models'
    cylinder(input_dir / 'demo.stl')
    cylinder(input_dir / 'second.STL',radius=1.2,height=0.8,sides=10)
    common = ['--config',source/'config.ini','--templates',source/'slice']
    run(['--help'],base)
    run([],base,False)
    run(['--input',base/'missing.stl',*common],base,False)
    run(['--input',input_dir,'--mode','unknown',*common],base,False)
    for mode in ('shell','fusion'):
        single, batch = base / (mode+' single'), base / (mode+' batch')
        run(['--input',input_dir/'demo.stl','--output',single,'--mode',mode,*common],base)
        expected = verify(single,'demo')
        run(['--input',input_dir,'--output',batch,'--mode',mode,*common],base)
        assert verify(batch,'demo') == expected, 'Batch changed the first model paths'
        verify(batch,'second')
        second_only = base / (mode+' second')
        run(['--input',input_dir/'second.STL','--output',second_only,'--mode',mode,*common],base)
        assert verify(second_only,'second') == verify(batch,'second'), 'State leaked between models'
        run(['--input',input_dir/'demo.stl','--output',single,'--mode',mode,*common],base,False)
    invalid = base / 'invalid.stl'
    invalid.write_bytes(b'not an ASCII STL\0\0')
    run(['--input',invalid,'--output',base/'invalid-output',*common],base,False)
    invalid_config = base / 'invalid.ini'
    invalid_config.write_text((source/'config.ini').read_text(encoding='utf-8').replace('layer_height = 0.2','layer_height = 0'),encoding='utf-8')
    run(['--input',input_dir/'demo.stl','--output',base/'zero-height','--config',invalid_config,'--templates',source/'slice'],base,False)
print('PASS: shell/fusion pipelines, batch equivalence, output isolation, and invalid input.')
