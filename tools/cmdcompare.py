#!/usr/bin/env python3
"""cmdcompare.py BASE [options]: checks that the working tree's commands behave exactly as BASE's did.

Builds BASE (a commit) in a git worktree, with this tree's libs/dbat/tests/ and libs/dbat/CMakeLists.txt copied over
it so that both sides run the same tests, and builds this tree, both with the debug preset. Then on each side:
  - dbat_test_commands --transcript: every command with every test argument, at three admin levels and in four
    states, each case with its own seed and the clock stopped, recording what each player saw and saved, the
    generator's state afterwards, and what is in the fixture's rooms;
  - dbat_test_techniques, if both sides have it, the same way;
  - dbat_test_bot --seed N for each seed, comparing the fingerprints of all the output.
and reports the first cases and seeds that differ. Exits 0 if nothing does.

options:
  --seeds N      bot seeds 1..N (default 240)
  --steps N      steps per bot seed (default 2000)
  --cases N      differing cases to show (default 10)
  -j N           processes (default: one per core)
  --keep         keep the work directory (transcripts, traces, the worktree)
"""
import argparse, difflib, os, re, shutil, subprocess, sys, tempfile
from concurrent.futures import ThreadPoolExecutor

ROOT = subprocess.run(['git', 'rev-parse', '--show-toplevel'], capture_output=True, text=True,
                      cwd=os.path.dirname(os.path.abspath(__file__))).stdout.strip()
TARGETS = ['dbat_test_commands', 'dbat_test_bot', 'dbat_test_techniques']


def build(tree, jobs):
    subprocess.run(['cmake', '--preset', 'debug'], cwd=tree, check=True, stdout=subprocess.DEVNULL)
    targets = [t for t in TARGETS if t != 'dbat_test_techniques' or
               os.path.exists(os.path.join(tree, 'libs/dbat/tests/test_techniques.cpp'))]
    r = subprocess.run(['cmake', '--build', '--preset', 'debug', f'-j{jobs}', '--target', *targets], cwd=tree,
                       capture_output=True, text=True)
    if r.returncode:
        sys.exit(f'cmdcompare: build failed in {tree}:\n{r.stdout[-3000:]}{r.stderr[-3000:]}')
    return os.path.join(tree, 'build/bin')


