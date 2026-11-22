"""Run the existing top-level native checks on Linux with GCC; no game required."""
import argparse
import os
from pathlib import Path
import subprocess
import sys


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cc', default=os.environ.get('CC', 'gcc'))
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    out = root / 'build' / 'portable-checks'
    out.mkdir(parents=True, exist_ok=True)
    names = ('test_game_identity', 'test_release_monitor',
             'test_policy_regression', 'test_specialist_room')
    for name in names:
        executable = out / name
        subprocess.run([args.cc, '-std=c11', '-O2', '-Wall', '-Wextra',
                        '-Wno-unused-function', str(root / 'tests' / (name + '.c')),
                        '-o', str(executable)], check=True)
        result = subprocess.run([str(executable)], text=True, stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT)
        (out / (name + '.txt')).write_text(result.stdout, encoding='utf-8')
        print(name + ': ' + result.stdout.rstrip().split('\n')[-1], flush=True)
        result.check_returncode()
    result = subprocess.run([sys.executable, str(root / 'tests' / 'test_roster_owner_index.py'),
                             '--cc', args.cc, '--out', str(out / 'roster_owner_index')],
                            text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    (out / 'test_roster_owner_index.txt').write_text(result.stdout, encoding='utf-8')
    print('test_roster_owner_index: ' + result.stdout.rstrip().split('\n')[-1], flush=True)
    result.check_returncode()
    result = subprocess.run([sys.executable, str(root / 'tests' / 'test_cache_stack.py'),
                             '--cc', args.cc, '--out', str(out / 'cache_stack')],
                            text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    (out / 'test_cache_stack.txt').write_text(result.stdout, encoding='utf-8')
    print('test_cache_stack: ' + result.stdout.rstrip().split('\n')[-1], flush=True)
    result.check_returncode()
    print('PASS: all four portable check programs and the roster owner index and cache stack property tests.')


if __name__ == '__main__':
    main()
