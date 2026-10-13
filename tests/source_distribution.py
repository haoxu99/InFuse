"""Audit publishable files without inspecting generated build/output folders."""
from pathlib import Path
import re
import json
import hashlib

ROOT = Path(__file__).resolve().parents[1]
SKIP = {'.git','.vs','__pycache__','results','downloads','dependencies','dist'}
FILES = []
for path in ROOT.rglob('*'):
    relative = path.relative_to(ROOT)
    if any(part in SKIP or part.startswith('build') for part in relative.parts):
        continue
    if any(ord(c)>127 for c in relative.as_posix()):
        raise AssertionError('Non-English filename: '+relative.as_posix())
    if not path.is_file():
        continue
    FILES.append(path)
    if path.suffix in ('.cpp','.h','.hpp','.md','.txt','.ini','.json','.py','.ps1','.yml','.bib','.svg','.stl','.csv') or path.name in ('CMakeLists.txt','.gitignore','.gitattributes'):
        text = path.read_text(encoding='utf-8-sig')
        if any(ord(c)>127 for c in text):
            raise AssertionError('Non-English/non-ASCII content: '+relative.as_posix())
        obsolete = ('ACAP'+'Injection','ACAP'+'_clean','acap'+'.exe','Usage: '+'acap')
        if any(marker in text for marker in obsolete):
            raise AssertionError('Obsolete project branding: '+relative.as_posix())
    if path.suffix in ('.cpp','.h','.hpp'):
        for match in re.finditer(r'#\s*include\s*"([^"]+)"',path.read_text(encoding='utf-8')):
            assert (path.parent/match[1]).is_file(), f'Missing local include: {relative}: {match[1]}'
model_metadata = json.loads((ROOT/'examples'/'models'/'model.json').read_text(encoding='utf-8'))
model = ROOT/'examples'/'models'/model_metadata['filename']
assert model.stat().st_size == model_metadata['size_bytes'], 'Supplied model size changed.'
assert hashlib.sha256(model.read_bytes()).hexdigest() == model_metadata['sha256'], 'Supplied model bytes changed.'
experiment = json.loads((ROOT/'reproducibility'/'experiment.json').read_text(encoding='utf-8'))
assert (ROOT/experiment['input']).resolve() == model.resolve(), 'Default experiment does not use the selected bunny.'
assert experiment['modes'] == ['shell'], 'Default experiment is not the selected single-cavity workflow.'
print(f'PASS: {len(FILES)} publishable files; English filenames/text, InFuse branding, local includes, and bunny single-cavity input integrity.')
