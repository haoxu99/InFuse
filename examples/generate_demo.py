"""Generate an original ASCII STL demo with no external dependencies."""
from pathlib import Path
import math
import sys

def cylinder(path, radius=1.5, height=1.0, sides=12):
    ring = [(radius * math.cos(2*math.pi*i/sides), radius * math.sin(2*math.pi*i/sides)) for i in range(sides)]
    triangles = []
    for i in range(sides):
        a, b = ring[i], ring[(i+1)%sides]
        a0, b0 = (*a, 0.0), (*b, 0.0)
        a1, b1 = (*a, height), (*b, height)
        triangles.extend(((a0,b0,b1),(a0,b1,a1),((0.0,0.0,0.0),b0,a0),((0.0,0.0,height),a1,b1)))
    lines = ['solid demo']
    for a,b,c in triangles:
        u, v = [b[i]-a[i] for i in range(3)], [c[i]-a[i] for i in range(3)]
        n = (u[1]*v[2]-u[2]*v[1],u[2]*v[0]-u[0]*v[2],u[0]*v[1]-u[1]*v[0])
        length = math.sqrt(sum(x*x for x in n))
        lines.extend(['facet normal ' + ' '.join(f'{x/length:.9g}' for x in n),'outer loop'])
        lines.extend('vertex ' + ' '.join(f'{x:.9g}' for x in p) for p in (a,b,c))
        lines.extend(('endloop','endfacet'))
    lines.append('endsolid demo')
    Path(path).parent.mkdir(parents=True, exist_ok=True)
    Path(path).write_text('\n'.join(lines)+'\n',encoding='ascii')

if __name__ == '__main__':
    path = Path(sys.argv[1]) if len(sys.argv)>1 else Path(__file__).with_name('demo.stl')
    cylinder(path)
    print(path.resolve())
