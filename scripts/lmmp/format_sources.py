#!/usr/bin/env python3
"""LMMP source formatter.

Rules applied to every tracked/staged text file:
  1. (C/C++ only) Align the trailing '\\' of multi-line macros: every
     continuation backslash sits in the column right after the longest
     line of the macro, i.e. the longest line keeps exactly one space
     before its '\\'.  Backslashes that live inside comments are never
     touched, and comment-only lines never widen the alignment column.
     The column is counted in characters; a macro whose lines contain
     characters that do not render one column wide (full-width/CJK text,
     zero-width or combining marks) is flagged with a warning, because
     the aligned backslashes may still look off in editors.
  2. Replace every tab with four spaces (except in makefiles, where
     recipes require hard tabs).
  3. Strip trailing whitespace from every line.
  4. Ensure the file ends with a newline.
  5. Collapse trailing blank lines down to a single blank line (a file
     that ends without blank lines keeps its single final newline).

Line endings are preserved per line (LF or CRLF), the encoding is
round-tripped through utf-8 with surrogate escapes, and binary files
(containing NUL bytes) are skipped.  Files and directories listed in
EXEMPT_PATHS (repository-relative, e.g. 'LICENSE') are never touched.

Usage:
  python scripts/lmmp/format_sources.py                 # format all tracked
                                                        # files
  python scripts/lmmp/format_sources.py FILE [FILE...]  # format given files
  python scripts/lmmp/format_sources.py --staged        # format files staged
                                                        # for commit, re-stage
  python scripts/lmmp/format_sources.py --dry-run ...   # only report what
                                                        # would change
"""

import fnmatch
import os
import subprocess
import sys
import unicodedata

# Rule 1 applies to these extensions only.
MACRO_EXTS = {'.c', '.h', '.cpp', '.hpp', '.cc', '.cxx', '.hh', '.inl'}

# Tabs are part of the syntax in makefiles; leave them alone.
TAB_EXEMPT_NAMES = {'makefile', 'gnumakefile'}
TAB_EXEMPT_EXTS = {'.mk'}

# Bypass list: repository-relative files or directories (always written
# with forward slashes) that are never formatted.  A directory pattern
# covers everything inside it, and glob patterns ('*.md', 'dist/*') work
# as well.  Paths are matched relative to the repository root, e.g.
# 'LICENSE', 'doc/', 'test/fixtures'.
EXEMPT_PATHS = {
    'LICENSE',
}

MAX_FILE_SIZE = 16 * 1024 * 1024


def is_macro_file(path):
    return os.path.splitext(path)[1].lower() in MACRO_EXTS


def is_tab_exempt(path):
    name = os.path.basename(path).lower()
    return (name in TAB_EXEMPT_NAMES
            or os.path.splitext(name)[1] in TAB_EXEMPT_EXTS)


def is_exempt_path(path, root=None):
    """True when ``path`` is bypassed by EXEMPT_PATHS.

    ``path`` is matched as a repository-relative, forward-slash path when
    ``root`` is given and the path lives under it; otherwise as given.
    Entries without glob characters match a file exactly or a directory
    and everything below it; glob entries use fnmatch semantics.
    """
    rel = path
    if root:
        try:
            rel = os.path.relpath(path, root)
        except ValueError:  # different drive (Windows)
            rel = path
    rel = rel.replace(os.sep, '/')
    while rel.startswith('./'):
        rel = rel[2:]
    for pattern in EXEMPT_PATHS:
        pattern = pattern.strip('/')
        if fnmatch.fnmatchcase(rel, pattern):
            return True
        if (not any(c in pattern for c in '*?[')
                and (rel == pattern or rel.startswith(pattern + '/'))):
            return True
    return False


