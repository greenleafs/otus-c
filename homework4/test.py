import os
import subprocess
import unittest


skipMessage = "Nothing to test. Run 'make' before!"


def is_homework_built():
    return os.path.exists('homework')


class Homework4TestCase(unittest.TestCase):

    def run_homework(self, cmd):
        process = subprocess.Popen(
            cmd,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            shell=True,
        )

        out, err = process.communicate()
        return (
            out.decode('utf-8'),
            err.decode('utf-8'),
            process.returncode
        )

    @unittest.skipUnless(is_homework_built(), skipMessage)
    def test_isnot_zipjpeg(self):
        file = 'test_files/non-zipjpeg.jpg'
        out, _, ret_code = self.run_homework(f'./homework {file}')
        self.assertEqual(ret_code, 0)
        self.assertIn('It isn\'t zip file.', out)

    @unittest.skipUnless(is_homework_built(), skipMessage)
    def test_is_zipjpeg(self):
        file = 'test_files/zipjpeg.jpg'
        out, _, ret_code = self.run_homework(f'./homework {file}')
        self.assertEqual(ret_code, 0)
        self.assertIn('Files: 185', out)
        self.assertIn('jpeg-9d/maktjpeg.st', out)

    @unittest.skipUnless(is_homework_built(), skipMessage)
    def test_not_exist_file(self):
        file = 'test_files/not_exist.zip'
        out, err, ret_code = self.run_homework(f'./homework {file}')
        self.assertEqual(ret_code, 0)
        self.assertIn('No such file or directory', err)
