/* pushd.c, created from pushd.def. */
#include <config.h>

#include <stdio.h>
#include <sys/param.h>

#if defined (HAVE_UNISTD_H)
#  include <unistd.h>
#endif

#include "bashansi.h"

#include <errno.h>

#include <tilde/tilde.h>

#include "shell.h"
#include "builtins.h"
#include "maxpath.h"
#include "common.h"

#if !defined (errno)
extern int errno;
#endif /* !errno */

static char *m_badarg = "%s: bad argument";

/* The list of remembered directories. */
static char **pushd_directory_list = (char **)NULL;

/* Number of existing slots in this list. */
static int directory_list_size;

/* Offset to the end of the list. */
static int directory_list_offset;

static void pushd_error ();
static void clear_directory_stack ();
static int cd_to_string ();
static int change_to_temp ();
static int get_dirstack_index ();
static void add_dirstack_element ();

#define NOCD		0x01
#define ROTATE		0x02
#define LONGFORM	0x04
#define CLEARSTAK	0x08

int
pushd_builtin (list)
     WORD_LIST *list;
{
  char *temp, *current_directory, *top;
  int j, flags;
  long num;
  char direction;

  /* If there is no argument list then switch current and
     top of list. */
  if (list == 0)
    {
      if (directory_list_offset == 0)
	{
	  builtin_error ("no other directory");
	  return (EXECUTION_FAILURE);
	}

      current_directory = get_working_directory ("pushd");
      if (current_directory == 0)
	return (EXECUTION_FAILURE);

      j = directory_list_offset - 1;
      temp = pushd_directory_list[j];
      pushd_directory_list[j] = current_directory;
      j = change_to_temp (temp);
      free (temp);
      return j;
    }

  for (flags = 0; list; list = list->next)
    {
      if (ISOPTION (list->word->word, 'n'))
	{
	  flags |= NOCD;
	}
      else if (ISOPTION (list->word->word, '-'))
        {
          list = list->next;
          break;
        }
      else if (list->word->word[0] == '-' && list->word->word[1] == '\0')
	/* Let `pushd -' work like it used to. */
	break;
      else if (((direction = list->word->word[0]) == '+') || direction == '-')
	{
	  if (legal_number (list->word->word + 1, &num) == 0)
	    {
	      builtin_error (m_badarg, list->word->word);
	      builtin_usage ();
	      return (EXECUTION_FAILURE);
	    }

	  if (direction == '-')
	    num = directory_list_offset - num;

	  if (num > directory_list_offset || num < 0)
	    {
	      pushd_error (directory_list_offset, list->word->word);
	      return (EXECUTION_FAILURE);
	    }
	  flags |= ROTATE;
	}
      else if (*list->word->word == '-')
	{
	  bad_option (list->word->word);
	  builtin_usage ();
	  return (EXECUTION_FAILURE);
	}
      else
	break;
    }

  if (flags & ROTATE)
    {
      /* Rotate the stack num times.  Remember, the current
	 directory acts like it is part of the stack. */
      temp = get_working_directory ("pushd");

      if (num == 0)
	{
	  j = ((flags & NOCD) == 0) ? change_to_temp (temp) : EXECUTION_SUCCESS;
	  free (temp);
	  return j;
	}

      do
	{
	  top = pushd_directory_list[directory_list_offset - 1];

	  for (j = directory_list_offset - 2; j > -1; j--)
	    pushd_directory_list[j + 1] = pushd_directory_list[j];

	  pushd_directory_list[j + 1] = temp;

	  temp = top;
	  num--;
	}
      while (num);

      j = ((flags & NOCD) == 0) ? change_to_temp (temp) : EXECUTION_SUCCESS;
      free (temp);
      return j;
    }

  if (list == 0)
    return (EXECUTION_SUCCESS);

  /* Change to the directory in list->word->word.  Save the current
     directory on the top of the stack. */
  current_directory = get_working_directory ("pushd");
  if (current_directory == 0)
    return (EXECUTION_FAILURE);

  j = ((flags & NOCD) == 0) ? cd_builtin (list) : EXECUTION_SUCCESS;
  if (j == EXECUTION_SUCCESS)
    {
      add_dirstack_element ((flags & NOCD) ? savestring (list->word->word) : current_directory);
      dirs_builtin ((WORD_LIST *)NULL);
      return (EXECUTION_SUCCESS);
    }
  else
    {
      free (current_directory);
      return (EXECUTION_FAILURE);
    }
}

