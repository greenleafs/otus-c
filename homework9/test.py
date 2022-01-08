import os
import unittest


skipMessage = "Nothing to test. Run 'make' before!"


def is_homework_built():
    return os.path.exists('homework')


class TestSimple(unittest.TestCase):

    @unittest.skipUnless(is_homework_built(), skipMessage)
    def test_simple(self):
        self.assertTrue(False, 'Simple template test.')
