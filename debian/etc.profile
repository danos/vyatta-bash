# /etc/profile: system-wide .profile file for the Bourne shell (sh(1))
# and Bourne compatible shells (bash(1), ksh(1), ash(1), ...).

PATH='/usr/local/bin:/usr/bin:/bin:/usr/bin/X11:/usr/games'

case x$BASH in
  x) case x$USER in xroot) PS1='# ';; *) PS1='$ '; esac;;
  *) PS1='\u@\h:\w \$ '
esac

export PATH PS1

umask 022