/* Pop the directory stack, and then change to the new top of the stack.
   If LIST is non-null it should consist of a word +N or -N, which says
   what element to delete from the stack.  The default is the top one. */
int
popd_builtin (list)
     WORD_LIST *list;
{
  register int i;
  long which;
  int flags;
  char direction;

  for (flags = 0, which = 0L, direction = '+'; list; list = list->next)
    {
      if (ISOPTION (list->word->word, 'n'))
        {
          flags |= NOCD;
        }
      else if (ISOPTION (list->word->word, '-'))
        {
          list = list->next;
          break;
        }
      else if (((direction = list->word->word[0]) == '+') || direction == '-')
	{
	  if (legal_number (list->word->word + 1, &which) == 0)
	    {
	      builtin_error (m_badarg, list->word->word);
	      builtin_usage ();
	      return (EXECUTION_FAILURE);
	    }
	}
      else if (*list->word->word == '-')
	{
	  bad_option (list->word->word);
	  builtin_usage ();
	  return (EXECUTION_FAILURE);
	}
      else
	break;
    }

  if (which > directory_list_offset || (directory_list_offset == 0 && which == 0))
    {
      pushd_error (directory_list_offset, list ? list->word->word : "");
      return (EXECUTION_FAILURE);
    }

  /* Handle case of no specification, or top of stack specification. */
  if ((direction == '+' && which == 0) ||
      (direction == '-' && which == directory_list_offset))
    {
      i = ((flags & NOCD) == 0) ? cd_to_string (pushd_directory_list[directory_list_offset - 1])
      			        : EXECUTION_SUCCESS;
      if (i != EXECUTION_SUCCESS)
	return (i);
      free (pushd_directory_list[--directory_list_offset]);
    }
  else
    {
      /* Since an offset other than the top directory was specified,
	 remove that directory from the list and shift the remainder
	 of the list into place. */
      i = (direction == '+') ? directory_list_offset - which : which;
      free (pushd_directory_list[i]);
      directory_list_offset--;

      /* Shift the remainder of the list into place. */
      for (; i < directory_list_offset; i++)
	pushd_directory_list[i] = pushd_directory_list[i + 1];
    }

  dirs_builtin ((WORD_LIST *)NULL);
  return (EXECUTION_SUCCESS);
}

/* Print the current list of directories on the directory stack. */
int
dirs_builtin (list)
     WORD_LIST *list;
{
  int flags, desired_index, index_flag, vflag;
  long i;
  char *temp, *w;

  for (flags = vflag = index_flag = 0, desired_index = -1, w = ""; list; list = list->next)
    {
      if (ISOPTION (list->word->word, 'l'))
	{
	  flags |= LONGFORM;
	}
      else if (ISOPTION (list->word->word, 'c'))
	{
	  flags |= CLEARSTAK;
	}
      else if (ISOPTION (list->word->word, 'v'))
	{
	  vflag |= 2;
	}
      else if (ISOPTION (list->word->word, 'p'))
	{
	  vflag |= 1;
	}
      else if (ISOPTION (list->word->word, '-'))
        {
          list = list->next;
          break;
        }
      else if (*list->word->word == '+' || *list->word->word == '-')
        {
          int sign;
          if (legal_number (w = list->word->word + 1, &i) == 0)
	    {
	      builtin_error (m_badarg, list->word->word);
	      builtin_usage ();
	      return (EXECUTION_FAILURE);
	    }
	  sign = (*list->word->word == '+') ? 1 : -1;
	  desired_index = get_dirstack_index (i, sign, &index_flag);
	}
      else
	{
	  bad_option (list->word->word);
	  builtin_usage ();
	  return (EXECUTION_FAILURE);
	}
    }

  if (flags & CLEARSTAK)
    {
      clear_directory_stack ();
      return (EXECUTION_SUCCESS);
    }

  if (index_flag && (desired_index < 0 || desired_index > directory_list_offset))
    {
      pushd_error (directory_list_offset, w);
      return (EXECUTION_FAILURE);
    }

#define DIRSTACK_FORMAT(temp) \
  (flags & LONGFORM) ? temp : polite_directory_format (temp)

  /* The first directory printed is always the current working directory. */
  if (index_flag == 0 || (index_flag == 1 && desired_index == 0))
    {
      temp = get_working_directory ("dirs");
      if (temp == 0)
	temp = savestring ("<no current directory>");
      if (vflag & 2)
	printf ("%2d  %s", 0, DIRSTACK_FORMAT (temp));
      else
	printf ("%s", DIRSTACK_FORMAT (temp));
      free (temp);
      if (index_flag)
	{
	  putchar ('\n');
	  return EXECUTION_SUCCESS;
	}
    }

#define DIRSTACK_ENTRY(i) \
  (flags & LONGFORM) ? pushd_directory_list[i] \
		     : polite_directory_format (pushd_directory_list[i])

  /* Now print the requested directory stack entries. */
  if (index_flag)
    {
      if (vflag & 2)
	printf ("%2d  %s", directory_list_offset - desired_index,
			   DIRSTACK_ENTRY (desired_index));
      else
	printf ("%s", DIRSTACK_ENTRY (desired_index));
    }
  else
    for (i = directory_list_offset - 1; i >= 0; i--)
      if (vflag >= 2)
	printf ("\n%2d  %s", directory_list_offset - (int)i, DIRSTACK_ENTRY (i));
      else
	printf ("%s%s", (vflag & 1) ? "\n" : " ", DIRSTACK_ENTRY (i));

  putchar ('\n');
  fflush (stdout);
  return (EXECUTION_SUCCESS);
}

