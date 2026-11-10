#!/usr/bin/env python3
"""printf_to_format.py: turns printf format strings into std::format ones, and calls of the *_printf() output
functions into calls of their std::format replacements.

  printf_to_format.py --selftest           checks the conversion against snprintf() for every conversion spec the
                                           code base uses (compiles and runs a C++ program)
  printf_to_format.py --specs              lists those specs and what each becomes
  printf_to_format.py FUNC:N[,FUNC:N...] FILE...
                                           rewrites calls of FUNC_printf(...) in FILE to FUNC(...), converting the
                                           format string, which is argument N (from 0); prints each call it
                                           couldn't convert, to do by hand

Conversion (printf -> std::format):
  %s %d %i %c %f %e %g %x %X %o %u %%, with flags - + space # 0, a width and a precision, and the length
  modifiers hh h l ll z j t L and <cinttypes>' PRId64-style macros.
  printf right-aligns strings and std::format left-aligns them, so a width always gets an explicit < or >.
  %d becomes {:d}, so a char argument prints as a number as it did; %c becomes {:c}.
  %u %x %X %o wrap their argument in as_unsigned(), as printf reads it unsigned; %c wraps it in as_char(), as
  printf prints any int as the byte it ends in (std::format throws on one out of char's range).
  A * width or precision comes after its value in std::format, so the two arguments swap.
  With no arguments a call is plain text: the replacements' std::string_view overload prints it as it is, so
  only %% is undone and braces are left alone.
Calls it leaves alone (and prints): a format that isn't a string literal, %n, %p, an integer precision
(%.3d), a * width whose arguments it can't find.
"""
import os, re, subprocess, sys, tempfile

SPEC = re.compile(r'%([-+ #0]*)(\*|\d+)?(?:\.(\*|\d*))?(hh|h|ll|l|z|j|t|L)?([diouxXeEfFgGcsp%n])')


class Unsupported(Exception):
    pass


def convert_spec(flags, width, prec, conv):
    """One printf conversion -> (std::format field, notes), notes being a list of
    'unsigned' (wrap the value in as_unsigned()), 'width' / 'prec' (a * argument comes before the value)."""
    notes = []
    if conv in 'np':
        raise Unsupported('%' + conv)
    spec = ''
    left = '-' in flags
    if width is not None:
        if left:
            spec += '<'
        elif '0' in flags and conv not in 'sc':
            pass  # zero padding: std::format puts the zeros after the sign, as printf does
        else:
            spec += '>'
    if '+' in flags and conv in 'dieEfFgG':
        spec += '+'
    elif ' ' in flags and conv in 'dieEfFgG':
        spec += ' '
    if '#' in flags and conv in 'xXoeEfFgG':
        spec += '#'
    if width is not None and '0' in flags and not left and conv not in 'sc':
        spec += '0'
    if width == '*':
        spec += '{}'
        notes.append('width')
    elif width is not None:
        spec += width
    if prec is not None:
        if conv in 'diouxXc':
            raise Unsupported('integer precision')
        if prec == '*':
            spec += '.{}'
            notes.append('prec')
        else:
            spec += '.' + (prec or '0')
    if conv in 'di':
        spec += 'd'
    elif conv in 'uxXo':
        spec += {'u': 'd', 'x': 'x', 'X': 'X', 'o': 'o'}[conv]
        notes.append('unsigned')
    elif conv == 'c':
        spec += 'c'
        notes.append('char')
    elif conv in 'eEfFgG':
        spec += conv
    elif conv == 's':
        pass
    return '{' + (':' + spec if spec else '') + '}', notes


