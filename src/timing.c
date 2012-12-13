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

#include "config.h"
#include "common.h"
#include "timing.h"
#include <string.h>


void timing_init()
{
	timeList = NULL;
	coreTimeList = NULL;

	coreTimeStack = NULL;
}

void timing_free()
{
	if (timeList) {
		g_slist_foreach(timeList, (GFunc) g_free, NULL);
		g_slist_free(timeList);
	}

	if (coreTimeList) {
		g_slist_foreach(coreTimeList, (GFunc) g_free, NULL);
		g_slist_free(coreTimeList);
	}

	if (coreTimeStack)
		g_list_free(coreTimeStack);
}

/**
 * New TimeEvent
 */
TimeEvent* timeevent_new(gint id, const gchar* name, gdouble value)
{
	TimeEvent *event = g_malloc0(sizeof(TimeEvent));
#ifdef HAVE_MPI
	event->proc = rank;
#else
	event->proc = 0;
#endif
	event->id = id;
	strncpy(event->name, name, NAME_SIZE);
	event->value = value;

	return event;
}

CoreTimeEvent* coretime_event_new(gint id, const gchar* name, CoreTime coreTime)
{
	CoreTimeEvent* event = g_malloc0(sizeof(CoreTimeEvent));
#ifdef HAVE_MPI
	event->proc = rank;
#else
	event->proc = 0;
#endif
	event->id = id;
	strncpy(event->name, name, NAME_SIZE);
	event->avgCoreTime = coreTime;
	event->minCoreTime = coreTime;
	event->maxCoreTime = coreTime;
	event->numCalls = 0;
	event->minCallTime = G_MAXDOUBLE;
	event->maxCallTime = G_MINDOUBLE;

	return event;
}

CoreTime coretime_new(gdouble time, glong data)
{
	CoreTime coreTime;
	coreTime.time = time;
	coreTime.data = data;

	return coreTime;
}

gchar* format_coretime_throughput(CoreTime coreTime)
{
	gchar* suffixes[] = { "B", "KiB", "MiB", "GiB", "TiB", "PiB", "EiB", "ZiB", "YiB" };
	gint s = 0;
	gdouble tp = (coreTime.time? coreTime.data/coreTime.time : 0);

	while (tp >= 1024) {
		s++;
		tp /= 1024;
	}

	GString* buffer = g_string_new("");
	g_string_append_printf(buffer, "%7.2f %3s/s", tp, suffixes[s]);

	gchar* string = buffer->str;
	g_string_free(buffer, FALSE);
	return string;
}

gchar* format_data_size(glong dataSize)
{
	gchar* suffixes[] = { "B", "KiB", "MiB", "GiB", "TiB", "PiB", "EiB", "ZiB", "YiB" };
	gint s = 0;
	gdouble frac = dataSize;

	while (frac >= 1024) {
			s++;
			frac /= 1024;
	}

	GString* buffer = g_string_new("");
	g_string_append_printf(buffer, "%7.2f %3s", frac, suffixes[s]);
	return g_string_free(buffer, FALSE);
}

void dump_coretime(GList* coreTimeStack, CoreTime coreTime)
{
	GList* iter = coreTimeStack;
	for(;iter;iter=g_list_next(iter)) {
		CoreTimeEvent* activeCoreTimeEvent = iter->data;

		//
		// throughput:

		// accumulate average core time
		activeCoreTimeEvent->avgCoreTime.data += coreTime.data;
		activeCoreTimeEvent->avgCoreTime.time += coreTime.time;

		// calculate current throughput
		gdouble currentTp = (coreTime.time? coreTime.data/coreTime.time : 0);

		// calculate active throughput
		CoreTime minCoreTime = activeCoreTimeEvent->minCoreTime;
		CoreTime maxCoreTime = activeCoreTimeEvent->maxCoreTime;
		gdouble activeMinTp = (minCoreTime.time? minCoreTime.data/minCoreTime.time : G_MAXDOUBLE);
		gdouble activeMaxTp = (maxCoreTime.time? maxCoreTime.data/maxCoreTime.time : G_MINDOUBLE);

		// set new active min/max core time from current core time
		if ((currentTp > 0) && (currentTp < activeMinTp))
			activeCoreTimeEvent->minCoreTime = coreTime;
		if (currentTp > activeMaxTp)
			activeCoreTimeEvent->maxCoreTime = coreTime;


		//
		// call time:

		// increase call counter
		activeCoreTimeEvent->numCalls++;

		gdouble currentCt   = coreTime.time;
		gdouble activeMinCt = activeCoreTimeEvent->minCallTime;
		gdouble activeMaxCt = activeCoreTimeEvent->maxCallTime;

		// set new active min/max call time from current core time
		if ((currentCt > 0) && (currentCt < activeMinCt))
			activeCoreTimeEvent->minCallTime = currentCt;
		if (currentCt > activeMaxCt)
			activeCoreTimeEvent->maxCallTime = currentCt;
	}
}

gint compare_time_events(gconstpointer a, gconstpointer b)
{
	TimeEvent* e0 = (TimeEvent*) a;
	TimeEvent* e1 = (TimeEvent*) b;

	if(e0->proc < e1->proc) {
		return -1;
	}
	else if(e0->proc > e1->proc) {
		return 1;
	}
	else {
		if(e0->id < e1->id)
			return -1;
		else if(e0->id > e1->id)
			return 1;
		else
			return 0;
	}
}

gint compare_time_events_full(gconstpointer a, gconstpointer b)
{
	TimeEvent* e0 = (TimeEvent*) a;
	TimeEvent* e1 = (TimeEvent*) b;
	gint cmp = strcmp(e0->name, e1->name);

	// Sort priority:
	// (1) name
	//  (2) proc
	//   (3) id

	if(cmp == -1) {
		return -1;
	}
	else if(cmp == 1) {
		return 1;
	}
	else {
		if(e0->proc < e1->proc) {
			return -1;
		}
		else if(e0->proc > e1->proc) {
			return 1;
		}
		else {
			if(e0->id < e1->id)
				return -1;
			else if(e0->id > e1->id)
				return 1;
			else
				return 0;
		}
	}
}

gint compare_coretime_events(gconstpointer a, gconstpointer b)
{
	CoreTimeEvent* e0 = (CoreTimeEvent*) a;
	CoreTimeEvent* e1 = (CoreTimeEvent*) b;

	if(e0->proc < e1->proc) {
		return -1;
	}
	else if(e0->proc > e1->proc) {
		return 1;
	}
	else {
		if(e0->id < e1->id)
			return -1;
		else if(e0->id > e1->id)
			return 1;
		else
			return 0;
	}
}

gint compare_coretime_events_full(gconstpointer a, gconstpointer b)
{
	CoreTimeEvent* e0 = (CoreTimeEvent*) a;
	CoreTimeEvent* e1 = (CoreTimeEvent*) b;
	gint cmp = strcmp(e0->name, e1->name);

	if(cmp == -1) {
		return -1;
	}
	else if(cmp == 1) {
		return 1;
	}
	else {
		if(e0->proc < e1->proc) {
			return -1;
		}
		else if(e0->proc > e1->proc) {
			return 1;
		}
		else {
			if(e0->id < e1->id)
				return -1;
			else if(e0->id > e1->id)
				return 1;
			else
				return 0;
		}
	}
}