static void
pushd_error (offset, arg)
     int offset;
     char *arg;
{
  if (offset == 0)
    builtin_error ("directory stack empty");
  else if (arg)
    builtin_error ("%s: bad directory stack index", arg);
  else
    builtin_error ("bad directory stack index");
}

static void
clear_directory_stack ()
{
  register int i;

  for (i = 0; i < directory_list_offset; i++)
    free (pushd_directory_list[i]);
  directory_list_offset = 0;
}

/* Switch to the directory in NAME.  This uses the cd_builtin to do the work,
   so if the result is EXECUTION_FAILURE then an error message has already
   been printed. */
static int
cd_to_string (name)
     char *name;
{
  WORD_LIST *tlist;
  int result;

  tlist = make_word_list (make_word (name), NULL);
  result = cd_builtin (tlist);
  dispose_words (tlist);
  return (result);
}

static int
change_to_temp (temp)
     char *temp;
{
  int tt;

  tt = temp ? cd_to_string (temp) : EXECUTION_FAILURE;

  if (tt == EXECUTION_SUCCESS)
    dirs_builtin ((WORD_LIST *)NULL);

  return (tt);
}

static void
add_dirstack_element (dir)
     char *dir;
{
  int j;

  if (directory_list_offset == directory_list_size)
    {
      j = (directory_list_size += 10) * sizeof (char *);
      pushd_directory_list = (char **)xrealloc (pushd_directory_list, j);
    }
  pushd_directory_list[directory_list_offset++] = dir;
}

static int
get_dirstack_index (ind, sign, indexp)
     int ind, sign, *indexp;
{
  if (indexp)
    *indexp = sign > 0 ? 1 : 2;

  /* dirs +0 prints the current working directory. */
  /* dirs -0 prints last element in directory stack */
  if (ind == 0 && sign > 0)
    return 0;
  else if (ind == directory_list_offset)
    {
      if (indexp)
	*indexp = sign > 0 ? 2 : 1;
      return 0;
    }
  else
    return (sign > 0 ? directory_list_offset - ind : ind);
}

char *
get_dirstack_element (ind, sign)
     int ind, sign;
{
  int i;

  i = get_dirstack_index (ind, sign, (int *)NULL);
  return (i < 0 || i > directory_list_offset) ? (char *)NULL
					      : pushd_directory_list[i];
}

void
set_dirstack_element (ind, sign, value)
     int ind, sign;
     char *value;
{
  int i;

  i = get_dirstack_index (ind, sign, (int *)NULL);
  if (ind == 0 || i < 0 || i > directory_list_offset)
    return;
  free (pushd_directory_list[i]);
  pushd_directory_list[i] = savestring (value);
}

