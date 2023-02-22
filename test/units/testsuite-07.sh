#!/usr/bin/env bash
# SPDX-License-Identifier: LGPL-2.1-or-later
set -eux
set -o pipefail

: >/failed

cat >/lib/systemd/system/my.service <<EOF
[Service]
Type=oneshot
ExecStart=/bin/echo Timer runs me
EOF

cat >/lib/systemd/system/my.timer <<EOF
[Timer]
OnBootSec=10s
OnUnitInactiveSec=1h
EOF

systemctl unmask my.timer

systemctl start my.timer

mkdir -p /etc/systemd/system/my.timer.d/
cat >/etc/systemd/system/my.timer.d/override.conf <<EOF
[Timer]
OnBootSec=10s
OnUnitInactiveSec=1h
EOF

systemctl daemon-reload

systemctl mask my.timer

touch /testok
rm /failed