def convert_format(fmt, with_args=True):
    """A printf format (the C string's contents, escapes left as written) -> (std::format string, notes per
    value consumed, in printf's order). Without arguments the text is printed as it is."""
    out, notes, i = [], [], 0
    for m in SPEC.finditer(fmt):
        text = fmt[i:m.start()]
        out.append(text.replace('{', '{{').replace('}', '}}') if with_args else text)
        i = m.end()
        flags, width, prec, length, conv = m.groups()
        if conv == '%':
            out.append('%')
            continue
        if not with_args:
            raise Unsupported('a conversion with no argument')
        field, n = convert_spec(flags, width, prec, conv)
        out.append(field)
        notes.append(n)
    text = fmt[i:]
    out.append(text.replace('{', '{{').replace('}', '}}') if with_args else text)
    return ''.join(out), notes


# ---------------------------------------------------------------------------------------------- source rewriting

def skip_string(src, i):
    """src[i] == '"': index after the closing quote."""
    i += 1
    while src[i] != '"':
        i += 2 if src[i] == '\\' else 1
    return i + 1


def split_args(src, i):
    """src[i] == '(': (list of (start, end) spans of the top-level arguments, index of the closing ')')."""
    depth, a, spans, j = 0, i + 1, [], i
    while True:
        c = src[j]
        if c == '"':
            j = skip_string(src, j)
            continue
        if c == "'":
            j += 1
            while src[j] != "'":
                j += 2 if src[j] == '\\' else 1
            j += 1
            continue
        if src.startswith('//', j):
            j = src.index('\n', j)
            continue
        if src.startswith('/*', j):
            j = src.index('*/', j) + 2
            continue
        if c in '([{':
            depth += 1
        elif c in ')]}':
            depth -= 1
            if depth == 0:
                spans.append((a, j))
                return spans, j
        elif c == ',' and depth == 1:
            spans.append((a, j))
            a = j + 1
        j += 1


PRI = re.compile(r'\bPRI([diouxX])(8|16|32|64|MAX|PTR|LEAST\d+|FAST\d+)\b')
LITERAL_PIECE = re.compile(r'\s*("(?:[^"\\]|\\.)*"|PRI[diouxX]\w*)\s*', re.S)


def literal_value(text):
    """The source of a format argument -> its contents (adjacent literals joined, PRId64 and the like read as
    the conversion letter they stand for), or None if it isn't a literal."""
    pos, parts = 0, []
    text = text.strip()
    if not text:
        return None
    while pos < len(text):
        m = LITERAL_PIECE.match(text, pos)
        if not m:
            return None
        piece = m.group(1)
        if piece.startswith('"'):
            parts.append(piece[1:-1])
        else:
            pm = PRI.fullmatch(piece)
            if not pm:
                return None
            parts.append('l' + pm.group(1))  # "%" PRId64 -> %ld: the same value, read as a 64-bit integer
        pos = m.end()
    return ''.join(parts)


def comment_spans(src):
    """(start, end) of each comment in src, strings and character literals passed over."""
    spans, i = [], 0
    while i < len(src):
        c = src[i]
        if c == '"':
            i = skip_string(src, i)
        elif c == "'":
            i += 1
            while i < len(src) and src[i] != "'":
                i += 2 if src[i] == '\\' else 1
            i += 1
        elif src.startswith('//', i):
            j = src.find('\n', i)
            j = len(src) if j < 0 else j
            spans.append((i, j))
            i = j
        elif src.startswith('/*', i):
            j = src.find('*/', i + 2)
            j = len(src) if j < 0 else j + 2
            spans.append((i, j))
            i = j
        else:
            i += 1
    return spans


