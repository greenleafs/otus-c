import os
import subprocess
import tempfile
import unittest


skipMessage = "Nothing to test. Run 'make' before!"


def is_homework_builded():
    return os.path.exists('homework')


class TestSimpleHomework3(unittest.TestCase):

    def run_homework(self, cmd, to_stdin_bytes=None):
        process = subprocess.Popen(
            cmd,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            shell=True,
        )

        if to_stdin_bytes:
            process.stdin.write(to_stdin_bytes)

        out, err = process.communicate()
        return out.decode('utf-8'), err.decode('utf-8'), process.returncode

    @unittest.skipUnless(is_homework_builded(), skipMessage)
    def test_from_cp1251(self):
        text = (
            b'\xD0\xEE\xEB\xFC\x20\xE8\x20\xE7\xED\xE0\xF7\xE5'
            b'\xED\xE8\xE5\x20\xF7\xE0\xED\xFC\xF1\xEA\xEE\xE9\x20\xEF\xF1'
            b'\xE8\xF5\xEE\xEA\xF3\xEB\xFC\xF2\xF3\xF0\xFB'
        )

        out, err, ret_code = self.run_homework(
            './homework -f cp1251',
            to_stdin_bytes=text
        )

        self.assertEqual(ret_code, 0,
                         f'Return code is not zero!\nout:\n{out}\nerr:\n{err}')
        self.assertEqual(out, 'Роль и значение чаньской психокультуры')

    @unittest.skipUnless(is_homework_builded(), skipMessage)
    def test_from_koi8r(self):
        text = (
            b'\xF2\xCF\xCC\xD8\x20\xC9\x20\xDA\xCE\xC1\xDE\xC5\xCE\xC9\xC5'
            b'\x20\xDE\xC1\xCE\xD8\xD3\xCB\xCF\xCA\x20\xD0\xD3\xC9\xC8\xCF'
            b'\xCB\xD5\xCC\xD8\xD4\xD5\xD2\xD9'
        )

        out, err, ret_code = self.run_homework(
            './homework -f koi8-r',
            to_stdin_bytes=text
        )

        self.assertEqual(ret_code, 0,
                         f'Return code is not zero!\nout:\n{out}\nerr:\n{err}')
        self.assertEqual(out, 'Роль и значение чаньской психокультуры')

    @unittest.skipUnless(is_homework_builded(), skipMessage)
    def test_from_iso8859_5(self):
        text = (
            b'\xC0\xDE\xDB\xEC\x20\xD8\x20\xD7\xDD\xD0\xE7\xD5\xDD\xD8\xD5'
            b'\x20\xE7\xD0\xDD\xEC\xE1\xDA\xDE\xD9\x20\xDF\xE1\xD8\xE5\xDE'
            b'\xDA\xE3\xDB\xEC\xE2\xE3\xE0\xEB'
        )

        out, err, ret_code = self.run_homework(
            './homework -f iso-8859-5',
            to_stdin_bytes=text
        )

        self.assertEqual(ret_code, 0,
                         f'Return code is not zero!\nout:\n{out}\nerr:\n{err}')
        self.assertEqual(out, 'Роль и значение чаньской психокультуры')

    @unittest.skipUnless(is_homework_builded(), skipMessage)
    def test_supported_encodings_list(self):
        out, _, ret_code = self.run_homework('./homework -l')
        self.assertEqual(ret_code, 0)

        waiting_for_result = (
            'Supported encodings:'
            '\tkoi8-r'
            '\tcp1251'
            '\tiso-8859-5'
        )
        result = ''.join(out.split('\n'))
        self.assertEqual(waiting_for_result, result)

    @unittest.skipUnless(is_homework_builded(), skipMessage)
    def test_help_message(self):
        out, _, ret_code = self.run_homework('./homework -h')
        self.assertEqual(ret_code, 0)

        waiting_for_result = (
            'Convert text from some character encoding to utf-8'
            'Usage: ./homework -f from-encoding [-i input file] [-o output file]'  # noqa: E501
	        '\t-f, --from-code=from-encoding - Use from-encoding for input characters.'  # noqa: E501
	        '\t-i, --input-file=file - Input file or stdin if not specified.'
	        '\t-o, --output-file=file - Output file or stdout if not specified.'
	        '\t-l, --list - List of supported encodings'
	        '\t-h, --help - This help.'

        )
        result = ''.join(out.split('\n'))
        self.assertEqual(waiting_for_result, result)

    @unittest.skipUnless(is_homework_builded(), skipMessage)
    def test_wrong_encoding(self):
        out, _, ret_code = self.run_homework('./homework -f cp866')
        self.assertEqual(ret_code, 1)

        waiting_for_result = (
            'Unsupported encoding -- cp866'
            'Supported encodings:'
            '\tkoi8-r'
            '\tcp1251'
            '\tiso-8859-5'
        )
        result = ''.join(out.split('\n'))
        self.assertEqual(waiting_for_result, result)

    @unittest.skipUnless(is_homework_builded(), skipMessage)
    def test_encoding_option_not_specfied(self):
        out, _, ret_code = self.run_homework('./homework')
        self.assertEqual(ret_code, 1)
        self.assertIn('Option is required -- \'f\'', out)

    @unittest.skipUnless(is_homework_builded(), skipMessage)
    def test_wrong_inputfile(self):
        out, _, ret_code = self.run_homework('./homework -f cp1251 -i ddd')
        self.assertEqual(ret_code, 1)

    @unittest.skipUnless(is_homework_builded(), skipMessage)
    def test_fromfile(self):
        # encoding cp1251
        text = (
            b'\xD0\xEE\xEB\xFC\x20\xE8\x20\xE7\xED\xE0\xF7\xE5'
            b'\xED\xE8\xE5\x20\xF7\xE0\xED\xFC\xF1\xEA\xEE\xE9\x20\xEF\xF1'
            b'\xE8\xF5\xEE\xEA\xF3\xEB\xFC\xF2\xF3\xF0\xFB'
        )

        with tempfile.NamedTemporaryFile() as inf:
            inf.write(text)
            inf.flush()
            out, _, ret_code = self.run_homework(
                f'./homework -f cp1251 -i {inf.name}'
            )
            self.assertEqual(ret_code, 0)
            self.assertEqual(out, 'Роль и значение чаньской психокультуры')

    @unittest.skipUnless(is_homework_builded(), skipMessage)
    def test_tofile(self):
        # encoding cp1251
        text = (
            b'\xD0\xEE\xEB\xFC\x20\xE8\x20\xE7\xED\xE0\xF7\xE5'
            b'\xED\xE8\xE5\x20\xF7\xE0\xED\xFC\xF1\xEA\xEE\xE9\x20\xEF\xF1'
            b'\xE8\xF5\xEE\xEA\xF3\xEB\xFC\xF2\xF3\xF0\xFB'
        )
        with tempfile.NamedTemporaryFile(mode='rb') as outf:
            _, _, ret_code = self.run_homework(
                f'./homework -f cp1251 -o {outf.name}',
                to_stdin_bytes=text
            )
            self.assertEqual(ret_code, 0)
            out = outf.read().decode('utf-8')
            self.assertEqual(out, 'Роль и значение чаньской психокультуры')

    @unittest.skipUnless(is_homework_builded(), skipMessage)
    def test_fromfile_tofile(self):
        # encoding cp1251
        text = (
            b'\xD0\xEE\xEB\xFC\x20\xE8\x20\xE7\xED\xE0\xF7\xE5'
            b'\xED\xE8\xE5\x20\xF7\xE0\xED\xFC\xF1\xEA\xEE\xE9\x20\xEF\xF1'
            b'\xE8\xF5\xEE\xEA\xF3\xEB\xFC\xF2\xF3\xF0\xFB'
        )
        with tempfile.NamedTemporaryFile() as inf:
            inf.write(text)
            inf.flush()
            with tempfile.NamedTemporaryFile(mode='rb') as outf:
                _, _, ret_code = self.run_homework(
                    f'./homework -f cp1251 -i {inf.name} -o {outf.name}',
                )
                self.assertEqual(ret_code, 0)
                out = outf.read().decode('utf-8')
                self.assertEqual(out, 'Роль и значение чаньской психокультуры')