def scan_line(line, in_block):
    """Classify each character of ``line`` as code or comment.

    Returns ``(code, in_block)`` where ``code`` is a bytearray of the same
    length as ``line`` (1 = character outside comments) and ``in_block``
    tells whether a /* ... */ comment is still open at the end of the line.
    Line comments and string/char literals are tracked so that comment
    markers inside strings are not misread as comments.
    """
    n = len(line)
    code = bytearray(n)
    i = 0
    while i < n:
        if in_block:
            if line.startswith('*/', i):
                in_block = False
                i += 2
            else:
                i += 1
            continue
        c = line[i]
        if c == '/' and line.startswith('//', i):
            break  # rest of the line is a comment
        if c == '/' and line.startswith('/*', i):
            in_block = True
            i += 2
            continue
        if c in ('"', "'"):
            quote = c
            code[i] = 1
            i += 1
            while i < n:
                if line[i] == '\\' and i + 1 < n:
                    code[i] = code[i + 1] = 1
                    i += 2
                else:
                    code[i] = 1
                    if line[i] == quote:
                        i += 1
                        break
                    i += 1
            continue
        code[i] = 1
        i += 1
    return code, in_block


def char_columns(ch):
    """Approximate the on-screen width of ``ch`` in a monospace editor.

    Returns 0 for control/format characters (zero-width spaces, BOM, ...)
    and combining marks, 2 for East-Asian wide/fullwidth characters and 1
    otherwise.  Backslash alignment counts characters, so any character
    whose width is not 1 can make an aligned column look misaligned.
    """
    if unicodedata.category(ch)[0] == 'C' or unicodedata.combining(ch):
        return 0
    if unicodedata.east_asian_width(ch) in ('W', 'F'):
        return 2
    return 1


def split_lines(text):
    """Split into (content, eol) pairs, keeping each line's own ending.

    Returns ``(lines, tail)``; ``tail`` is the unterminated trailing chunk
    ('' when the file already ends with a newline).
    """
    parts = text.split('\n')
    tail = parts.pop()
    lines = []
    for part in parts:
        if part.endswith('\r'):
            lines.append((part[:-1], '\r\n'))
        else:
            lines.append((part, '\n'))
    return lines, tail


def align_macro_backslashes(lines):
    """Rule 1: align continuation backslashes of multi-line macros.

    ``lines`` is a list of already tab-expanded, right-stripped contents.
    A macro run starts at a preprocessor directive ('#' first) that either
    splices (ends with '\\') or leaves a block comment open, and continues
    while lines splice or sit inside an unclosed block comment.  Only
    backslashes outside comments are realigned.

    Returns ``(lines, warnings)``; each warning is ``(line_number, text)``
    flagging macro runs whose aligned lines contain characters that do
    not render one column wide.
    """
    n = len(lines)
    # Comment classification for the whole file first: block comments may
    # open in one macro line and close several lines later.
    masks = []
    block_open = []
    in_block = False
    for content in lines:
        mask, in_block = scan_line(content, in_block)
        masks.append(mask)
        block_open.append(in_block)

    out = list(lines)
    warnings = []
    i = 0
    while i < n:
        starts_directive = lines[i].lstrip().startswith('#')
        continues = lines[i].endswith('\\') or block_open[i]
        if starts_directive and continues:
            j = i
            while j < n and (lines[j].endswith('\\') or block_open[j]):
                j += 1
            members = range(i, j)
        elif lines[i].endswith('\\'):
            # A spliced run that does not start with '#': leave it alone.
            j = i + 1
            while j < n and lines[j].endswith('\\'):
                j += 1
            i = j
            continue
        else:
            i += 1
            continue
        # Lines whose trailing backslash is code get realigned; the others
        # (backslash inside a comment) stay as they are and do not count
        # towards the alignment column.
        contents = {k: lines[k][:-1].rstrip() for k in members
                    if lines[k].endswith('\\') and masks[k][-1]}
        if contents:
            odd = {k: [ch for ch in c if char_columns(ch) != 1]
                   for k, c in contents.items()}
            odd = {k: bad for k, bad in odd.items() if bad}
            if odd:
                first = min(odd)
                sample = odd[first][0]
                warnings.append((
                    first + 1,
                    '{} character(s) that do not render one column wide '
                    '(first: {!r} U+{:04X}) in macro spanning lines '
                    '{}-{}; backslashes are aligned by character count, '
                    'so the column may look misaligned in editors'.format(
                        sum(len(bad) for bad in odd.values()),
                        sample, ord(sample), i + 1, j)))
            width = max(len(c) for c in contents.values())
            for k, c in contents.items():
                out[k] = c + ' ' * (width - len(c) + 1) + '\\'
        i = j
    return out, warnings


