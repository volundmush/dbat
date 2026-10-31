#!/usr/bin/env python3
"""objdiff.py <ref-obj-dir> <new-obj-dir>: compare normalised disassembly of every function, matched by
name across all object files (so functions may move between files). Prints functions that differ,
appear, or disappear. Exit status 1 if anything differs. Set OBJDIFF_RENAME for intentional renames."""
import collections, os, re, subprocess, sys

# OBJDIFF_RENAME="old=new,old2=new2": symbols renamed on purpose, treated as equal on the ref side.
RENAME = dict(p.split('=', 1) for p in os.environ.get('OBJDIFF_RENAME', '').split(',') if '=' in p)

def functions(objdir):
    out = subprocess.run(f'objdump -d --no-show-raw-insn -r {objdir}/*.o', shell=True,
                         capture_output=True, text=True).stdout
    fns = collections.defaultdict(list)
    cur = None
    for line in out.splitlines():
        m = re.match(r'^[0-9a-f]+ <(.+)>:$', line)
        if m:
            cur = re.sub(r'\.(isra|constprop|part|cold)\.\d+', '', m.group(1))
            fns[cur].append([])
            continue
        if line.startswith('Disassembly of section') or 'file format' in line:
            cur = None
            continue
        if cur is None or not line.strip():
            continue
        line = re.sub(r'^\s*[0-9a-f]+:\s*', '', line)
        line = re.sub(r'\b[0-9a-f]+ <[^>]*>', '<J>', line)                      # branch/call targets
        line = re.sub(r'\.LC\d+|\.rodata[\w.]*(\+0x[0-9a-f]+)?', '<STR>', line)  # string literals
        line = re.sub(r'(R_X86_64_\w+\s+)([\w.]+)\.(isra|constprop|part|cold)\.\d+', r'\1\2', line)
        # static vs extern linkage only changes how a call is relocated and whether endbr64 is emitted
        if line.strip() == 'endbr64':
            continue
        # alignment padding varies with the size of neighbouring code
        if re.match(r'^(data16 |cs )*(nop[wl]?|xchg\s+%ax,%ax)\b', line.strip()):
            continue
        # offsets into a file's own data sections depend on what else lives in that file
        line = re.sub(r'(R_X86_64_\w+)\s+\.(data|bss|rodata)[\w.]*([+-]0x[0-9a-f]+)?', r'\1 <DATA>', line)
        line = re.sub(r'R_X86_64_(PLT32|PC32)\s+(\.text\.)?([\w.]+)-0x4', r'R_CALL \3', line)
        fns[cur][-1].append(line)
    return {k: sorted('\n'.join(b) for b in v) for k, v in fns.items()}

ref, new = functions(sys.argv[1]), functions(sys.argv[2])
if RENAME:
    sub = lambda t: re.sub(r'\b(' + '|'.join(map(re.escape, RENAME)) + r')\b', lambda m: RENAME[m.group(1)], t)
    ref = {RENAME.get(k, k): [sub(b) for b in v] for k, v in ref.items()}
gone = sorted(set(ref) - set(new)); added = sorted(set(new) - set(ref))
changed = sorted(k for k in set(ref) & set(new) if ref[k] != new[k])
for label, names in (('changed', changed), ('removed', gone), ('added', added)):
    if names:
        print(f'{label} ({len(names)}): ' + ' '.join(names))
print(f'objdiff: {len(ref)} ref functions, {len(new)} new; {len(changed)} changed, {len(gone)} removed, {len(added)} added')
sys.exit(1 if changed or gone or added else 0)
