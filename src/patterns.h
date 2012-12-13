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

#ifndef PATTERNS_H_
#define PATTERNS_H_

#include "config.h"
#ifdef HAVE_MPI
  #include <mpi.h>
#endif
#include "groups.h"


#ifdef HAVE_MPI

GHashTable* patternMap;		// pattern map (name(string) -> pattern(Pattern))

typedef enum {
	PATTERN0, PATTERN1, PATTERN2, PATTERN3
} PatternType;

typedef struct {
	PatternType type;		// defines the access pattern type to a file (currently only PATTERN2 implemented)
	gint iter;				// number of iterations in level 0 and 1 (also used in 3, 4 to calculate buffer size)
	gint elem;				// number of elements per process
	gint level;				// the level to access in (0=NC/C, 1=C/C, 2=NC/NC, 3=C/NC)
	MPI_Datatype datatype;	// the datatype which is used to represent data (currently mpi array)
	MPI_Datatype eType;		// elementary datatype
	gint type_size;			// size of the mpi datatype
} Pattern;


void patterns_init();
void patterns_free();

Pattern* pattern_new(PatternType type, gint iter, gint elem, gint level);
void     create_pattern(gchar* name, PatternType type, gint iter, gint elem, gint level, GroupBlock* group);

#endif /* HAVE_MPI */

#endif /* PATTERNS_H_ */
