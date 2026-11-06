#!/usr/bin/env python3
"""fuzz.py TARGET [options]
Runs one of the coverage-guided fuzz targets (load, boot, script, bot) on many cores, carrying on past
what it finds, and sorts what it found by where it happened. Build the fuzz preset first:
  cmake --preset fuzz && cmake --build --preset fuzz
Everything goes under --out, including a corpus that later runs carry on from. See tools/README.md."""
import argparse, glob, os, re, shutil, signal, subprocess, sys, tempfile, threading, time
from collections import Counter
from concurrent.futures import ThreadPoolExecutor

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TESTS = os.path.join(ROOT, "libs/dbat/tests")
FIXTURE = os.path.join(TESTS, "fixtures")
# in the order that fuzz_load.cpp and fuzz_boot.cpp number them
LOAD_FILES = ["accounts/plrfiles/K-O/kelvarin.plr", "accounts/plrobjs/K-O/kelvarin.new", None, None]
BOOT_FILES = ["world/wld/3.wld", "world/mob/3.mob", "world/obj/3.obj", "world/zon/3.zon", "world/trg/3.trg",
              "world/shp/3.shp", "world/gld/3.gld", "world/wld/index", "world/mob/index", "world/obj/index",
              "world/zon/index", "world/trg/index", "world/shp/index", "world/gld/index", "etc/config",
              "etc/assemblies", "etc/badsites", "etc/hcontrol", "etc/levels", "etc/time",
              "etc/auction", "etc/boards", "etc/plrmail", "plrfiles/index", "misc/socials",
              "misc/socials.new", "misc/messages", "text/help/help.hlp", "text/help/index", "disabled.cmds"]


def write_seed(corpus, name, data):
    with open(os.path.join(corpus, name), "wb") as f:
        f.write(data)


def seeds_load(corpus):
    for number, path in enumerate(LOAD_FILES):
        if path:
            with open(os.path.join(FIXTURE, path), "rb") as f:
                write_seed(corpus, f"file{number}", bytes([number]) + f.read())
    write_seed(corpus, "aliases", bytes([2]) + b"look\nl\n0\n")
    write_seed(corpus, "vars", bytes([3]) + b"name 0 value\n")


def seeds_boot(corpus):
    for number, path in enumerate(BOOT_FILES):
        full = os.path.join(FIXTURE, "world", path)
        if os.path.isfile(full):
            with open(full, "rb") as f:
                write_seed(corpus, f"file{number}", bytes([number]) + f.read()[:60000])


def seeds_script(corpus):
    with open(os.path.join(FIXTURE, "world/world/trg/3.trg"), "rb") as f:
        triggers = f.read().split(b"\n#")
    for number, trigger in enumerate(triggers):
        parts = trigger.split(b"~\n")
        if len(parts) >= 3:
            write_seed(corpus, f"script{number}", bytes([number % 3]) + parts[2])


def seeds_bot(corpus, binary):
    for path in glob.glob(os.path.join(TESTS, "corpus", "*.txt")):
        shutil.copy(path, os.path.join(corpus, os.path.basename(path)))
    # the bot program built alongside, for some games to start from
    bot = os.path.join(os.path.dirname(binary), "dbat_test_bot")
    with tempfile.TemporaryDirectory(dir=corpus + "/..") as work:
        for seed in range(1, 41):
            journal = os.path.join(corpus, f"bot{seed}")
            subprocess.run([bot, "--seed", str(seed), "60", "--journal", journal], cwd=work, stdout=subprocess.DEVNULL,
                           stderr=subprocess.DEVNULL, env=dict(os.environ, TMPDIR=work, ASAN_OPTIONS="detect_leaks=0"))


def make_dictionary(path, target):
    """Words the game looks for, which a fuzzer would take a long time to come across by chance."""
    words = set()
    source = os.path.join(ROOT, "libs/dbat/src")
    if target == "bot":
        with open(os.path.join(source, "commands/command.cpp"), errors="replace") as f:
            words |= set(re.findall(r'\{\s*"([a-z][a-z_]{1,15})"', f.read()))
        words |= {"type ", "pulse ", "clock ", "leave ", "connect ", "spawn ", "login ", "Bot0", "Bot1"}
    elif target == "script":
        for name in ("dg/dg_variables.cpp", "dg/dg_scripts.cpp", "dg/dg_mobcmd.cpp", "dg/dg_objcmd.cpp", "dg/dg_wldcmd.cpp"):
            with open(os.path.join(source, name), errors="replace") as f:
                words |= set(re.findall(r'"([a-z][a-z_]{1,15})"', f.read()))
        words |= {"%actor%", "%self%", "%actor.name%", "%self.room%", "%random.char%", "%echo%", "%purge%", "%force%",
                  "%teleport%", "%load%", "wait 1", "if ", "elseif ", "else", "end", "while ", "done", "switch ", "case ",
                  "break", "return 0", "halt", "eval ", "set ", "global ", "remote ", "rdelete ", "context ", "unset ",
                  "extract ", "dg_letter ", "makeuid ", "nop "}
    elif target == "load":
        with open(os.path.join(source, "db/players.cpp"), errors="replace") as f:
            words |= {t + ": " for t in re.findall(r'strcmp\(tag, "([^"]{4})"\)', f.read())}
    with open(path, "w") as f:
        for word in sorted(words):
            escaped = word.replace("\\", "\\\\").replace('"', '\\"')
            f.write(f'"{escaped}"\n')


def sweep(out, stop):
    """Crashes and timeouts leave their game's temp directories behind, which adds up."""
    while not stop.wait(60):
        for path in glob.glob(os.path.join(out, "dbat-test.*")) + glob.glob(os.path.join(out, "libFuzzerTemp.*", "dbat-test.*")):
            try:
                if time.time() - os.path.getmtime(path) > 120:
                    shutil.rmtree(path, ignore_errors=True)
            except OSError:
                pass