def rewrite_file(path, funcs, fmt_index):
    """fmt_index: {func: index of its format argument}. Returns list of (line, reason) left alone."""
    src = open(path).read()
    comments = comment_spans(src)
    out, i, left = [], 0, []
    call = re.compile(r'\b(' + '|'.join(re.escape(f) for f in funcs) + r')_printf\s*\(')
    while True:
        m = call.search(src, i)
        if not m:
            out.append(src[i:])
            break
        func = m.group(1)
        if any(a <= m.start() < b for a, b in comments):  # code in a comment: only its old name back
            out.append(src[i:m.start()] + func + '(')
            i = m.end()
            continue
        paren = m.end() - 1
        spans, close = split_args(src, paren)
        fi = fmt_index[func]
        line = src.count('\n', 0, m.start()) + 1
        try:
            if fi >= len(spans):
                raise Unsupported('too few arguments')
            fmt_src = src[spans[fi][0]:spans[fi][1]]
            fmt = literal_value(fmt_src)
            if fmt is None:
                raise Unsupported('format not a literal')
            values = spans[fi + 1:]
            new_fmt, notes = convert_format(fmt, with_args=bool(values))
            consumed = sum(1 + ('width' in n) + ('prec' in n) for n in notes)
            if consumed != len(values):
                raise Unsupported(f'{consumed} conversions for {len(values)} arguments')
            arg_texts = [src[a:b] for a, b in values]
            new_args, k = [], 0
            for n in notes:
                extra = []
                if 'width' in n:
                    extra.append(arg_texts[k])
                    k += 1
                if 'prec' in n:
                    extra.append(arg_texts[k])
                    k += 1
                value = arg_texts[k]
                k += 1
                if 'unsigned' in n or 'char' in n:
                    lead = value[:len(value) - len(value.lstrip())]
                    value = f'{lead}{"as_unsigned" if "unsigned" in n else "as_char"}({value.strip()})'
                new_args.append(value)
                new_args.extend(extra)  # std::format takes the value before its width and precision
            lead = fmt_src[:len(fmt_src) - len(fmt_src.lstrip())]
            new_fmt_src = lead + '"' + new_fmt + '"'
            head = src[m.start():spans[fi][0]].replace(func + '_printf', func, 1)
            pieces = [head, new_fmt_src]
            for a in new_args:
                pieces.append(',' + a)
            out.append(src[i:m.start()])
            out.append(''.join(pieces))
            out.append(src[close:close + 1])
            i = close + 1
        except Unsupported as e:
            left.append((line, str(e)))
            out.append(src[i:m.end()])
            i = m.end()
    new = ''.join(out)
    if new != src:
        open(path, 'w').write(new)
    return left


# ---------------------------------------------------------------------------------------------- self-test

def codebase_specs(root):
    specs = set()
    lit = re.compile(r'"(?:[^"\\]|\\.)*"')
    for d, _, files in os.walk(root):
        if '/build' in d:
            continue
        for f in files:
            if f.endswith(('.cpp', '.hpp')):
                for s in lit.findall(open(os.path.join(d, f), errors='replace').read()):
                    for m in SPEC.finditer(s):
                        specs.add(m.group(0))
    return sorted(specs)


SAMPLES = {
    'd': ['0', '7', '-7', '42', '-12345', '2147483647', '(-2147483647 - 1)'],
    'i': ['0', '-3', '99'],
    'u': ['0u', '7u', '4294967295u'],
    'x': ['0u', '255u', '3735928559u'], 'X': ['0u', '255u', '3735928559u'], 'o': ['0u', '8u', '511u'],
    'c': ["'a'", "'Z'", "' '", "200", "-56", "toupper('q')"],
    's': ['""', '"x"', '"hello"', '"a long string of text"'],
    'f': ['0.0', '1.5', '-2.25', '1234.5678', '0.000123'], 'F': ['0.0', '1.5'],
    'e': ['0.0', '1.5', '-123456.789'], 'E': ['1.5'],
    'g': ['0.0', '1.5', '100000.0', '1234567.0', '0.0001', '0.00001'], 'G': ['1e-10'],
}
TYPE = {'hh': {'d': 'signed char', 'i': 'signed char', 'u': 'unsigned char', 'x': 'unsigned char'},
        'h': {'d': 'short', 'u': 'unsigned short'},
        'l': {'d': 'long', 'i': 'long', 'u': 'unsigned long', 'x': 'unsigned long', 'X': 'unsigned long', 'o': 'unsigned long'},
        'll': {'d': 'long long', 'i': 'long long', 'u': 'unsigned long long', 'x': 'unsigned long long'},
        'z': {'d': 'long', 'u': 'size_t', 'x': 'size_t'}, 'j': {'d': 'intmax_t', 'u': 'uintmax_t'}, 't': {'d': 'ptrdiff_t'}}


