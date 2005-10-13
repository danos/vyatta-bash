# ~/.bash_logout: executed by bash(1) when login shell exits.

# when leaving the console clear the screen to increase privacy

# Are we a login shell?
if [ "X`ps --pid $PPID -o comm=`" = "Xlogin" ]; then
    # Just clean the screen, requires ncurses-bin being installed
    [ -x /usr/bin/clear ] && /usr/bin/clear
fi
