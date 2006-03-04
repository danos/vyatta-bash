#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <linux/kd.h>
#include <sys/ioctl.h>
#include <linux/vt.h>

#include <curses.h>
#include <term.h>

#define VERSION "0.1"

char* progname;

void usage()
{
  fprintf(stderr, "Usage: %s [option]\n", progname);
  fprintf(stderr, "valid options are:\n");
  fprintf(stderr, "\t-q  --quiet    don't print error messages\n");
  fprintf(stderr, "\t-h --help      display this help text and exit\n");
  fprintf(stderr, "\t-V --version   display version information and exit\n");
}

const struct option opts[] =
{
  /* operations */
    {"help", no_argument, 0, 'h'},
    {"version", no_argument, 0, 'V'},
    {"quiet", no_argument, 0, 'q'},
    {0, 0, 0, 0}
};

static int putch(int c)
{
        return putchar(c);
}


/* taken from console-utils, lib/misc-console-utils.c */

int is_a_console(int fd)
{
  char arg;

  arg = 0;
  return (ioctl(fd, KDGKBTYPE, &arg) == 0
          && ((arg == KB_101) || (arg == KB_84)));
}

static int open_a_console(char *fnam)
{
  int fd;

  /* try read-only */
  fd = open(fnam, O_RDWR);

  /* if failed, try read-only */
  if (fd < 0 && errno == EACCES)
      fd = open(fnam, O_RDONLY);

  /* if failed, try write-only */
  if (fd < 0 && errno == EACCES)
      fd = open(fnam, O_WRONLY);

  /* if failed, fail */
  if (fd < 0)
      return -1;

  /* if not a console, fail */
  if (! is_a_console(fd))
    {
      close(fd);
      return -1;
    }

  /* success */
  return fd;
}

/*
 * Get an fd for use with kbd/console ioctls.
 * We try several things because opening /dev/console will fail
 * if someone else used X (which does a chown on /dev/console).
 *
 * if tty_name is non-NULL, try this one instead.
 */

int get_console_fd(char* tty_name)
{
  int fd;

  if (tty_name)
    {
      if (-1 == (fd = open_a_console(tty_name)))
        return -1;
      else
        return fd;
    }

  fd = open_a_console("/dev/tty");
  if (fd >= 0)
    return fd;

  fd = open_a_console("/dev/tty0");
  if (fd >= 0)
    return fd;

  fd = open_a_console("/dev/console");
  if (fd >= 0)
    return fd;

  for (fd = 0; fd < 3; fd++)
    if (is_a_console(fd))
      return fd;

#if 0
  fprintf(stderr,
          _("Couldnt get a file descriptor referring to the console\n"));
#endif
  return -1;            /* total failure */
}


int main (int argc, char* argv[])
{
  int fd, num, tmp_num;
  int result;                          /* option handling */
  int an_option;
  int quiet = 0;
  struct vt_stat vtstat;

  if ((progname = strrchr(argv[0], '/')) == NULL)
    progname = argv[0];
  else
    progname++;

  while (1)
    {
      result = getopt_long(argc, argv, "Vhq", opts, &an_option);
      
      if (result == EOF)
	break;

      switch (result)
        {
        case 'V':
	  fprintf(stdout, "%s: Version %s\n", progname, VERSION);
          exit (0);
        case 'h':
          usage();
          exit (0);
	  
        case 'q':
          quiet = 1;
        }
    }

  if (optind < argc)
    {
      if (!quiet)
	fprintf(stderr, "%s: no non-option arguments are valid", progname);
      exit(1);
    }

  if (getenv("DISPLAY"))
    {
      if (!quiet)
	fprintf(stderr, "%s: not a console (DISPLAY is set)\n", progname);
      exit(1);
    }

  if ((fd = get_console_fd(NULL)) == -1)
    {
      if (!quiet)
	fprintf(stderr, "%s: terminal is not a console\n", progname);
      exit(1);
    }

  /* clear screen */
  setupterm((char *) 0, 1, (int *) 0);
  if (tputs(clear_screen, lines > 0 ? lines : 1, putch) == ERR)
    {
      exit(1);
    }

  /* get current vt */
  if (ioctl(fd, VT_GETSTATE, &vtstat) < 0)
    {
      if (!quiet)
	fprintf(stderr, "%s: cannot get VTstate\n", progname);
      exit(1);
    }
  num = vtstat.v_active;
  tmp_num = (num == 1 ? 2 : 1);

  /* switch vt to clear the scrollback buffer */
  if (ioctl(fd, VT_ACTIVATE, tmp_num))
    {
      if (!quiet)
	perror("chvt: VT_ACTIVATE");
      exit(1);
    }

  if (ioctl(fd, VT_WAITACTIVE, tmp_num))
    {
      if (!quiet)
	perror("VT_WAITACTIVE");
      exit(1);
    }

  /* switch back */
  if (ioctl(fd, VT_ACTIVATE, num))
    {
      if (!quiet)
	perror("chvt: VT_ACTIVATE");
      exit(1);
    }

  if (ioctl(fd, VT_WAITACTIVE, num))
    {
      if (!quiet)
	perror("VT_WAITACTIVE");
      exit(1);
    }

  return 0;
}

