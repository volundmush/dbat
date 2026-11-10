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
  printf_to_format.py --buffers FILE...    makes each local char buffer in FILE a std::string where every use is
                                           one it knows: written by snprintf, sprintf, strcpy, strcat or
                                           sprintbit() and the like (as statements), cleared, read with *, [0]
                                           or strlen(), changed by a function with a std::string & overload
                                           (CAP(), search_replace()...), or passed to a function (as .c_str()
                                           unless it takes a std::string_view); then tidies format calls (a
                                           value s.c_str() becomes s); prints the buffers it left alone

Conversion (printf -> std::format):
  %s %d %i %c %f %e %g %x %X %o %u %%, with flags - + space # 0, a width and a precision, and the length
  modifiers hh h l ll z j t L and <cinttypes>' PRId64-style macros.
  printf right-aligns strings and std::format left-aligns them, so a width always gets an explicit < or >.
  %d becomes {:d}, so a char argument prints as a number as it did; %c becomes {:c}.
  %u %x %X %o wrap their argument in as_unsigned(), as printf reads it unsigned; %c wraps it in as_char(), as
  printf prints any int as the byte it ends in (std::format throws on one out of char's range).
  A * width or precision (%*d, %-*s, %.*s) becomes printf_width() or printf_precision() around the value:
  std::format's own run-time widths can't be checked when compiled by clang with libstdc++.
  With no arguments a call is plain text: the replacements' std::string_view overload prints it as it is, so
  only %% is undone and braces are left alone.
  %p wraps its argument in as_pointer(), which prints a null pointer as "(nil)" as glibc's printf does.
  An integer precision (%.3d, %-3.3d) wraps its argument in printf_digits(), which gives at least that many
  digits after the sign as printf does.
Calls it leaves alone (and prints): a format that isn't a string literal, %n, an integer precision with flags
other than - and 0, a * width whose arguments it can't find.
"""
import os, re, subprocess, sys, tempfile

SPEC = re.compile(r'%([-+ #0]*)(\*|\d+)?(?:\.(\*|\d*))?(hh|h|ll|l|z|j|t|L)?([diouxXeEfFgGcsp%n])')


class Unsupported(Exception):
    pass


def convert_spec(flags, width, prec, conv):
    """One printf conversion -> (std::format field, notes), notes being a list of
    'unsigned' (wrap the value in as_unsigned()), 'width' / 'prec' (a * argument comes before the value)."""
    notes = []
    if conv == 'n':
        raise Unsupported('%n')
    if conv == 'p':
        if flags or width is not None or prec is not None:
            raise Unsupported('%p with a width or flags')
        notes.append('pointer')
        return '{}', notes
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
        if prec is not None or flags.strip('-') or conv not in 'sdi':
            raise Unsupported('a * width with more than - on %' + conv)
        notes.append('width-left' if left else 'width')
        return '{}', notes
    elif width is not None:
        spec += width
    if prec is not None and conv in 'diu' and prec not in ('*', '') and width != '*' and not set(flags) - set('-0'):
        # %.3d: at least that many digits (printf_digits()), padded to the width as text; 0 is ignored with it
        notes.append(f'digits:{prec}')
        if conv == 'u':
            notes.append('unsigned')
        return '{' + ((':' + ('<' if left else '>') + width) if width else '') + '}', notes
    if prec is not None:
        if conv in 'diouxXc':
            raise Unsupported('integer precision')
        if prec == '*':
            if width is not None or flags or conv != 's':
                raise Unsupported('a * precision with more than %.*s')
            notes.append('prec')
            return '{}', notes
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


def convert_call(src, spans, fi):
    """The arguments of a printf-style call (spans from split_args(), the format argument at fi) ->
    (the std::format string's source, the source of each value after it, converted). Raises Unsupported."""
    if fi >= len(spans):
        raise Unsupported('too few arguments')
    fmt_src = src[spans[fi][0]:spans[fi][1]]
    fmt = literal_value(fmt_src)
    if fmt is None:
        raise Unsupported('format not a literal')
    values = spans[fi + 1:]
    new_fmt, notes = convert_format(fmt, with_args=bool(values))
    consumed = sum(1 + any(x.startswith('width') or x == 'prec' for x in n) for n in notes)
    if consumed != len(values):
        raise Unsupported(f'{consumed} conversions for {len(values)} arguments')
    arg_texts = [src[a:b] for a, b in values]
    new_args, k = [], 0
    for n in notes:
        runtime = None
        if any(x.startswith('width') or x == 'prec' for x in n):
            runtime = arg_texts[k].strip()  # printf takes a * width or precision before its value
            k += 1
        value = arg_texts[k]
        k += 1
        lead = value[:len(value) - len(value.lstrip())]
        digits = next((x[7:] for x in n if x.startswith('digits:')), None)
        if digits is not None:
            inner = f'as_unsigned({value.strip()})' if 'unsigned' in n else value.strip()
            value = f'{lead}printf_digits({inner}, {digits})'
        elif 'pointer' in n:
            value = f'{lead}as_pointer({value.strip()})'
        elif 'unsigned' in n or 'char' in n:
            value = f'{lead}{"as_unsigned" if "unsigned" in n else "as_char"}({value.strip()})'
        elif 'width' in n or 'width-left' in n:
            value = f'{lead}printf_width({value.strip()}, {runtime}, {"true" if "width-left" in n else "false"})'
        elif 'prec' in n:
            value = f'{lead}printf_precision({value.strip()}, {runtime})'
        new_args.append(value)
    lead = fmt_src[:len(fmt_src) - len(fmt_src.lstrip())]
    new_fmt_src = lead + '"' + new_fmt + '"'
    return new_fmt_src, new_args


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
            new_fmt_src, new_args = convert_call(src, spans, fi)
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


# ---------------------------------------------------------------------------------------------- buffers

def noncode(src):
    """A bytearray, 1 at each position inside a string, a character literal or a comment."""
    mask = bytearray(len(src))
    i = 0
    while i < len(src):
        c = src[i]
        if c == '"' or c == "'":
            j = i + 1
            while j < len(src) and src[j] != c:
                j += 2 if src[j] == '\\' else 1
            j += 1
        elif src.startswith('//', i):
            j = src.find('\n', i)
            j = len(src) if j < 0 else j
        elif src.startswith('/*', i):
            j = src.find('*/', i + 2)
            j = len(src) if j < 0 else j + 2
        else:
            i += 1
            continue
        mask[i:j] = b'\x01' * (j - i)
        i = j
    return mask


DECL_LINE = re.compile(r'^([ \t]+)char[ \t]+([^;()]*);([ \t]*(?:/\*.*\*/|//.*))?[ \t]*$', re.M)
DECLARATOR = re.compile(r'\s*([A-Za-z_]\w*)\s*\[([^\]]+)\]\s*(?:=\s*(?:""|\{\s*(?:0|\'\\0\'|"")?\s*\}))?\s*')
OTHER_DECLARATOR = re.compile(r'\s*\**\s*[A-Za-z_]\w*\s*(?:=\s*[^,{}"]+)?\s*')
WRITERS = {'snprintf': 2, 'sprintf': 1, 'strcpy': None, 'strlcpy': None, 'strcat': None, 'strlcat': None}
# functions that take a std::string_view, to which a std::string is passed as it is
SV_SINKS = {'send_to_char', 'write_to_output', 'send_to_room', 'send_to_all', 'send_to_outdoor', 'send_to_moon',
            'send_to_planet', 'send_to_range', 'send_to_imm', 'mudlog', 'mud_log', 'log_imm_action', 'script_log',
            'str_format'}
# functions that write into a char * argument
MUTATORS = {'one_argument', 'two_arguments', 'three_arguments', 'half_chop', 'any_one_arg', 'one_word',
            'skip_spaces', 'trim', 'get_number',
            'fgets', 'get_line', 'strncpy', 'memcpy', 'memmove', 'memset', 'strtok', 'strsep', 'sscanf', 'strftime',
            'fread', 'bzero', 'strncat', 'vsnprintf'}
# functions that change a char * in place and have an overload that changes a std::string & instead
STRING_REF = {'CAP', 'prune_crlf', 'delete_doubledollar', 'search_replace', 'strip_cr'}
# functions that fill a char buffer (the argument at the first index, its size at the second) and have a
# version that returns the text as a std::string instead
PRODUCERS = {'sprintbit': (2, 3), 'sprinttype': (2, 3), 'sprintbitarray': (3, 4)}
KEYWORDS = {'if', 'while', 'for', 'switch', 'return', 'sizeof', 'do'}


def enclosing_call(src, mask, p):
    """The name of the function whose argument list holds position p, and the index of its '(' (or None)."""
    depth, j = 0, p - 1
    while j >= 0:
        if not mask[j]:
            c = src[j]
            if c in ')]':
                depth += 1
            elif c in '([':
                if depth == 0:
                    if c == '[':
                        return None, None
                    m = re.search(r'([A-Za-z_][\w:]*)\s*$', src[max(0, j - 80):j])
                    return (m.group(1) if m else None), j
                depth -= 1
            elif c in '{};' and depth == 0:
                return None, None
        j -= 1
    return None, None


def statement_start(src, mask, p):
    """Whether position p starts a statement (only blanks between it and a ; { } ) else or case label)."""
    j = p - 1
    while j >= 0 and src[j] in ' \t\n':
        j -= 1
    if j < 0 or mask[j]:
        return mask[j] if j >= 0 else True  # after a comment
    if src[j] in ';{})':
        return True
    if src[j] == ':' and src[j - 1] != ':':
        return True
    return src[max(0, j - 3):j + 1] == 'else' and (j < 4 or not (src[j - 4].isalnum() or src[j - 4] == '_'))


def block_bounds(src, mask, a, b):
    """(index of the '{' opening the block that holds src[a:b], index of its '}')."""
    depth, j = 0, a - 1
    while j >= 0:
        if not mask[j]:
            if src[j] == '}':
                depth += 1
            elif src[j] == '{':
                if depth == 0:
                    break
                depth -= 1
        j -= 1
    open_ = j
    depth, j = 0, b
    while j < len(src):
        if not mask[j]:
            if src[j] == '{':
                depth += 1
            elif src[j] == '}':
                if depth == 0:
                    break
                depth -= 1
        j += 1
    return open_, j


def size_ok(text, name, size):
    t = re.sub(r'\s+', '', text)
    return t in (f'sizeof({name})', f'sizeof{name}', re.sub(r'\s+', '', size))


def strip_c_str(v):
    v2 = v.strip()
    return v2[:-len('.c_str()')] if v2.endswith('.c_str()') else v2


def buffer_edits(src, mask, name, size, scope_a, scope_b):
    """The edits [(start, end, text)] that make the char buffer name a std::string, or raises Unsupported."""
    edits, writes, counters = [], 0, set()
    for m in re.finditer(r'\b' + re.escape(name) + r'\b', src[scope_a:scope_b]):
        p = scope_a + m.start()
        e = p + len(name)
        if mask[p] or any(a <= p < b for a, b, _ in edits):
            continue
        before = src[max(0, p - 120):p]
        after = src[e:e + 120]
        line = src.count('\n', 0, p) + 1
        if re.search(r'(\.|->|::)\s*$', before):
            continue  # a member of the same name
        if re.search(r'\b(char|int|bool|long|short|struct|unsigned|void|float|double)\b[\s*&]*$', before):
            raise Unsupported(f'line {line}: declared again')
        func, paren = enclosing_call(src, mask, p)
        # a write: snprintf(name, sizeof(name), ...), sprintf(name, ...), strcpy(name, x), strcat(...)
        if func in WRITERS and re.search(r'\(\s*$', before):
            spans, close = split_args(src, paren)
            first = re.sub(r'\s+', '', src[spans[0][0]:spans[0][1]])
            call_start = paren - len(re.search(r'([A-Za-z_]\w*)\s*$', src[:paren]).group(0))
            # a length kept beside it: len = snprintf(name, ...); len += snprintf(name + len, sizeof(name) - len, ...)
            counted = re.search(r'\b([A-Za-z_]\w*)\s*(\+?=)\s*$', src[max(0, call_start - 60):call_start])
            mlen = re.fullmatch(re.escape(name) + r'\+([A-Za-z_]\w*)', first)
            if func == 'strlcpy' and counted and counted.group(2) == '=' and first == name:
                # len = strlcpy(name, x, sizeof(name)): the length beside it starts there
                stmt = call_start - len(counted.group(0))
                if not size_ok(src[spans[2][0]:spans[2][1]], name, size) or not statement_start(src, mask, stmt) \
                        or not re.match(r'\s*;', src[close + 1:]):
                    raise Unsupported(f'line {line}: the result of {func} is used')
                counters.add(counted.group(1))
                edits.append((stmt, close + 1, f'{name} = {strip_c_str(src[spans[1][0]:spans[1][1]])}'))
                writes += 1
                continue
            if func == 'snprintf' and (mlen or (counted and first == name)):
                counter = mlen.group(1) if mlen else counted.group(1)
                if counted and counted.group(1) != counter:
                    raise Unsupported(f'line {line}: the result of {func} is used')
                if mlen and counted and counted.group(2) != '+=':
                    raise Unsupported(f'line {line}: the result of {func} is used')
                size_src = re.sub(r'\s+', '', src[spans[1][0]:spans[1][1]])
                if mlen and size_src not in (f'sizeof({name})-{counter}', f'{re.sub(r"[ ]", "", size)}-{counter}'):
                    raise Unsupported(f'line {line}: {func} with size {size_src}')
                if not mlen and not size_ok(size_src, name, size):
                    raise Unsupported(f'line {line}: {func} with size {size_src}')
                stmt = call_start - len(counted.group(0)) if counted else call_start
                if not statement_start(src, mask, stmt) or not re.match(r'\s*;', src[close + 1:]):
                    raise Unsupported(f'line {line}: the result of {func} is used')
                counters.add(counter)
                new_fmt_src, new_args = convert_call(src, spans, 2)
                expr = new_fmt_src.strip() if not new_args else \
                    'str_format(' + new_fmt_src.strip() + ',' + ','.join(new_args) + ')'
                edits.append((stmt, close + 1, f'{name} {"+=" if mlen else "="} {expr}'))
                writes += 1
                continue
            if first == name:
                op = '='
            elif first == f'{name}+strlen({name})':
                op = '+='
            else:
                raise Unsupported(f'line {line}: writes into {first}')
            if not statement_start(src, mask, call_start) or not re.match(r'\s*;', src[close + 1:]):
                raise Unsupported(f'line {line}: the result of {func} is used')
            if func in ('snprintf', 'strlcpy', 'strlcat'):
                if len(spans) < 3:
                    raise Unsupported(f'line {line}: too few arguments')
                size_src = src[spans[1 if func == 'snprintf' else 2][0]:spans[1 if func == 'snprintf' else 2][1]]
                ok = size_ok(size_src, name, size) if op == '=' else \
                    re.sub(r'\s+', '', size_src) in (f'sizeof({name})-strlen({name})', f'{re.sub(r"[ ]", "", size)}-strlen({name})')
                if not ok:
                    raise Unsupported(f'line {line}: {func} with size {size_src.strip()}')
            if func in ('strlcat', 'strcat'):
                if op != '=':
                    raise Unsupported(f'line {line}: odd strcat')
                op = '+='
            if WRITERS[func] is not None:
                new_fmt_src, new_args = convert_call(src, spans, WRITERS[func])
                if not new_args:
                    expr = new_fmt_src.strip()
                elif new_fmt_src.strip() == '"{}"' and len(new_args) == 1 and new_args[0].strip().endswith('.c_str()'):
                    expr = strip_c_str(new_args[0])
                else:
                    expr = 'str_format(' + new_fmt_src.strip() + ',' + ','.join(new_args) + ')'
            else:
                expr = strip_c_str(src[spans[1][0]:spans[1][1]])
            text = f'{name}.clear()' if expr == '""' and op == '=' else f'{name} {op} {expr}'
            edits.append((call_start, close + 1, text))
            writes += 1
            continue
        # sprintbit(v, names, name, sizeof(name)) and the like
        if func in PRODUCERS and re.search(r',\s*$', before):
            spans, close = split_args(src, paren)
            di, si = PRODUCERS[func]
            call_start = paren - len(re.search(r'([A-Za-z_]\w*)\s*$', src[:paren]).group(0))
            if len(spans) != si + 1 or re.sub(r'\s+', '', src[spans[di][0]:spans[di][1]]) != name or \
                    not size_ok(src[spans[si][0]:spans[si][1]], name, size) or \
                    not statement_start(src, mask, call_start) or not re.match(r'\s*;', src[close + 1:]):
                raise Unsupported(f'line {line}: passed to {func}')
            args = ','.join(src[a:b] for a, b in spans[:di])
            edits.append((call_start, close + 1, f'{name} = {func}({args.strip()})'))
            writes += 1
            continue
        # *name = '\0'; name[0] = '\0';
        mstar = re.search(r'\*\s*$', before)
        if mstar and statement_start(src, mask, p - len(mstar.group(0))) and re.match(r"\s*=\s*('\\0'|0)\s*;", after):
            end = e + re.match(r"\s*=\s*('\\0'|0)", after).end()
            edits.append((p - len(mstar.group(0)), end, f'{name}.clear()'))
            continue
        mz = re.match(r"\s*\[\s*0\s*\]\s*=\s*('\\0'|0)(?=\s*;)", after)
        if mz and statement_start(src, mask, p):
            edits.append((p, e + mz.end(), f'{name}.clear()'))
            continue
        # strlen(name)
        if func == 'strlen' and re.search(r'\(\s*$', before) and re.match(r'\s*\)', after):
            edits.append((paren - len(re.search(r'strlen\s*$', src[:paren]).group(0)),
                          e + re.match(r'\s*\)', after).end(), f'{name}.size()'))
            continue
        # *name, read
        if mstar:
            star = p - len(mstar.group(0))
            prev = src[:star].rstrip()
            if prev and (prev[-1].isalnum() or prev[-1] in '_)]'):
                raise Unsupported(f'line {line}: {name} multiplied?')
            if re.match(r'\s*=[^=]', after) or re.match(r'\s*(\+\+|--)', after) or re.search(r'(\+\+|--)\s*$', prev):
                raise Unsupported(f'line {line}: writes *{name}')
            if prev.endswith('!') and not prev.endswith('!='):
                bang = len(prev) - 1
                edits.append((bang, e, f'{name}.empty()'))
            elif (prev.endswith('(') or prev.endswith('&&') or prev.endswith('||')) and re.match(r'\s*(\)|&&|\|\||\?)', after):
                edits.append((star, e, f'!{name}.empty()'))
            else:
                edits.append((star, e, f'{name}[0]'))
            continue
        # name[0], read
        if re.match(r'\s*\[', after):
            if re.match(r'\s*\[\s*0\s*\]', after) and not re.match(r'\s*\[\s*0\s*\]\s*(=[^=]|\+\+|--|\+=|-=)', after):
                continue
            raise Unsupported(f'line {line}: indexes {name}')
        # an argument
        if re.search(r'[(,]\s*$', before) and re.match(r'\s*[,)]', after):
            if func is None or func in KEYWORDS:
                raise Unsupported(f'line {line}: {name} as a value')
            if func in MUTATORS:
                raise Unsupported(f'line {line}: passed to {func}')
            if func in STRING_REF:
                if not re.search(r'\(\s*$', before):
                    edits.append((p, e, f'{name}.c_str()'))  # not the text it changes
            elif func not in SV_SINKS:
                edits.append((p, e, f'{name}.c_str()'))
            else:
                only = re.search(r'"\{\}"\s*,\s*$', before)
                if only and func != 'str_format' and re.match(r'\s*\)', after):
                    edits.append((p - len(only.group(0)), e, name))  # printed as it is
            continue
        # returned: a function returning a std::string takes it, one returning a char * fails to compile
        if re.search(r'\breturn\s*\(?\s*$', before) and re.match(r'\s*\)?\s*;', after):
            continue
        # one side of a ?:
        if (re.search(r'\?\s*$', before) or re.search(r'(?<!:):\s*$', before)) and re.match(r'\s*[:),;]', after):
            edits.append((p, e, f'{name}.c_str()'))
            continue
        raise Unsupported(f'line {line}: {name} used as {src[p - 20:e + 20].strip()!r}')
    if not writes:
        raise Unsupported('never written with snprintf, sprintf, strcpy, strcat or sprintbit')
    for counter in counters:  # the length kept beside it goes, if nothing else reads it
        decl = re.compile(r'\n[ \t]*(?:size_t|int|unsigned|long|unsigned int)\s+' + re.escape(counter) +
                          r'(?:\s*=\s*0)?\s*;[ \t]*(?=\n)')
        first_use = min(a for a, b, t in edits if t.startswith(name))
        dm = None
        for d in decl.finditer(src, 0, first_use):
            dm = d  # the last declaration before it is used
        if not dm or mask[dm.start() + 1]:
            raise Unsupported(f'its length {counter} is not declared on a line of its own')
        _, counter_end = block_bounds(src, mask, dm.start() + 1, dm.end())
        if counter_end < first_use:
            raise Unsupported(f'its length {counter} is declared in a block of its own')
        for m in re.finditer(r'\b' + re.escape(counter) + r'\b', src[dm.end():counter_end]):
            p = dm.end() + m.start()
            if mask[p] or any(a <= p < b for a, b, _ in edits) or re.search(r'(\.|->)\s*$', src[p - 3:p]):
                continue
            line_start = src.rfind('\n', 0, p) + 1
            reset = re.compile(r'[ \t]*' + re.escape(counter) + r'\s*=\s*0\s*;[ \t]*\n').match(src, line_start)
            if reset:  # the next write starts from the beginning again
                indent = reset.group(0)[:len(reset.group(0)) - len(reset.group(0).lstrip())]
                edits.append((reset.start(), reset.end(), f'{indent}{name}.clear();\n'))
                continue
            raise Unsupported(f'line {src.count(chr(10), 0, p) + 1}: its length {counter} is read')
        edits.append((dm.start(), dm.end(), ''))
    return edits


def apply_edits(src, edits):
    for a, b, t in sorted(edits, reverse=True):
        src = src[:a] + t + src[b:]
    return src


FORMAT_CALLS = SV_SINKS


def tidy_format_values(src):
    """In calls that format their values with std::format: a value x.c_str() or std::string(x).c_str() becomes x
    (std::format prints a string as it is), and f(..., "{}", s.c_str()) becomes f(..., s) where f prints text as it
    is."""
    mask = noncode(src)
    call = re.compile(r'\b(' + '|'.join(FORMAT_CALLS) + r')\s*\(')
    edits = []
    for m in call.finditer(src):
        if mask[m.start()] or re.search(r'(\.|->|::)\s*$', src[max(0, m.start() - 3):m.start()]):
            continue
        spans, close = split_args(src, m.end() - 1)
        fi = next((k for k, (a, b) in enumerate(spans) if literal_value(src[a:b]) is not None), None)
        if fi is None:
            continue
        values = spans[fi + 1:]
        for a, b in values:
            v = src[a:b]
            w = re.fullmatch(r'(\s*)std::string\((\w+)\)\.c_str\(\)\s*', v) or re.fullmatch(r'(\s*)(.*?)\.c_str\(\)\s*', v, re.S)
            if w and '?' not in w.group(2) and not re.search(r'\s[-+*/<>&|]', w.group(2)):
                edits.append((a, b, w.group(1) + w.group(2)))
        if m.group(1) != 'str_format' and len(values) == 1 and literal_value(src[spans[fi][0]:spans[fi][1]]) == '{}':
            v = src[values[0][0]:values[0][1]]
            w = re.fullmatch(r'\s*(?:std::string\((\w+)\)|(.*?))\.c_str\(\)\s*', v, re.S)
            if w and '?' not in v:
                lead = src[spans[fi][0]:spans[fi][1]]
                lead = lead[:len(lead) - len(lead.lstrip())]
                edits = [x for x in edits if x[0] != values[0][0]]
                edits.append((spans[fi][0], values[0][1], lead + (w.group(1) or w.group(2))))
    src = apply_edits(src, edits)
    return re.sub(r'(\n[ \t]*(\w+)\.clear\(\);)(\n[ \t]*\2\.clear\(\);)+', r'\1', src)  # cleared twice


def merge_first_value(src, decl_end, name):
    """src[:decl_end] ends with 'std::string name;'. If the next statement is name = x; or name.clear();, the
    declaration takes it over."""
    m = re.compile(r'\s*' + re.escape(name) + r'( = |\.clear\(\);)').match(src, decl_end)
    if not m or not src[:decl_end].endswith(';'):  # one with a comment after it stays as it is
        return src
    if m.group(1) != ' = ':
        return src[:decl_end] + src[m.end():]  # a new string is empty already
    mask = noncode(src)
    j = m.end()
    while src[j] != ';' or mask[j]:
        j += 1
    value = src[m.end():j]
    if re.search(r'\b' + re.escape(name) + r'\b', value):
        return src
    return src[:decl_end - 1] + ' = ' + value + ';' + src[j + 1:]


def normalize_counted(src):
    """Rewrites the ways a length beside a buffer is kept into the one buffer_edits() knows,
    len += snprintf(buf + len, sizeof(buf) - len, ...):
      nlen = snprintf(buf + len, sizeof(buf) - len, ...); if (len + nlen >= sizeof(buf) || nlen < 0) break; len += nlen;
      nlen = snprintf(buf + len, sizeof(buf) - len, ...); len += nlen;
    (a string has no end to stop at), and drops the **OVERFLOW** marker written over a full buffer's end."""
    mask = noncode(src)
    call = re.compile(r'\b(\w+) = snprintf\s*\(\s*(\w+) \+ (\w+), sizeof\(\2\) - \3,')
    edits = []
    for m in call.finditer(src):
        if mask[m.start()]:
            continue
        nlen, buf, ln = m.groups()
        _, close = split_args(src, src.index('(', m.start() + len(nlen) + 3))
        guard = re.compile(r';\s*(?:if \(' + ln + r' \+ ' + nlen + r' >= sizeof\(' + buf + r'\) \|\| ' + nlen +
                           r' < 0\)\s*break;\s*)?' + ln + r' \+= ' + nlen + r';').match(src, close + 1)
        if guard:
            edits.append((m.start(), m.start() + len(nlen), ln))
            edits.append((m.start() + len(nlen), m.start() + len(nlen) + 3, ' += '))
            edits.append((close + 1, guard.end(), ';'))
    src = apply_edits(src, edits)
    src = re.sub(r'\n[ \t]*if \((\w+) >= sizeof\((\w+)\)\)\s*strcpy\(\2 \+ sizeof\(\2\) - strlen\(overflow\) - 1, overflow\);[^\n]*', '', src)
    if 'overflow' not in re.sub(r'\n[ \t]*const char \*overflow\s*=\s*"[^"]*";', '', src).replace('**OVERFLOW**', ''):
        src = re.sub(r'\n[ \t]*const char \*overflow\s*=\s*"[^"]*";', '', src)
    return src


def convert_buffers(path):
    """Makes each local char buffer in path that is written only by snprintf/sprintf/strcpy/strcat and read in ways
    a std::string can be a std::string. Returns [(line, name, reason)] for those left alone."""
    src = normalize_counted(open(path).read())
    tried, left = set(), []
    while True:
        mask = noncode(src)
        found = None
        for dm in DECL_LINE.finditer(src):
            if mask[dm.start(2)]:
                continue
            parts = [x for x in re.split(r',', dm.group(2))]
            decls = [DECLARATOR.fullmatch(x) for x in parts]
            # the others on the line (char *p, char c = 'x') stay as they are
            if not all(d or OTHER_DECLARATOR.fullmatch(x) for d, x in zip(decls, parts)) or not any(decls):
                continue
            for k, d in enumerate(decls):
                if not d:
                    continue
                key = (d.group(1), dm.group(0), src.count(dm.group(0), dm.end()))  # identical ones after it stay put
                if key not in tried:
                    found = (dm, decls, k, key)
                    break
            if found:
                break
        if not found:
            break
        dm, decls, k, key = found
        parts = [x.strip() for x in re.split(r',', dm.group(2))]
        tried.add(key)
        name, size = decls[k].group(1), decls[k].group(2).strip()
        line = src.count('\n', 0, dm.start()) + 1
        open_, close = block_bounds(src, mask, dm.start(), dm.end())
        head = src[max(0, open_ - 300):open_]
        head = head[max(head.rfind(';'), head.rfind('}')) + 1:]
        if open_ < 0 or re.search(r'\b(struct|class|union|enum)\b(\s+\w+)?\s*(:[^(){};]*)?$', head.strip()):
            left.append((line, name, 'not in a function'))
            continue
        try:
            edits = buffer_edits(src, mask, name, size, dm.end(), close)
        except Unsupported as e:
            left.append((line, name, str(e)))
            continue
        indent = dm.group(1)
        lines = []
        if k:
            lines.append(indent + 'char ' + ', '.join(parts[:k]) + ';')
        lines.append(indent + f'std::string {name};' + (dm.group(3) or ''))
        if k + 1 < len(decls):
            lines.append(indent + 'char ' + ', '.join(parts[k + 1:]) + ';')
        edits.append((dm.start(), dm.end(), '\n'.join(lines)))
        src = apply_edits(src, edits)
        src = merge_first_value(src, dm.start() + len('\n'.join(lines[:-1 if k + 1 < len(decls) else None])), name)
    src = tidy_format_values(src)
    if src != open(path).read():
        open(path, 'w').write(src)
    # report each buffer left alone once, by its final reason
    seen, out = set(), []
    for line, name, why in left:
        if (line, name) not in seen:
            seen.add((line, name))
            out.append((line, name, why))
    return out


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
    'p': ['(void *)nullptr', '(void *)0x1234', '(void *)0x7ffd3a1eae60'],
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
             '#include "dbat/format.hpp"',
             'static int bad = 0;',
             'template <class... A> static void same(const char *pf, const char *sf, const std::string &got, const char *want) {',
             '  if (got != want) { std::printf("MISMATCH %s -> %s: printf \\"%s\\", std::format \\"%s\\"\\n", pf, sf, want, got.c_str()); bad++; }',
             '}', 'int main() {', '  static char buf[1 << 16];']
    n = 0
    for spec in codebase_specs(root):
        m = SPEC.fullmatch(spec)
        flags, width, prec, length, conv = m.groups()
        if conv in '%n':
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
            digits = next((x[7:] for x in notes if x.startswith('digits:')), None)
            if digits is not None:
                val = f'printf_digits({"as_unsigned(" + v + ")" if "unsigned" in notes else v}, {digits})'
            elif 'pointer' in notes:
                val = f'as_pointer({v})'
            elif 'unsigned' in notes:
                val = f'as_unsigned({v})'
            elif 'char' in notes:
                val = f'as_char({v})'
            elif 'width' in notes or 'width-left' in notes:
                val = f'printf_width({v}, 7, {"true" if "width-left" in notes else "false"})'
            elif 'prec' in notes:
                val = f'printf_precision({v}, 3)'
            else:
                val = v
            sf_args.append(val)
            fmt = '[' + spec + ']'
            sfmt = '[' + field + ']'
            lines.append(f'  std::snprintf(buf, sizeof(buf), "{fmt}", {", ".join(pf_args)});')
            lines.append(f'  same("{fmt}", "{sfmt}", std::format("{sfmt}", {", ".join(sf_args)}), buf);')
            n += 1
    lines += ['  std::printf("%d comparisons, %d mismatches\\n", ' + str(n) + ', bad);', '  return bad != 0;', '}']
    with tempfile.TemporaryDirectory() as tmp:
        cpp, exe = os.path.join(tmp, 'check.cpp'), os.path.join(tmp, 'check')
        open(cpp, 'w').write('\n'.join(lines) + '\n')
        inc = os.path.join(root, 'dbat', 'include')
        r = subprocess.run(['g++', '-std=c++26', '-Wno-format', '-I', inc, '-o', exe, cpp], capture_output=True, text=True)
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
    if sys.argv[1] == '--buffers':
        for path in sys.argv[2:]:
            for line, name, why in convert_buffers(path):
                print(f'{path}:{line}: {name} left as it was: {why}')
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