def selftest(root):
    lines = ['#include <cstdio>', '#include <cctype>', '#include <cstdint>', '#include <cstddef>', '#include <format>', '#include <string>',
             '#include <type_traits>',
             'template <class T> auto as_unsigned(T v) { return static_cast<std::make_unsigned_t<T>>(v); }',
             'template <class T> char as_char(T v) { return static_cast<char>(v); }',
             'static int bad = 0;',
             'template <class... A> static void same(const char *pf, const char *sf, const std::string &got, const char *want) {',
             '  if (got != want) { std::printf("MISMATCH %s -> %s: printf \\"%s\\", std::format \\"%s\\"\\n", pf, sf, want, got.c_str()); bad++; }',
             '}', 'int main() {', '  static char buf[1 << 16];']
    n = 0
    for spec in codebase_specs(root):
        m = SPEC.fullmatch(spec)
        flags, width, prec, length, conv = m.groups()
        if conv in '%np':
            continue
        try:
            field, notes = convert_spec(flags, width, prec, conv)
        except Unsupported:
            continue
        ctype = TYPE.get(length or '', {}).get(conv)
        if conv == 'L':
            continue
        for v in SAMPLES.get(conv, []):
            if length == 'L':
                v = f'(long double){v}'
            elif ctype:
                v = f'({ctype})({v})'
            pf_args, sf_args = [], []
            if width == '*':
                pf_args.append('7')
            if prec == '*':
                pf_args.append('3')
            pf_args.append(v)
            val = f'as_unsigned({v})' if 'unsigned' in notes else f'as_char({v})' if 'char' in notes else v
            sf_args.append(val)
            if width == '*':
                sf_args.append('7')
            if prec == '*':
                sf_args.append('3')
            fmt = '[' + spec + ']'
            sfmt = '[' + field + ']'
            lines.append(f'  std::snprintf(buf, sizeof(buf), "{fmt}", {", ".join(pf_args)});')
            lines.append(f'  same("{fmt}", "{sfmt}", std::format("{sfmt}", {", ".join(sf_args)}), buf);')
            n += 1
    lines += ['  std::printf("%d comparisons, %d mismatches\\n", ' + str(n) + ', bad);', '  return bad != 0;', '}']
    with tempfile.TemporaryDirectory() as tmp:
        cpp, exe = os.path.join(tmp, 'check.cpp'), os.path.join(tmp, 'check')
        open(cpp, 'w').write('\n'.join(lines) + '\n')
        r = subprocess.run(['g++', '-std=c++26', '-Wno-format', '-o', exe, cpp], capture_output=True, text=True)
        if r.returncode:
            print(r.stderr[:4000])
            return 2
        return subprocess.run([exe]).returncode


def main():
    root = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'libs')
    if sys.argv[1:] == ['--selftest']:
        sys.exit(selftest(root))
    if sys.argv[1:] == ['--specs']:
        for s in codebase_specs(root):
            m = SPEC.fullmatch(s)
            try:
                f = convert_spec(*m.group(1, 2, 3, 5))[0] if m.group(5) not in '%' else '%'
            except Unsupported as e:
                f = f'(by hand: {e})'
            print(f'{s:12} {f}')
        return
    funcs = sys.argv[1].split(',')
    fmt_index = {}
    for spec in funcs:
        name, _, idx = spec.partition(':')
        fmt_index[name] = int(idx)
    status = 0
    for path in sys.argv[2:]:
        for line, why in rewrite_file(path, list(fmt_index), fmt_index):
            print(f'{path}:{line}: left as it was: {why}')
            status = 1
    sys.exit(status)


if __name__ == '__main__':
    main()
