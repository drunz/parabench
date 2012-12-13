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

#ifndef TIMING_H_
#define TIMING_H_

#include <glib.h>

#define NAME_SIZE 255		// the size of the time event name strings

#define CORETIME_START() GTimer* _coreTimer = g_timer_new();
#define CORETIME_STOP(t) g_timer_stop(_coreTimer); \
                         gdouble (t) = g_timer_elapsed(_coreTimer, NULL); \
	                     g_timer_destroy(_coreTimer); \

#ifdef HAVE_MPI
MPI_Datatype timeevent_type;
MPI_Datatype coretimeevent_type;
#endif

GList*  coreTimeStack;

GSList* timeList;			// list with completed time events
GSList* coreTimeList;		// list with completed core time events

typedef struct {
	gint proc;				// process id
	gint id;				// used to keep track of global command start order
	gdouble value;			// time value
	gchar name[NAME_SIZE];	// name of the time event
} TimeEvent;

typedef struct {
	gdouble time;			// duration of I/O function core
	glong   data;			// data processed in I/O function core
} CoreTime;

typedef struct {
	gint proc;				// process id
	gint id;				// used to keep track of global command start order
	CoreTime avgCoreTime;	// average core time
	CoreTime minCoreTime;	// min core time
	CoreTime maxCoreTime;	// max core time
	glong numCalls;		// number of I/O calls
	// get average call time from: avgCoreTime.time / numCalls
	gdouble minCallTime;	// min raw I/O call time
	gdouble maxCallTime;	// max raw I/O call time
	gchar name[NAME_SIZE];	// name of the time event
} CoreTimeEvent;


void timing_init();
void timing_free();

TimeEvent*		timeevent_new(gint id, const gchar* name, gdouble value);
CoreTimeEvent*	coretime_event_new(gint id, const gchar* name, CoreTime coreTime);
CoreTime		coretime_new(gdouble time, glong data);

gchar* format_coretime_throughput(CoreTime coreTime);
gchar* format_data_size(glong dataSize);
void   dump_coretime(GList* coreTimeStack, CoreTime coreTime);

gint compare_time_events(gconstpointer a, gconstpointer b);
gint compare_time_events_full(gconstpointer a, gconstpointer b);
gint compare_coretime_events(gconstpointer a, gconstpointer b);
gint compare_coretime_events_full(gconstpointer a, gconstpointer b);

#endif /* TIMING_H_ */
