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
#include "groups.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <glib/gprintf.h>


#ifdef HAVE_MPI

void groups_init()
{
	groupStack = NULL;
	groupMap = g_hash_table_new_full(g_str_hash, g_str_equal, & g_free, & g_free);
	sizeGroupmap = g_hash_table_new_full(g_str_hash, g_str_equal, & g_free, NULL);

	if(MPI_Comm_group(MPI_COMM_WORLD, &worldGroup) != MPI_SUCCESS)
		Error("MPI_Comm_group get error for COMM_WORLD\n");

	worldGroupBlock = groupblock_new(TRUE, size, worldGroup, MPI_COMM_WORLD);
	groupsDefined = FALSE;
}

void groups_free()
{
	if (worldGroupBlock) g_free(worldGroupBlock);
	if (sizeGroupmap) g_hash_table_destroy(sizeGroupmap);
	if (groupStack) g_list_free(groupStack);
	if (groupMap) g_hash_table_destroy(groupMap);
}

/**
 * New group definition descriptor
 */
Group* group_new(const gchar* name, GroupTag tag, gint subtag) {
	gint nameLength = (name? (strlen(name)+1) : 0);

	Group* def = g_malloc0(sizeof(Group) + nameLength);
	def->name = (name? ((gpointer) def + sizeof(Group)) : NULL);
	def->tag = tag;
	def->subtag = subtag;
	if (name) memcpy((gpointer) def->name, name, nameLength);
	return def;
}

/**
 * New group block descriptor
 */
GroupBlock* groupblock_new(gboolean member, gint groupsize, MPI_Group mpigroup, MPI_Comm mpicomm) {
	GroupBlock* block = g_malloc0(sizeof(GroupBlock));
	block->member = member;
	block->groupsize = groupsize;
	block->mpigroup = mpigroup;
	block->mpicomm = mpicomm;

	return block;
}

/**
 * We only need to sort the vardesc list in descending order of
 * the length of its names (variable names).
 */
static gint compare_groups(gconstpointer a, gconstpointer b) {
	Group* e0 = (Group*) a;
	Group* e1 = (Group*) b;

	if(e0->tag < e1->tag) {
		return -1;
	}
	else if(e0->tag > e1->tag) {
		return 1;
	}
	else {
		if(e0->subtag < e1->subtag)
			return -1;
		else if(e0->subtag > e1->subtag)
			return 1;
		else
			return 0;
	}
}

/**
 * Creates group block descriptors for each defined group in the test program.
 * It uses the groupsize values set by command line parameters from the sizeGroupmap.
 * If a group has been defined but no size has been set, it will be set to default size 0
 * and all code within the corresponding group block will be skipped.
 */
void create_groups(GSList* groupList) {
	GSList* iter;
	GroupBlock* block;
	gchar* name;
	int ret;
	int num;
	int* ranks;
	int i;
	int min_rank 	= 0; // minimum rank after TAG_SINGLE space
	int lower_bound = 0; // minimum rank for current mapping
	int last_subtag = 0; // subtag of the previously mapped group (will only be considered for TAG_DISJOINT)

	/* sort group defines list, we want SINGLE DISJOINT NONE order */
	groupList = g_slist_sort(groupList, compare_groups);
	iter = groupList;

	for(;iter;iter=g_slist_next(iter)) {
		Group* group = (Group*) iter->data;
		name = strdup(group->name);
		num = GPOINTER_TO_INT(g_hash_table_lookup(sizeGroupmap, name));

		MPI_Barrier(MPI_COMM_WORLD);

		Verbose("Creating new group: %s:%d", name, num);
		block = groupblock_new(FALSE, 0, MPI_GROUP_EMPTY, MPI_COMM_SELF);

		/* The group mapping will be accomplished the following way:
		 *
		 * +-------------------+ rank = size-1
		 * |    |     |    |   |
		 * | D0 | ... | Dk | N |
		 * |    |     |    |   |  ^
		 * +-------------------+  |
		 * |         S         |  |
		 * +-------------------+  |
		 * |        ...        |  |
		 * +-------------------+
		 * |         S         |
		 * +-------------------+ rank = 0
		 *
		 * S = TAG_SINGLE
		 * D = TAG_DISJOINT
		 * N = TAG_NONE
		 *
		 * That means single tagged groups will be mapped to the lower process space.
		 * Disjoint tagged groups will be mapped to the remaining process space,
		 * while the subtag allows to have disjoint sets of conglumerated groups (D1,..., Dk).
		 * Remind that a conglumerated group Di can consist of multiple subgroups.
		 * All untagged processes will be mapped to the remaining space above
		 * the single tagged process space.
		 */
		if((group->tag == TAG_DISJOINT) && (last_subtag != group->subtag)) {
			lower_bound = min_rank;
		}
		else if(group->tag == TAG_NONE) {
			lower_bound = min_rank;
		}

		if((num > 0) && (lower_bound < size)) {

			if(lower_bound+num > size) {
				if(rank == MASTER) {
					Warning("Mapping for group \"%s\" incomplete (%d of %d not mapped).\n", name, lower_bound+num-size, num);
				}
				// The remaining process space is too small to completely map the group.
				// We map it anyhow but will chop the groupsize though we have inclomplete mapping.
				num -= lower_bound+num-size;
			}

			// We map <num> ranks according to the defined group tags to each group.
			// If we reach the maximum rank, we will stop mapping the following ranks.
			// It may be that some groups will not be mapped if the user has specified
			// wrong mapping tags and group sizes for the current world size.
			ranks = (int *) calloc(num, sizeof(int));
			for(i=lower_bound; i<(lower_bound+num) && i<size; i++) {
				ranks[i-lower_bound] = i;
				Verbose("Assigning rank %d to group %s", i, name);
			}

			// Update the bounds for next group to map
			if(group->tag == TAG_SINGLE) {
				lower_bound += num;
				min_rank = lower_bound;
			}
			else if(group->tag == TAG_DISJOINT) {
				lower_bound += num;
				last_subtag = group->subtag;
			}

			MPI_Group_incl(worldGroup, num, ranks, &(block->mpigroup));
			ret = MPI_Comm_create(MPI_COMM_WORLD, block->mpigroup, &(block->mpicomm));
		    if(ret != 0){
		        Error("MPI_Comm_create error for group: %s ", name);
		    }

			if(block->mpicomm != MPI_COMM_NULL){
				block->member = TRUE;
				block->groupsize = num;
				g_hash_table_insert(groupMap, name, block);
		    }
		    // this process is not member
		    else {
				block->member = FALSE;
				block->groupsize = num;
				block->mpicomm = MPI_COMM_SELF;
				g_hash_table_insert(groupMap, name, block);
			}

			free(ranks);
		}
		else {
			if(rank == MASTER) {
				Warning("Group \"%s\" will not be mapped (please correct mapping parameters).\n", name);
			}

			g_hash_table_insert(groupMap, name, block);
		}
	}

}

inline GroupBlock* groupblock_get_active()
{
	if(groupStack)
		return ((GroupBlock*) g_list_first(groupStack)->data);
	else
		return worldGroupBlock;
}

/**
 * Fetches the group block identified by groupName.
 * If groupName is NULL, the current active group block is returned.
 * If no group block is active, the world group block will be returned.
 */
GroupBlock* groupblock_get(const gchar* groupName)
{
	if (groupName != NULL) {
		if (strcmp(groupName, "world") == 0)
			return worldGroupBlock;
		else
			return g_hash_table_lookup(groupMap, groupName);
	}
	else
		return groupblock_get_active();
}

#endif /* HAVE_MPI */
