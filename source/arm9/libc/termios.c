// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2026 Antonio Niño Díaz

#include <termios.h>
#include <unistd.h>

int tcgetattr(int fd, struct termios *termios_p)
{
    if (fd == STDIN_FILENO)
    {
        termios_p->c_iflag = IGNBRK | IGNPAR | IXANY;
        termios_p->c_oflag = ONLRET;
        termios_p->c_cflag = CS8 | CREAD;
        termios_p->c_lflag = 0;
        for (int i = 0; i < NCCS; i++)
            termios_p->c_cc[i] = -1; //_POSIX_VDISABLE;
        termios_p->c_ispeed = B2000000;
        termios_p->c_ospeed = B2000000;

        return 0;
    }

    return -1;
}

int tcsetattr(int fd, int optional_actions,
              const struct termios *termios_p)
{
    if ((fd == STDIN_FILENO) || (fd == STDOUT_FILENO) || (fd == STDERR_FILENO))
    {
        // Do nothing
        (void)optional_actions;
        (void)termios_p;
        return 0;
    }

    return -1;
}

int tcflush(int fd, int queue_selector)
{
    if ((fd == STDIN_FILENO) || (fd == STDOUT_FILENO) || (fd == STDERR_FILENO))
    {
        // Do nothing
        (void)queue_selector;
        return 0;
    }

    return -1;
}

int cfsetospeed(struct termios *termios_p, speed_t speed)
{
    termios_p->c_ospeed = speed;
    return 0;
}

speed_t cfgetospeed(const struct termios *termios_p)
{
    return termios_p->c_ospeed;
}
