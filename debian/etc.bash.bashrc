# System-wide .bashrc file for interactive bash(1) shells.

# If running interactively, then:
if [ "$PS1" ]; then

    # set a fancy prompt (overwrite the one in /etc/profile)
    PS1='${debian_chroot:+($debian_chroot)}\u@\h:\w\$ '

    # check the window size after each command and, if necessary,
    # update the values of LINES and COLUMNS.
    shopt -s checkwinsize

    # enable bash completion in interactive shells
    #if [ -f /etc/bash_completion ]; then
    #    . /etc/bash_completion
    #fi
fi