def run_transcripts(binary, out_dir, shards, temp):
    os.makedirs(out_dir, exist_ok=True)
    env = dict(os.environ, TMPDIR=temp)

    def shard(k):
        path = os.path.join(out_dir, f'{k}.txt')
        subprocess.run([binary, '--transcript', path, '--shard', f'{k}/{shards}'], cwd=os.path.dirname(binary),
                       env=env, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        return path

    with ThreadPoolExecutor(shards) as ex:
        return list(ex.map(shard, range(shards)))


def cases(path):
    """{header: text} in file order"""
    text = open(path, encoding='latin-1').read()
    parts = re.split(r'^######## ', text, flags=re.M)
    return [(p.split('\n', 1)[0], p) for p in parts if p]


def compare_transcripts(name, base_paths, new_paths, show):
    differing, total, shown = 0, 0, 0
    for b, n in zip(base_paths, new_paths):
        bc, nc = cases(b), cases(n)
        if [h for h, _ in bc] != [h for h, _ in nc]:
            print(f'{name}: the cases themselves differ in {os.path.basename(n)} ({len(bc)} against {len(nc)})')
            differing += 1
            continue
        for (h, bt), (_, nt) in zip(bc, nc):
            total += 1
            if bt == nt:
                continue
            differing += 1
            if shown < show:
                shown += 1
                print(f'--- {name}: {h}')
                diff = difflib.unified_diff(bt.splitlines(), nt.splitlines(), 'base', 'new', lineterm='', n=1)
                for line in list(diff)[2:42]:
                    print('   ', line[:200])
    print(f'{name}: {total} cases, {differing} differ')
    return differing


def bot_fingerprint(binary, seed, steps, temp, trace=None):
    with tempfile.TemporaryDirectory(prefix='cmdcompare-', dir=temp) as t:
        env = dict(os.environ, TMPDIR=t)
        if trace:
            env['BOT_TRACE'] = trace
        r = subprocess.run([binary, '--seed', str(seed), str(steps), '--journal', os.path.join(t, 'journal')],
                           cwd=os.path.dirname(binary), env=env, capture_output=True)
        m = re.search(rb'fingerprint (\w+)', r.stdout)
        return m.group(1).decode() if m else f'none (exit {r.returncode})'


def compare_bots(base_bin, new_bin, seeds, steps, jobs, work, show):
    def one(seed):
        return seed, bot_fingerprint(base_bin, seed, steps, work), bot_fingerprint(new_bin, seed, steps, work)

    with ThreadPoolExecutor(jobs) as ex:
        differ = [r for r in ex.map(one, range(1, seeds + 1)) if r[1] != r[2]]
    for seed, _, _ in differ[:show]:
        traces = [os.path.join(work, f'trace-{seed}-{side}.txt') for side in ('base', 'new')]
        bot_fingerprint(base_bin, seed, steps, work, traces[0])
        bot_fingerprint(new_bin, seed, steps, work, traces[1])
        a = open(traces[0], encoding='latin-1').read().splitlines()
        b = open(traces[1], encoding='latin-1').read().splitlines()
        print(f'--- bot seed {seed}')
        for line in list(difflib.unified_diff(a, b, 'base', 'new', lineterm='', n=1))[2:22]:
            print('   ', line[:200])
    print(f'bot: {seeds} seeds, {len(differ)} differ' + (f': {" ".join(str(d[0]) for d in differ)}' if differ else ''))
    return len(differ)


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument('base')
    p.add_argument('--seeds', type=int, default=240)
    p.add_argument('--steps', type=int, default=2000)
    p.add_argument('--cases', type=int, default=10)
    p.add_argument('-j', type=int, default=os.cpu_count())
    p.add_argument('--keep', action='store_true')
    a = p.parse_args()

    work = tempfile.mkdtemp(prefix='cmdcompare.', dir=os.environ.get('TMPDIR'))
    base_tree = os.path.join(work, 'base-tree')
    try:
        subprocess.run(['git', 'worktree', 'add', '-q', '--detach', base_tree, a.base], cwd=ROOT, check=True)
        shutil.rmtree(os.path.join(base_tree, 'libs/dbat/tests'))
        shutil.copytree(os.path.join(ROOT, 'libs/dbat/tests'), os.path.join(base_tree, 'libs/dbat/tests'))
        # where the test programs are defined; the library's sources are found by a glob, so this is safe
        shutil.copy(os.path.join(ROOT, 'libs/dbat/CMakeLists.txt'), os.path.join(base_tree, 'libs/dbat/CMakeLists.txt'))
        with ThreadPoolExecutor(2) as ex:
            base_bin, new_bin = ex.map(lambda t: build(t, max(1, a.j // 2)), [base_tree, ROOT])

        differing = 0
        for test in ('dbat_test_commands', 'dbat_test_techniques'):
            if not (os.path.exists(os.path.join(base_bin, test)) and os.path.exists(os.path.join(new_bin, test))):
                continue
            shards = a.j
            paths = [run_transcripts(os.path.join(b, test), os.path.join(work, f'{test}-{side}'), shards, work)
                     for b, side in ((base_bin, 'base'), (new_bin, 'new'))]
            differing += compare_transcripts(test, *paths, a.cases)
        differing += compare_bots(os.path.join(base_bin, 'dbat_test_bot'), os.path.join(new_bin, 'dbat_test_bot'),
                                  a.seeds, a.steps, a.j, work, a.cases)
        print('cmdcompare: SAME' if not differing else 'cmdcompare: DIFFERENT')
        return 1 if differing else 0
    finally:
        if a.keep:
            print(f'kept {work}')
        else:
            subprocess.run(['git', 'worktree', 'remove', '--force', base_tree], cwd=ROOT, capture_output=True)
            shutil.rmtree(work, ignore_errors=True)


if __name__ == '__main__':
    sys.exit(main())