WORD_LIST *
get_directory_stack ()
{
  register int i;
  WORD_LIST *ret;
  char *d, *t;

  for (ret = (WORD_LIST *)NULL, i = 0; i < directory_list_offset; i++)
    {
      d = polite_directory_format (pushd_directory_list[i]);
      ret = make_word_list (make_word (d), ret);
    }
  /* Now the current directory. */
  d = get_working_directory ("dirstack");
  i = 0;	/* sentinel to decide whether or not to free d */
  if (d == 0)
    d = ".";
  else
    {
      t = polite_directory_format (d);
      /* polite_directory_format sometimes returns its argument unchanged.
	 If it does not, we can free d right away.  If it does, we need to
	 mark d to be deleted later. */
      if (t != d)
	{
	  free (d);
	  d = t;
	}
      else /* t == d, so d is what we want */
	i = 1;
    }
  ret = make_word_list (make_word (d), ret);
  if (i)
    free (d);
  return ret;	/* was (REVERSE_LIST (ret, (WORD_LIST *)); */
}

static char *dirs_doc[] = {
  "Display the list of currently remembered directories.  Directories",
  "find their way onto the list with the `pushd' command; you can get",
  "back up through the list with the `popd' command.",
  "",
  "The -l flag specifies that `dirs' should not print shorthand versions",
  "of directories which are relative to your home directory.  This means",
  "that `~/bin' might be displayed as `/homes/bfox/bin'.  The -v flag",
  "causes `dirs' to print the directory stack with one entry per line,",
  "prepending the directory name with its position in the stack.  The -p",
  "flag does the same thing, but the stack position is not prepended.",
  "The -c flag clears the directory stack by deleting all of the elements.",
  "",
  "+N   displays the Nth entry counting from the left of the list shown by",
  "     dirs when invoked without options, starting with zero.",
  "",
  "-N   displays the Nth entry counting from the right of the list shown by",
  "     dirs when invoked without options, starting with zero.",
  (char *)NULL
};

static char *pushd_doc[] = {
  "Adds a directory to the top of the directory stack, or rotates",
  "the stack, making the new top of the stack the current working",
  "directory.  With no arguments, exchanges the top two directories.",
  "",
  "+N   Rotates the stack so that the Nth directory (counting",
  "     from the left of the list shown by `dirs') is at the top.",
  "",
  "-N   Rotates the stack so that the Nth directory (counting",
  "     from the right) is at the top.",
  "",
  "-n   suppress the normal change of directory when adding directories",
  "     to the stack, so only the stack is manipulated.",
  "",
  "dir  adds DIR to the directory stack at the top, making it the",
  "     new current working directory.",
  "",
  "You can see the directory stack with the `dirs' command.",
  (char *)NULL
};

static char *popd_doc[] = {
  "Removes entries from the directory stack.  With no arguments,",
  "removes the top directory from the stack, and cd's to the new",
  "top directory.",
  "",
  "+N   removes the Nth entry counting from the left of the list",
  "     shown by `dirs', starting with zero.  For example: `popd +0'",
  "     removes the first directory, `popd +1' the second.",   
  "",
  "-N   removes the Nth entry counting from the right of the list",
  "     shown by `dirs', starting with zero.  For example: `popd -0'",
  "     removes the last directory, `popd -1' the next to last.",
  "",
  "-n   suppress the normal change of directory when removing directories",
  "     from the stack, so only the stack is manipulated.",
  "",
  "You can see the directory stack with the `dirs' command.",
  (char *)NULL
};

struct builtin pushd_struct = {
	"pushd",
	pushd_builtin,
	BUILTIN_ENABLED,
	pushd_doc,
	"pushd [+N | -N] [-n] [dir]",
	0
};

struct builtin popd_struct = {
	"popd",
	popd_builtin,
	BUILTIN_ENABLED,
	popd_doc,
	"popd [+N | -N] [-n]",
	0
};

struct builtin dirs_struct = {
	"dirs",
	dirs_builtin,
	BUILTIN_ENABLED,
	dirs_doc,
	"dirs [-clpv] [+N] [-N]",
	0
};
