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

#include "common.h"
#include "patterns.h"

#include <glib.h>
#include <glib/gprintf.h>


#ifdef HAVE_MPI

void patterns_init()
{
	patternMap = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, & g_free);
}

void patterns_free()
{
	if (patternMap) g_hash_table_destroy(patternMap);
}

Pattern* pattern_new(PatternType type, gint iter, gint elem, gint level) {
	Pattern *pattern = g_malloc0(sizeof(Pattern));
	pattern->type = type;
	pattern->iter = iter;
	pattern->elem = elem;
	pattern->level = level;

	return pattern;
}

void create_pattern(gchar* name, PatternType type, gint iter, gint elem, gint level, GroupBlock* group)
{
	Verbose("Creating pattern%d \"%s\" elem %d level %d\n", type, name, elem, level);
	Pattern* pattern = pattern_new(type, iter, elem, level);

	gint groupSize = (group? group->groupsize : size);
	gint groupRank;
	if (group)
		MPI_Comm_rank(group->mpicomm, &groupRank);
	else
		groupRank = rank;

	Verbose("GroupSize = %d, GroupRank = %d\n", groupSize, groupRank);

	MPI_Type_contiguous(elem, MPI_BYTE, &pattern->eType);
	MPI_Type_commit(&pattern->eType);
	pattern->type_size = 1;

	switch (type) {
		/* contiguous data */
		case PATTERN1: {
			int array_sizes[] = { groupSize };
			int array_subsizes[] = { 1 };
			int array_starts[] = { groupRank };

			MPI_Type_create_subarray(
				1,				/* number of array dimensions*/
				array_sizes,	/* number of eTypes in each dimension of the full array*/
				array_subsizes,	/* number of eTypes in each dimension of the subarray */
				array_starts,	/* starting coordinates of the subarray in each dimension*/
				MPI_ORDER_C,	/* array storage order flag (state) */
				pattern->eType,	/* eType (old datatype) */
				&pattern->datatype);
			MPI_Type_commit(&pattern->datatype);
			break;
		}

		/* non-contiguous data */
		case PATTERN2: {
			int array_sizes[] = { iter, groupSize };
			int array_subsizes[] = { iter, 1 };
			int array_starts[] = { 0, groupRank };

			MPI_Type_create_subarray(
				2,				/* number of array dimensions*/
				array_sizes,	/* number of eTypes in each dimension of the full array*/
				array_subsizes,	/* number of eTypes in each dimension of the subarray */
				array_starts,	/* starting coordinates of the subarray in each dimension*/
				MPI_ORDER_C,	/* array storage order flag (state) */
				pattern->eType,	/* eType (old datatype) */
				&pattern->datatype);
			MPI_Type_commit(&pattern->datatype);
			break;
		}

		default: Error("Pattern%d not yet supported!\n", type);
	}

	g_hash_table_insert(patternMap, name, pattern);
}

#endif /* HAVE_MPI */
