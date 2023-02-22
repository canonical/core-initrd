#!/usr/bin/env bash
# SPDX-License-Identifier: LGPL-2.1-or-later
set -eux
set -o pipefail

systemd-analyze log-level debug
systemd-analyze log-target console

systemd-run --wait --unit=test27-one \
            -p StandardOutput=file:/tmp/stdout \
            -p StandardError=file:/tmp/stderr \
            -p Type=exec \
            sh -c 'echo x ; echo y >&2'
cmp /tmp/stdout <<EOF
x
EOF
cmp /tmp/stderr <<EOF
y
EOF

systemd-run --wait --unit=test27-two \
            -p StandardOutput=file:/tmp/stdout \
            -p StandardError=file:/tmp/stderr \
            -p Type=exec \
            sh -c 'echo z ; echo a >&2'
cmp /tmp/stdout <<EOF
z
EOF
cmp /tmp/stderr <<EOF
a
EOF

systemd-run --wait --unit=test27-three \
            -p StandardOutput=append:/tmp/stdout \
            -p StandardError=append:/tmp/stderr \
            -p Type=exec \
            sh -c 'echo b ; echo c >&2'
cmp /tmp/stdout <<EOF
z
b
EOF
cmp /tmp/stderr <<EOF
a
c
EOF

systemd-run --wait --unit=test27-four \
            -p StandardOutput=truncate:/tmp/stdout \
            -p StandardError=truncate:/tmp/stderr \
            -p Type=exec \
            sh -c 'echo a ; echo b >&2'
cmp /tmp/stdout <<EOF
a
EOF
cmp /tmp/stderr <<EOF
b
EOF

systemd-analyze log-level info

echo OK >/testok

exit 0
