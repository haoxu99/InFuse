"""Test paper-stage export without rerunning the geometric planner."""
import csv
import json
from pathlib import Path
import sys
import tempfile
import unittest
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'scripts'))
from paper_result import export_paper_result


class PaperResultTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix='infuse-paper-test-')
        self.addCleanup(self.temporary.cleanup)
        self.output = Path(self.temporary.name)
        self.model = self.output/'shell'/'sample'
        (self.model/'work').mkdir(parents=True)
        (self.model/'patches').mkdir()
        # One contour record with one printing polyline and one injection marker.
        (self.model/'work'/'sample_paths.txt').write_text(
            '1\n2\n2\n0 0 0\n1 0 0\n1\n1 0 0\n', encoding='utf-8')
        for index in range(2):
            (self.model/'patches'/f'patch_{index}.obj').write_text('v 0 0 0\n', encoding='utf-8')
        self.patches = [[[(0., 0., 0.), (1., 0., 0.)]],
                        [[(1., 0., 0.), (1., 0., 1.)]]]
        self.manifest = {'paper_result': {'figure': 'Test figure',
                        'printing_stage_count': 2, 'injection_event_count': 1}}

    def export(self):
        return export_paper_result(self.output, self.model, 'sample', self.patches, self.manifest)

    def test_standard_outputs_and_cumulative_mapping(self):
        report = self.export()
        self.assertEqual(report['stage_event_count_check'], 'passed')
        self.assertEqual(len(report['stages']), 2)
        with (self.output/'paper_stages.csv').open(encoding='utf-8', newline='') as stream:
            rows = list(csv.DictReader(stream))
        self.assertEqual(rows[0]['next_event'], 'I1')
        self.assertEqual(rows[1]['next_event'], 'complete')
        self.assertEqual(rows[1]['cumulative_objs'],
                         'shell/sample/patches/patch_0.obj;shell/sample/patches/patch_1.obj')
        with (self.output/'injection_events.csv').open(encoding='utf-8', newline='') as stream:
            events = list(csv.DictReader(stream))
        self.assertEqual(len(events), 1)
        self.assertEqual(events[0]['after_printing_stage'], 'P1')
        self.assertEqual(float(events[0]['x_mm']), 1.)
        self.assertEqual(events[0]['layer_record_index'], '0')
        self.assertEqual(json.loads((self.output/'paper_result.json').read_text())['generated_injection_events'], 1)
        svg = ET.parse(self.output/'paper_figure.svg')
        ns = {'svg': 'http://www.w3.org/2000/svg'}
        self.assertEqual(len(svg.findall('.//svg:use', ns)), 3)
        self.assertEqual(len(svg.findall('.//svg:circle', ns)), 1)

    def test_incorrect_count_is_rejected(self):
        self.manifest['paper_result']['printing_stage_count'] = 3
        with self.assertRaisesRegex(ValueError, 'counts'):
            self.export()

    def test_missing_mesh_is_rejected(self):
        (self.model/'patches'/'patch_1.obj').unlink()
        with self.assertRaisesRegex(ValueError, 'Missing paper-stage mesh'):
            self.export()

    def test_non_paper_experiment_is_unchanged(self):
        self.assertIsNone(export_paper_result(self.output, self.model, 'sample', self.patches, {}))
        self.assertFalse((self.output/'paper_result.json').exists())


if __name__ == '__main__':
    unittest.main()
