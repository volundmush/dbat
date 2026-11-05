#!/usr/bin/env python3
"""hunt.py --binary dbat_test_bot [options]
Runs the seeded bots on every core until told to stop, groups failures by cause, shrinks one example
of each, and keeps a report. Seeds carry on from where the last hunt in OUT stopped, and failures it
already knows about aren't reported as new. Stop it with Ctrl-C; the report is always up to date."""
import argparse, json, os, re, shutil, signal, subprocess, sys, tempfile, threading, time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import shrink

MAX_KEPT_REPORT = 200_000  # bytes of one sanitizer report to keep


def default_jobs(world):
    """One per core, unless memory runs out first: an ASan worker takes about this much."""
    per_worker = 750 if world else 150  # MB
    with open("/proc/meminfo") as f:
        available = next(int(line.split()[1]) for line in f if line.startswith("MemAvailable")) // 1024
    return max(1, min(os.cpu_count(), (available - 2048) // per_worker))


class Hunt:
    def __init__(self, args):
        self.args = args
        self.lock = threading.Lock()
        self.stop = threading.Event()
        self.state_path = os.path.join(args.out, "state.json")
        self.state = {"next_seed": args.start, "seeds_run": 0, "findings": {}}
        if os.path.exists(self.state_path):
            with open(self.state_path) as f:
                self.state = json.load(f)
        self.started = time.time()
        self.seeds_this_run = 0
        self.to_shrink = []

    def next_seed(self):
        with self.lock:
            seed = self.state["next_seed"]
            self.state["next_seed"] += 1
            return seed

    def worker(self):
        while not self.stop.is_set():
            if self.args.time and time.time() - self.started > self.args.time:
                return
            self.run_seed(self.next_seed())

    def run_seed(self, seed):
        with tempfile.TemporaryDirectory(prefix="dbat-hunt-") as temp:
            journal = os.path.join(temp, "journal.txt")
            report = os.path.join(temp, "report.txt")
            env = dict(os.environ, TMPDIR=temp, ASAN_OPTIONS=shrink.ASAN_OPTIONS)
            command = [self.args.binary, *shrink.WORLD_ARGS, "--seed", str(seed), str(self.args.steps), "--journal", journal]
            with open(report, "wb") as out:
                try:
                    run = subprocess.run(command, stdout=out, stderr=subprocess.STDOUT, env=env,
                                         cwd=os.path.dirname(os.path.abspath(self.args.binary)),
                                         timeout=self.args.seed_timeout, preexec_fn=lambda: os.nice(10))
                    returncode = run.returncode
                except subprocess.TimeoutExpired:
                    returncode = -signal.SIGALRM
            with open(report, "rb") as f:
                text = f.read(MAX_KEPT_REPORT).decode("latin-1")
            found = shrink.signature(text, returncode)
            # A message per player would hide everything else, and the name isn't the point.
            logged = {re.sub(r"(plr\w+/[A-Z]-[A-Z]/)[^. ]+", r"\1NAME", line[8:]) for line in text.splitlines() if line.startswith("LOGGED: ")}
            with self.lock:
                self.state["seeds_run"] += 1
                self.seeds_this_run += 1
                if found:
                    self.record(found, seed, journal, text)
                for message in logged:
                    self.record("logged: " + message, seed, journal, text)

    def record(self, found, seed, journal, text):
        findings = self.state["findings"]
        if found in findings:
            findings[found]["count"] += 1
        else:
            number = len(findings) + 1
            directory = os.path.join(self.args.out, f"finding-{number:03}")
            os.makedirs(directory, exist_ok=True)
            shutil.copy(journal, os.path.join(directory, "journal.txt"))
            with open(os.path.join(directory, "report.txt"), "w", encoding="latin-1") as f:
                f.write(text)
            findings[found] = {"number": number, "count": 1, "seed": seed, "steps": self.args.steps, "shrunk_lines": None}
            self.to_shrink.append(found)
            print(f"NEW {number:03}: {found}", flush=True)
        self.save()

    def save(self):
        temporary = self.state_path + ".tmp"
        with open(temporary, "w") as f:
            json.dump(self.state, f, indent=1)
        os.replace(temporary, self.state_path)
        lines = [f"# Hunt: {self.state['seeds_run']} seeds run, {len(self.state['findings'])} distinct failures", ""]
        for found, info in sorted(self.state["findings"].items(), key=lambda kv: kv[1]["number"]):
            lines.append(f"- **{info['number']:03}** seen {info['count']}x: `{found}`")
            shrunk = f"{info['shrunk_lines']} lines" if info["shrunk_lines"] else "not shrunk yet"
            lines.append(f"  - seed {info['seed']}; `finding-{info['number']:03}/` ({shrunk})")
        with open(os.path.join(self.args.out, "report.md"), "w") as f:
            f.write("\n".join(lines) + "\n")

    def shrink_findings(self):
        for found, info in sorted(self.state["findings"].items(), key=lambda kv: kv[1]["number"]):
            if info["shrunk_lines"]:
                continue
            if found.startswith("logged: ") and not self.args.shrink_logged:
                continue
            directory = os.path.join(self.args.out, f"finding-{info['number']:03}")
            with open(os.path.join(directory, "journal.txt"), encoding="latin-1") as f:
                lines = f.readlines()
            # A logged message doesn't end a run by itself, so ask the bot to stop at this one.
            env = {"DBAT_SYSERR": found[8:]} if found.startswith("logged: ") else {}
            wanted = f"AUDIT: {found} []" if env else found
            replayed = shrink.run_journal(self.args.binary, lines, env)
            if replayed != wanted:
                # Noted, not hidden: a failure that a replay can't repeat needs a closer look.
                print(f"{info['number']:03}: replay gave {replayed!r}", flush=True)
                continue
            result = shrink.shrink(self.args.binary, lines, wanted, self.args.jobs, env, self.args.shrink_time)
            with open(os.path.join(directory, "shrunk.txt"), "w", encoding="latin-1") as f:
                f.write("".join(result))
            info["shrunk_lines"] = len(result)
            print(f"{info['number']:03}: {len(lines)} lines -> {len(result)}", flush=True)
            self.save()


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--binary", required=True)
    parser.add_argument("--out", default="hunt")
    parser.add_argument("-j", "--jobs", type=int, help="workers (default: one per core, as memory allows)")
    parser.add_argument("-t", "--time", type=int, default=0, help="stop after this many seconds (default: until Ctrl-C)")
    parser.add_argument("--steps", type=int, default=2000)
    parser.add_argument("--start", type=int, default=1, help="first seed, for a new hunt")
    parser.add_argument("--seed-timeout", type=int, default=600)
    parser.add_argument("--no-shrink", action="store_true")
    parser.add_argument("--shrink-time", type=int, default=120, help="seconds to spend on each failure")
    parser.add_argument("--shrink-logged", action="store_true", help="also shrink failures that were only logged")
    parser.add_argument("--world", help="play a copy of the world in this directory, instead of the fixture")
    args = parser.parse_args()
    args.binary = os.path.abspath(args.binary)
    args.jobs = args.jobs or default_jobs(args.world)
    print(f"{args.jobs} workers", flush=True)
    if args.world:
        shrink.WORLD_ARGS[:] = ["--world", os.path.abspath(args.world)]
    os.makedirs(args.out, exist_ok=True)

    hunt = Hunt(args)
    signal.signal(signal.SIGINT, lambda *_: hunt.stop.set())
    signal.signal(signal.SIGTERM, lambda *_: hunt.stop.set())
    threads = [threading.Thread(target=hunt.worker, daemon=True) for _ in range(args.jobs)]
    for thread in threads:
        thread.start()
    while any(thread.is_alive() for thread in threads):
        time.sleep(1)
    hunt.save()
    print(f"{hunt.seeds_this_run} seeds this run, {hunt.state['seeds_run']} in all, "
          f"{len(hunt.state['findings'])} distinct failures", flush=True)
    if not args.no_shrink:
        hunt.shrink_findings()
    hunt.save()
    print(f"Report: {os.path.join(args.out, 'report.md')}")


if __name__ == "__main__":
    main()
