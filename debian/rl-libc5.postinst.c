/* A C wrapper to ldconfig for libreadline2_2.1-2.1 and later (?) */
/* Copyright (C) 1997, James Troup <jjtroup@comp.brad.ac.uk> */

/* This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

int
main (int argc, char **argv)
{
  execlp("ldconfig", "ldconfig", 0);
  fprintf(stderr, "Error: exec ldconfig : %s\n", strerror (errno));
  fprintf(stderr, "Run ldconfig by hand, or bash might be unusable !\n");
  return(1);
}
