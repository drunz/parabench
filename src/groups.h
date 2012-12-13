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

#ifndef GROUPS_H_
#define GROUPS_H_

#include "config.h"
#ifdef HAVE_MPI
  #include <mpi.h>
#endif
#include <glib.h>


#ifdef HAVE_MPI

typedef enum {
	TAG_SINGLE, TAG_DISJOINT, TAG_NONE
} GroupTag;

typedef struct {
	gchar* name;	// the name of this group which will be used as reference in group blocks
	GroupTag tag;	// allows to influence group mapping
	gint subtag;		// allows to create subgroups for disjoint tagged groups
} Group;

typedef struct {
	gboolean member;	// 1 if this process is member of the group
	gint groupsize;		// number of processes in this group
	MPI_Group mpigroup;	// mpi
	MPI_Comm mpicomm;	// mpi
} GroupBlock;


GHashTable* groupMap;     // groups map (name(string) -> descriptor(GroupBlock))
GHashTable* sizeGroupmap; // groups map (name(string) -> size(int)) (used for command line group size definition)

GList*      groupStack;   // stack used to manage active group blocks (used for implicit mpicomm fetch)

gboolean    groupsDefined;
GroupBlock* worldGroupBlock;
MPI_Group   worldGroup;


void		 groups_init();
void		 groups_free();

Group*       group_new(const gchar* name, GroupTag tag, gint subtag);
GroupBlock*	 groupblock_new(gboolean member, gint groupsize, MPI_Group mpigroup, MPI_Comm mpicomm);
void         create_groups(GSList* groupList);

inline GroupBlock* groupblock_get_active();
GroupBlock*	       groupblock_get();

#endif /* HAVE_MPI */

#endif /* GROUPS_H_ */