def describe(binary, artifact, out):
    """Runs one saved input again, to see what it does."""
    with tempfile.TemporaryDirectory(dir=out) as work:
        try:
            result = subprocess.run([binary, artifact, "-timeout=60", "-detect_leaks=0"], cwd=work, capture_output=True, timeout=120,
                                    env=dict(os.environ, TMPDIR=work, ASAN_OPTIONS="detect_leaks=0:handle_abort=1", UBSAN_OPTIONS="print_stacktrace=1"))
        except subprocess.TimeoutExpired:
            return ("hang", "?") if os.path.basename(artifact).startswith("timeout") else ("slow", "?")
    text = result.stdout.decode(errors="replace") + result.stderr.decode(errors="replace")
    if result.returncode == 0:
        return ("does not repeat", "?")
    match = re.search(r"(AUDIT: .*|runtime error: .*|ERROR: AddressSanitizer: [a-zA-Z-]+|ERROR: libFuzzer: [a-z-]+|ALARM: .*)", text)
    kind = re.sub(r"0x[0-9a-f]+", "", match.group(1) if match else f"status {result.returncode}")[:90]
    kind = re.sub(r"\b\d+\b", "#", kind) if kind.startswith("runtime") else kind
    # the accessors only say where a bad pointer happened to get used
    frames = re.findall(r"#\d+ \S+ in (\S+) (\S*libs/dbat/src/\S+?):(\d+)", text)
    frames = [frame for frame in frames if "_utils.cpp" not in frame[1]] or frames
    where = f"{frames[0][0]} {frames[0][1].split('libs/dbat/')[1]}:{frames[0][2]}" if frames else "?"
    return (kind, where)


def summarize(out, binary, artifacts, jobs):
    """Each failure once, by the first frame of the game's own code."""
    found = Counter()
    example = {}
    paths = sorted(glob.glob(os.path.join(artifacts, "*")))
    with ThreadPoolExecutor(jobs) as pool:
        keys = list(pool.map(lambda artifact: describe(binary, artifact, out), paths))
    for artifact, key in zip(paths, keys):
        found[key] += 1
        example.setdefault(key, os.path.basename(artifact))
    with open(os.path.join(out, "report.txt"), "w") as f:
        for (kind, where), count in found.most_common():
            line = f"{count:4} {where}  {kind}  ({example[(kind, where)]})"
            print(line)
            f.write(line + "\n")
    print(f"{len(found)} distinct, in {out}/report.txt")


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("target", choices=["load", "boot", "script", "bot"])
    parser.add_argument("--binary-dir", default=os.path.join(ROOT, "build-fuzz/bin"))
    parser.add_argument("--out", default=None)
    parser.add_argument("-j", "--jobs", type=int, default=8)
    parser.add_argument("-t", "--time", type=int, default=600, help="seconds")
    parser.add_argument("--max-len", type=int, default=0, help="longest input, in bytes")
    args = parser.parse_args()
    binary = os.path.abspath(os.path.join(args.binary_dir, f"dbat_fuzz_{args.target}"))
    out = os.path.abspath(args.out or os.path.join(ROOT, "build-fuzz/fuzz", args.target))
    corpus, artifacts, logs = (os.path.join(out, name) for name in ("corpus", "artifacts", "logs"))
    for path in (corpus, artifacts, logs):
        os.makedirs(path, exist_ok=True)
    if not os.listdir(corpus):
        if args.target == "bot":
            seeds_bot(corpus, binary)
        else:
            {"load": seeds_load, "boot": seeds_boot, "script": seeds_script}[args.target](corpus)
    dictionary = os.path.join(out, "dictionary")
    make_dictionary(dictionary, args.target)
    limits = {"load": 65536, "boot": 65536, "script": 4096, "bot": 8192}
    command = [binary, corpus, f"-fork={args.jobs}", f"-max_total_time={args.time}", "-ignore_crashes=1", "-ignore_timeouts=1",
               "-ignore_ooms=1", f"-artifact_prefix={artifacts}/", f"-max_len={args.max_len or limits[args.target]}", "-timeout=30",
               "-rss_limit_mb=4096", "-close_fd_mask=1", "-print_final_stats=1", "-detect_leaks=0"]
    if os.path.getsize(dictionary):
        command.append(f"-dict={dictionary}")
    env = dict(os.environ, ASAN_OPTIONS="detect_leaks=0:handle_abort=1", TMPDIR=out)
    stop = threading.Event()
    threading.Thread(target=sweep, args=(out, stop), daemon=True).start()
    with open(os.path.join(logs, "fuzzer.log"), "w") as log:
        fuzzer = subprocess.Popen(command, cwd=out, stdout=log, stderr=subprocess.STDOUT, env=env, start_new_session=True)
        try:
            status = fuzzer.wait(timeout=args.time + 180)
        except subprocess.TimeoutExpired:
            # a stuck worker would otherwise keep libFuzzer waiting for it forever
            print("fuzzer did not stop in time, killing it")
            os.killpg(fuzzer.pid, signal.SIGKILL)
            status = fuzzer.wait()
    stop.set()
    for path in glob.glob(os.path.join(out, "dbat-test.*")):
        shutil.rmtree(path, ignore_errors=True)
    print(f"fuzzer finished with status {status}, {len(os.listdir(corpus))} inputs in the corpus, {len(os.listdir(artifacts))} artifacts")
    summarize(out, binary, artifacts, args.jobs)
    return 0


if __name__ == "__main__":
    sys.exit(main())
