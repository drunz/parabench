/* Parabench - A parallel file system benchmark
 * Copyright (C) 2009-2010  Dennis Runz
 * University of Heidelberg
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef COMMON_H_
#define COMMON_H_

#include "config.h"
#ifdef HAVE_MPI
  #include <mpi.h>
#endif
#include <glib.h>
#include <stdarg.h>


#define MASTER 0				// master process rank
#define NAME_SIZE 255			// the size of the static time event name string

int rank;						// MPI rank of this process
int size;						// number of MPI processes


#define MPI_ASSERT(X, Y, Z)  { \
  int rc = (X); \
  if (rc != MPI_SUCCESS) { \
    ErrorMPI((Y), rc, (Z)); \
  } \
}


#define Log(message, ...) _log("", message, ## __VA_ARGS__)
#define Warning(message, ...) _log("Warning: ", message, ## __VA_ARGS__)
#define Error(message, ...) _error(message, ## __VA_ARGS__)

#ifdef _VERBOSE
  #define Verbose(message, ...) _log("", message, ## __VA_ARGS__)
#else
  #define Verbose(message, ...) 0
#endif

void _log(const gchar* prefix, const gchar* message, ...);
void _error(const gchar* message, ...);
void ErrorMPI(gchar* location, gint error_code, gboolean quit);

gchar* time_str();
gchar* date_str();

#endif /* COMMON_H_ */
