#!/usr/bin/env python3
"""coverage_report.py root build out: turns gcov's data into a summary, per-file table and never-run list."""
import collections, glob, json, os, subprocess, sys

root, build, out = sys.argv[1:4]
src_prefix = os.path.join(root, "libs/dbat/src") + "/"
lines = collections.defaultdict(dict)  # file -> line -> count
funcs = collections.defaultdict(dict)  # file -> function -> (count, first line)

for gcda in glob.glob(build + "/**/*.gcda", recursive=True):
    run = subprocess.run(["gcov", "--json-format", "--stdout", gcda], capture_output=True, cwd=os.path.dirname(gcda))
    for text in run.stdout.decode("utf-8", "replace").splitlines():
        if not text.strip():
            continue
        for entry in json.loads(text)["files"]:
            name = os.path.normpath(entry["file"] if entry["file"].startswith("/") else os.path.join(root, entry["file"]))
            if not name.startswith(src_prefix):
                continue
            short = name[len(src_prefix):]
            for line in entry["lines"]:
                lines[short][line["line_number"]] = max(lines[short].get(line["line_number"], 0), line["count"])
            for func in entry["functions"]:
                funcs[short][func["demangled_name"]] = (func["execution_count"], func["start_line"])

# A file with no .gcda at all counts as wholly unreached, but gcov only knows about files it was given.
rows = []
for short in sorted(lines):
    total = len(lines[short])
    hit = sum(1 for count in lines[short].values() if count)
    fn_total = len(funcs[short])
    fn_hit = sum(1 for count, _ in funcs[short].values() if count)
    rows.append((short, hit, total, fn_hit, fn_total))

all_hit, all_total = sum(r[1] for r in rows), sum(r[2] for r in rows)
fn_hit, fn_total = sum(r[3] for r in rows), sum(r[4] for r in rows)
pct = lambda a, b: 100.0 * a / b if b else 0.0

with open(os.path.join(out, "files.txt"), "w") as f:
    f.write(f"{'lines':>7} {'hit/total':>13} {'funcs':>7}  file (lowest first)\n")
    for short, hit, total, fh, ft in sorted(rows, key=lambda r: (pct(r[1], r[2]), -r[2])):
        f.write(f"{pct(hit, total):6.1f}% {hit:>6}/{total:<6} {fh:>3}/{ft:<3}  {short}\n")

with open(os.path.join(out, "never-run.txt"), "w") as f:
    for short in sorted(funcs):
        for name, (count, line) in sorted(funcs[short].items(), key=lambda kv: kv[1][1]):
            if not count:
                f.write(f"{short}:{line} {name}\n")

by_dir = collections.defaultdict(lambda: [0, 0])
for short, hit, total, _, _ in rows:
    top = short.split("/")[0]
    by_dir[top][0] += hit
    by_dir[top][1] += total

summary = [f"lines {all_hit}/{all_total} = {pct(all_hit, all_total):.1f}%",
           f"functions {fn_hit}/{fn_total} = {pct(fn_hit, fn_total):.1f}%", ""]
for top, (hit, total) in sorted(by_dir.items(), key=lambda kv: pct(*kv[1])):
    summary.append(f"{pct(hit, total):6.1f}% {hit:>6}/{total:<6} {top}/")
text = "\n".join(summary) + "\n"
open(os.path.join(out, "summary.txt"), "w").write(text)
sys.stdout.write(text)
print(f"\nDetails: {out}/files.txt and {out}/never-run.txt")
