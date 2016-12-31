import asmase
import re

from tests.asmase import AsmaseTestCase


proc_maps_re = re.compile(r'[a-f0-9]+-[a-f0-9]+ .... [a-f0-9]+ [a-f0-9]+:[a-f0-9]+ [0-9]+ +(.*)')


class TestMunmap(AsmaseTestCase):
    def setUp(self):
        self.assembler = asmase.Assembler()
        self.instance = None

    def get_maps(self, instance):
        l = []
        with open('/proc/{}/maps'.format(instance.getpid()), 'r') as f:
            for line in f:
                match = proc_maps_re.fullmatch(line.rstrip('\n'))
                l.append(match.group(1))
        return l

    def test_file(self):
        with asmase.Instance(asmase.ASMASE_MUNMAP_FILE) as instance:
            paths = self.get_maps(instance)
            self.assertTrue(any(path.startswith('/memfd:asmase_tracee') for path in paths))
            file_paths = [path for path in paths
                          if path.startswith('/') and
                          not path.startswith('/memfd:asmase_tracee')]
            self.assertFalse(file_paths)

    def test_anon(self):
        with asmase.Instance(asmase.ASMASE_MUNMAP_ANON) as instance:
            self.assertNotIn('', self.get_maps(instance))

    def test_heap(self):
        with asmase.Instance(asmase.ASMASE_MUNMAP_HEAP) as instance:
            self.assertNotIn('[heap]', self.get_maps(instance))

    def _test_all(self, flags):
        with asmase.Instance(flags) as instance:
            paths = self.get_maps(instance)
            for i in range(len(paths) - 1, -1, -1):
                if paths[i].startswith('[') and paths[i] != '[heap]':
                    del paths[i]
                elif paths[i].startswith('/memfd:asmase_tracee'):
                    del paths[i]
            self.assertFalse(paths)

    def test_all(self):
        self._test_all(asmase.ASMASE_MUNMAP_ALL)

    def test_sandboxed(self):
        self._test_all(asmase.ASMASE_SANDBOX_ALL | asmase.ASMASE_MUNMAP_ALL)