def format_text(text, path, warnings_out=None):
    """Apply rules 2-5 (and rule 1 for C/C++ files) to ``text``.

    Macro alignment warnings, if any, are appended to ``warnings_out``
    as ``(line_number, text)`` pairs.
    """
    bom = ''
    if text.startswith('﻿'):
        bom = text[0]
        text = text[1:]

    expand_tabs = not is_tab_exempt(path)
    lines, tail = split_lines(text)

    def clean(content):
        if expand_tabs:
            content = content.replace('\t', '    ')
        return content.rstrip()

    lines = [(clean(content), eol) for content, eol in lines]

    # Rule 4: terminate a trailing partial line (if it is more than just
    # whitespace) with the file's dominant line ending.
    if tail:
        tail = clean(tail)
        if tail:
            eol = lines[-1][1] if lines else '\n'
            lines.append((tail, eol))

    # Rule 5: at most one trailing blank line.
    while len(lines) > 1 and lines[-1][0] == '' and lines[-2][0] == '':
        lines.pop()

    if is_macro_file(path):
        contents, macro_warnings = align_macro_backslashes(
            [c for c, _ in lines])
        if warnings_out is not None:
            warnings_out.extend(macro_warnings)
        lines = [(c, eol) for c, (_c, eol) in zip(contents, lines)]

    return bom + ''.join(content + eol for content, eol in lines)


def format_file(path, dry_run=False):
    """Format one file in place.  Returns True when the file changed."""
    try:
        with open(path, 'rb') as f:
            data = f.read()
    except OSError as e:
        print('error: cannot read {}: {}'.format(path, e), file=sys.stderr)
        return False
    if b'\x00' in data or len(data) > MAX_FILE_SIZE:
        return False  # binary or oversized: leave untouched
    old = data.decode('utf-8', errors='surrogateescape')
    warnings = []
    new = format_text(old, path, warnings)
    for line_no, message in warnings:
        print('warning: {}:{}: {}'.format(path, line_no, message),
              file=sys.stderr)
    if new == old:
        return False
    if not dry_run:
        with open(path, 'wb') as f:
            f.write(new.encode('utf-8', errors='surrogateescape'))
    return True


def git_output(args):
    out = subprocess.run(args, capture_output=True, check=True)
    return out.stdout.decode('utf-8', errors='surrogateescape')


def repo_root():
    """Return the absolute repository root, or None outside a repo."""
    try:
        return git_output(['git', 'rev-parse', '--show-toplevel']).strip()
    except (subprocess.CalledProcessError, OSError):
        return None


def collect_files(mode, root):
    """Return absolute paths of the tracked/staged files under ``root``
    and chdir to the repo root (so that git pathspec output resolves
    regardless of the caller's working directory)."""
    listing = (['git', 'diff', '--cached', '--name-only',
                '--diff-filter=ACM', '-z'] if mode == 'staged'
               else ['git', 'ls-files', '-z'])
    files = [p for p in git_output(listing).split('\0') if p]
    os.chdir(root)
    return [os.path.normpath(os.path.join(root, p)) for p in files]


def main(argv):
    args = [a for a in argv if a not in ('--staged', '--dry-run')]
    staged = '--staged' in argv
    dry_run = '--dry-run' in argv
    if staged and args:
        print('error: --staged cannot be combined with file arguments',
              file=sys.stderr)
        return 2
    root = repo_root()
    try:
        if staged or not args:
            if root is None:
                raise OSError('git rev-parse failed')
            files = collect_files('staged' if staged else 'all', root)
        else:
            files = args
    except (subprocess.CalledProcessError, OSError) as e:
        print('error: not a git repository ({})'.format(e), file=sys.stderr)
        return 2

    changed = []
    for path in files:
        if is_exempt_path(path, root):
            continue
        if format_file(path, dry_run):
            changed.append(path)
    for p in changed:
        print('{}: {}'.format('would format' if dry_run else 'formatted', p))
    verb = 'would format' if dry_run else 'formatted'
    print('{} {} file(s)'.format(verb, len(changed)))

    if changed and staged and not dry_run:
        subprocess.run(['git', 'add', '--'] +
                       [os.path.relpath(p, root) for p in changed],
                       check=True)
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
