#!/usr/bin/env python3
"""Strip a semicolon that ends an import line in a staged redscript tree.

The Mac game compiler rejects `import NGPlus.*;`. The NG+ sources are not edited.
build-all.sh runs this on the staged copy. Print each file that changes.
"""
import os
import sys


def transform(text):
    changed = 0
    out = []
    for line in text.splitlines(keepends=True):
        nl = ''
        body = line
        if body.endswith('\r\n'):
            body, nl = body[:-2], '\r\n'
        elif body.endswith('\n'):
            body, nl = body[:-1], '\n'
        elif body.endswith('\r'):
            body, nl = body[:-1], '\r'
        stripped = body.rstrip(' \t')
        trail = body[len(stripped):]
        code = stripped.lstrip(' \t')
        if code.startswith('import ') and stripped.endswith(';'):
            stripped = stripped[:-1].rstrip(' \t')
            body = stripped + trail
            changed += 1
        out.append(body + nl)
    return ''.join(out), changed


def main(root):
    root = os.path.abspath(root)
    if not os.path.isdir(root):
        print('transform-reds: not a directory: %s' % root, file=sys.stderr)
        return 2
    nfiles = 0
    for dirpath, _, names in os.walk(root):
        for name in sorted(names):
            if not name.endswith('.reds'):
                continue
            path = os.path.join(dirpath, name)
            with open(path, 'rb') as handle:
                raw = handle.read()
            text = raw.decode('utf-8')
            new, count = transform(text)
            if count:
                with open(path, 'w', encoding='utf-8', newline='') as handle:
                    handle.write(new)
                rel = os.path.relpath(path, root)
                suffix = '' if count == 1 else 's'
                print('ngplus scripts: stripped import semicolon in %s (%d line%s)' % (rel, count, suffix))
                nfiles += 1
    if nfiles == 0:
        print('ngplus scripts: no import semicolons to strip')
    return 0


if __name__ == '__main__':
    if len(sys.argv) != 2:
        print('usage: transform-reds.py <staged-script-dir>', file=sys.stderr)
        sys.exit(2)
    sys.exit(main(sys.argv[1]))
