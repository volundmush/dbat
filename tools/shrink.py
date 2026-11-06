#!/usr/bin/env python3
"""shrink.py [-o OUT] [-j JOBS] [-t SECONDS] BINARY JOURNAL
Cuts a failing dbat_test_bot journal down to the fewest lines that still fail the same way, stopping
after SECONDS with the best so far. Prints the failure and the result's length; writes the result to
OUT (default JOURNAL.shrunk)."""
import argparse, concurrent.futures, os, re, signal, subprocess, sys, tempfile, time

TIMEOUT = 300
ASAN_OPTIONS = "detect_leaks=0:strict_string_checks=1:detect_stack_use_after_return=1:check_initialization_order=1:strict_memcmp=1"


def signature(output, returncode):
    """The kind of failure and where, matching what test_bot itself groups failures by."""
    summary, frame = None, None
    for line in output.splitlines():
        if summary is None:
            if line.startswith("AUDIT: "):
                summary = line
            elif "runtime error:" in line:
                summary = line[line.index("runtime error:"):]
            elif "SUMMARY: " in line:
                summary = line[line.index("SUMMARY: ") + 9:]
        if frame is None and "/libs/dbat/src/" in line and "_utils.cpp" not in line:
            match = re.search(r" in (\S+)", line)
            if match:
                frame = match.group(1)
    if summary is None:
        if returncode == 0:
            return None
        if returncode == -signal.SIGALRM:
            return "hang"
        summary = f"killed by signal {-returncode}" if returncode < 0 else f"exit status {returncode}"
    # Sanitizers quote addresses and paths that differ from run to run.
    summary = re.sub(r"0x[0-9a-f]+", "0x?", summary)
    summary = re.sub(r"\(/[^)]*\)( \(BuildId: [0-9a-f]+\))?", "", summary)
    summary = summary.replace(os.getcwd(), "")
    if summary.startswith("AUDIT: "):  # numbers are ids and vnums, which change from run to run
        summary = re.sub(r"\d+", "#", summary)
    return f"{summary.strip()} [{frame or ''}]"


WORLD_ARGS = []  # ["--world", DIR] when playing a real world


def run_journal(binary, lines, env=None):
    """Replays lines; returns the failure's signature, or None if it ran clean."""
    with tempfile.TemporaryDirectory(prefix="dbat-shrink-") as temp:
        path = os.path.join(temp, "journal.txt")
        with open(path, "w", encoding="latin-1") as f:
            f.write("".join(lines))
        environment = dict(os.environ, TMPDIR=temp, ASAN_OPTIONS=ASAN_OPTIONS)
        environment.update(env or {})
        try:
            run = subprocess.run([binary, *WORLD_ARGS, "--replay", path], stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                 env=environment, timeout=TIMEOUT, cwd=os.path.dirname(os.path.abspath(binary)))
        except subprocess.TimeoutExpired:
            return "hang"
        return signature(run.stdout.decode("latin-1"), run.returncode)


def merge_pulses(lines):
    merged = []
    for line in lines:
        match = re.match(r"(?:@\d+ )?pulse (\d+)$", line.strip())
        previous = re.match(r"pulse (\d+)$", merged[-1].strip()) if merged else None
        if match and previous:
            merged[-1] = f"pulse {int(previous.group(1)) + int(match.group(1))}\n"
        elif match:
            merged.append(f"pulse {match.group(1)}\n")
        else:
            merged.append(line)
    return merged


def shrink(binary, lines, wanted, jobs, env=None, seconds=300):
    """Delta debugging: tries removing ever smaller chunks while the same failure remains."""
    # Runs of pulses can merge into one line, when all that matters is the time passing.
    merged = merge_pulses(lines)
    if merged != lines and run_journal(binary, merged, env) == wanted:
        lines = merged
    chunk = max(len(lines) // 2, 1)
    deadline = time.time() + seconds
    with concurrent.futures.ThreadPoolExecutor(jobs) as pool:
        while True:
            removed_any = False
            start = 0
            while start < len(lines):
                if time.time() > deadline:
                    return lines
                # Try a batch of neighbouring chunks at once and keep the first that still fails.
                starts = [start + i * chunk for i in range(jobs) if start + i * chunk < len(lines)]
                attempts = [lines[:s] + lines[s + chunk:] for s in starts]
                results = list(pool.map(lambda attempt: run_journal(binary, attempt, env), attempts))
                for s, attempt, result in zip(starts, attempts, results):
                    if result == wanted:
                        lines = attempt
                        removed_any = True
                        start = s
                        break
                else:
                    start = starts[-1] + chunk
            if chunk == 1 and not removed_any:
                return lines
            if not removed_any or chunk > 1:
                chunk = max(chunk // 2, 1)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("-o", "--output")
    parser.add_argument("-j", "--jobs", type=int, default=os.cpu_count())
    parser.add_argument("-t", "--time", type=int, default=300)
    parser.add_argument("--world", help="the journal plays a copy of the world in this directory")
    parser.add_argument("binary")
    parser.add_argument("journal")
    args = parser.parse_args()

    if args.world:
        WORLD_ARGS[:] = ["--world", os.path.abspath(args.world)]
    with open(args.journal, encoding="latin-1") as f:
        lines = f.readlines()
    wanted = run_journal(args.binary, lines)
    if wanted is None:
        sys.exit(f"{args.journal} doesn't fail")
    print(f"failure: {wanted}")
    result = shrink(args.binary, lines, wanted, args.jobs, seconds=args.time)
    output = args.output or args.journal + ".shrunk"
    with open(output, "w", encoding="latin-1") as f:
        f.write("".join(result))
    print(f"{len(lines)} lines -> {len(result)}, in {output}")


if __name__ == "__main__":
    main()
