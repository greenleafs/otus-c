import collections
import os
import random
import subprocess
import unittest


skipMessage = "Nothing to test. Run 'make' before!"


def is_homework_built():
    return os.path.exists('homework')


def words(n):  # noqa
    """Helper func for generate random words."""
    symbols = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
    digits = list(range(1, 15))
    for i in range(n):
        s = random.choice(symbols)
        d = random.choice(digits)
        if i == 0 or i % 10:
            res = f'{s * d} '
        else:
            res = f'{s * d}\n'
        yield res


class TestSimpleHomework8(unittest.TestCase):

    @staticmethod
    def run_homework(cmd, to_stdin_bytes=None):
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

    @staticmethod
    def generate_file(fname, word_count=1000000):
        # Generate words and dump it to file
        _words = collections.defaultdict(int)
        with open(fname, 'w') as f:
            for w in words(word_count):
                f.write(w)
                _words[w] += 1
        return _words

    @staticmethod
    def count_words_from_file(fname):
        # Generate words and dump it to file
        _words = collections.defaultdict(int)
        with open(fname, 'r') as f:
            for line in f:
                for word in line.split():
                    _words[word] += 1
        return set(_words.items())

    @staticmethod
    def count_words_using_homework(fname):
        _words_set = set()
        result, *_ = TestSimpleHomework8.run_homework(f'./homework {fname}')
        for line in result.split('\n'):
            w_c = line.split(' ')
            if len(w_c) > 1:
                word, count = w_c[0], int(w_c[1])
                _words_set.add((word, count))
        return _words_set

    @unittest.skipUnless(is_homework_built(), skipMessage)
    def test_homework(self):
        fname = 'test_data.txt'
        reference_words_set = self.count_words_from_file(fname)
        actual_words_set = self.count_words_using_homework(fname)
        self.assertTrue(reference_words_set == actual_words_set)

    @unittest.skipUnless(is_homework_built(), skipMessage)
    def test_homework_empty_file(self):
        fname = 'test_data_empty.txt'
        reference_words_set = self.count_words_from_file(fname)
        actual_words_set = self.count_words_using_homework(fname)
        self.assertTrue(reference_words_set == actual_words_set)

    @unittest.skipUnless(is_homework_built(), skipMessage)
    def test_homework_one_word_in_file(self):
        fname = 'test_data1.txt'
        reference_words_set = self.count_words_from_file(fname)
        actual_words_set = self.count_words_using_homework(fname)
        self.assertTrue(reference_words_set == actual_words_set)
