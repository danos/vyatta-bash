# /etc/profile: system-wide .profile file for bash(1).

PATH="/usr/local/bin:/usr/bin:/bin:/usr/bin/X11:/usr/games:."
PS1="\\$ "

export PATH PS1

ulimit -c unlimited
umask 002
