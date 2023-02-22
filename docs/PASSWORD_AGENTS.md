---
title: Password Agents
category: Interfaces
layout: default
SPDX-License-Identifier: LGPL-2.1-or-later
---

# Password Agents

systemd 12 and newer support lightweight password agents which can be used to query the user for system-level passwords or passphrases. These are passphrases that are not related to a specific user, but to some kind of hardware or service. Right now this is used exclusively for encrypted hard-disk passphrases but later on this is likely to be used to query passphrases of SSL certificates at Apache startup time as well. The basic idea is that a system component requesting a password entry can simply drop a simple .ini-style file into `/run/systemd/ask-password` which multiple different agents may watch via `inotify()`, and query the user as necessary. The answer is then sent back to the querier via an `AF_UNIX`/`SOCK_DGRAM` socket. Multiple agents might be running at the same time in which case they all should query the user and the agent which answers first wins. Right now systemd ships with the following passphrase agents:

* A Plymouth agent used for querying passwords during boot-up
* A console agent used in similar situations if Plymouth is not available
* A GNOME agent which can be run as part of the normal user session which pops up a notification message and icon which when clicked receives the passphrase from the user. This is useful and necessary in case an encrypted system hard-disk is plugged in when the machine is already up.
* A [`wall(1)`](http://man7.org/linux/man-pages/man1/wall.1.html) agent which sends wall messages as soon as a password shall be entered.
* A simple tty agent which is built into "`systemctl start`" (and similar commands) and asks passwords to the user during manual startup of a service
* A simple tty agent which can be run manually to respond to all queued passwords

It is easy to write additional agents. The basic algorithm to follow looks like this:

* Create an inotify watch on /run/systemd/ask-password, watch for `IN_CLOSE_WRITE|IN_MOVED_TO`
* Ignore all events on files in that directory that do not start with "`ask.`"
* As soon as a file named "`ask.xxxx`" shows up, read it. It's a simple `.ini` file that may be parsed with the usual parsers. The `xxxx` suffix is randomized.
* Make sure to ignore unknown `.ini` file keys in those files, so that we can easily extend the format later on.
* You'll find the question to ask the user in the `Message=` field in the `[Ask]` section. It is a single-line string in UTF-8, which might be internationalized (by the party that originally asks the question, not by the agent).
* You'll find an icon name (following the XDG icon naming spec) to show next to the message in the `Icon=` field in the `[Ask]` section
* You'll find the PID of the client asking the question in the `PID=` field in the `[Ask]` section (Before asking your question use `kill(PID, 0)` and ignore the file if this returns `ESRCH`; there's no need to show the data of this field but if you want to you may)
* `Echo=` specifies whether the input should be obscured. If this field is missing or is `Echo=0`, the input should not be shown.
* The socket to send the response to is configured via `Socket=` in the `[Ask]` section. It is a `AF_UNIX`/`SOCK_DGRAM` socket in the file system.
* Ignore files where the time specified in the `NotAfter=` field in the `[Ask]` section is in the past. The time is specified in usecs, and refers to the `CLOCK_MONOTONIC` clock. If `NotAfter=` is `0`, no such check should take place.
* Make sure to hide a password query dialog as soon as a) the `ask.xxxx` file is deleted, watch this with inotify. b) the `NotAfter=` time elapses, if it is set `!= 0`.
* Access to the socket is restricted to privileged users. To acquire the necessary privileges to send the answer back, consider using PolicyKit. In fact, the GNOME agent we ship does that, and you may simply piggyback on that, by executing "`/usr/bin/pkexec /lib/systemd/systemd-reply-password 1 /path/to/socket`" or "`/usr/bin/pkexec /lib/systemd/systemd-reply-password 0 /path/to/socket`" and writing the password to its standard input. Use '`1`' as argument if a password was entered by the user, or '`0`' if the user canceled the request.
* If you do not want to use PK ensure to acquire the necessary privileges in some other way and send a single datagram to the socket consisting of the password string either prefixed with "`+`" or with "`-`" depending on whether the password entry was successful or not. You may but don't have to include a final `NUL` byte in your message.

Again, it is essential that you stop showing the password box/notification/status icon if the `ask.xxx` file is removed or when `NotAfter=` elapses (if it is set `!= 0`)!

It may happen that multiple password entries are pending at the same time. Your agent needs to be able to deal with that. Depending on your environment you may either choose to show all outstanding passwords at the same time or instead only one and as soon as the user has replied to that one go on to the next one.

You may test this all with manually invoking the "`systemd-ask-password`" tool on the command line. Pass `--no-tty` to ensure the password is asked via the agent system. Note that only privileged users may use this tool (after all this is intended purely for system-level passwords).

If you write a system level agent a smart way to activate it is using systemd `.path` units. This will ensure that systemd will watch the `/run/systemd/ask-password` directory and spawn the agent as soon as that directory becomes non-empty. In fact, the console, wall and Plymouth agents are started like this. If systemd is used to maintain user sessions as well you can use a similar scheme to automatically spawn your user password agent as well. (As of this moment we have not switched any DE over to use systemd for session management, however.)
