#include <stdlib.h>

extern int rl_shell;
extern char* (*rl_get_string_value_hook)();

char *
get_env_value (varname)
     char *varname;
{
  return (rl_shell ?
	  (rl_get_string_value_hook)(varname) :
	  ((char *)getenv (varname)));
}
