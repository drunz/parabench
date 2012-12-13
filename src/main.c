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
#include "timing.h"
#include "interpreter.h"
#include "iio_posix.h"
#include "groups.h"
#include "xml.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <glib.h>

gboolean export = FALSE;
gboolean silent = FALSE;
gboolean clean = FALSE;
gboolean parseOnly = FALSE;
gboolean version = FALSE;
gboolean agileMode = FALSE;
gboolean waitForStartSignal = FALSE;

gchar* sourceFileName;

static gboolean group_cb(const gchar* option_name, const gchar* value, gpointer data, GError** error)
{
	gchar* offset;
	gchar* name;
	gint size;

	offset = strchr(value, ':');

	if (offset == NULL) {
		name = g_strdup(value);
		size = 0;
	} else {
		name = g_strndup(value, offset-value);
		sscanf(offset+1, "%d", &size);
	}

	g_print("%s:%d\n", name, size);

#ifdef HAVE_MPI
	g_hash_table_insert(sizeGroupmap, name, GINT_TO_POINTER(size));
#endif

	return TRUE;
}

static GOptionEntry entries[] =
{
	{ "version", 'v', 0, G_OPTION_ARG_NONE, &version, "Shows the version of this program", NULL },
	{ "export", 'e', 0, G_OPTION_ARG_NONE, &export, "Export timing results to XML", NULL },
	{ "silent", 's', 0, G_OPTION_ARG_NONE, &silent, "Same as '-f' but will suppress printing reports to stdout", NULL },
	{ "clean", 'c', 0, G_OPTION_ARG_NONE, &clean, "Remove all data created during benchmark", NULL },
	{ "dry-run", 'd', 0, G_OPTION_ARG_NONE, &parseOnly, "Don't do any I/O calls", NULL },
	{ "agile", 'a', 0, G_OPTION_ARG_NONE, &agileMode, "Toggles agile mode where sleeps will be skipped", NULL },
	{ "wait", 'w', 0, G_OPTION_ARG_NONE, &waitForStartSignal, "Wait for signal SIGUSR1 after parsing is done", NULL },
	{ "group", 'g', 0, G_OPTION_ARG_CALLBACK, group_cb, "Set number of processes to SIZE from group NAME. Default value of SIZE is 0 if group size is not set on command line. Read the manual for mapping tags.", "NAME[:SIZE]" },
	{ NULL }
};

void parse_parameters(int* argc, char*** argv) {
	GError* error = NULL;
	GOptionContext* context;

	context = g_option_context_new(NULL);
	g_option_context_add_main_entries(context, entries, NULL);

	if (!g_option_context_parse(context, argc, argv, &error))
	{
		g_print("option parsing failed: %s\n", error->message);
		exit(1);
	}

	g_option_context_free(context);
}


#ifdef HAVE_MPI
void create_mpitype_timeevent() { 
	MPI_Datatype type[3] = {MPI_INT, MPI_DOUBLE, MPI_CHAR};
	int          blocklen[3] = {2, 1, NAME_SIZE};
	MPI_Aint	 disp[3];
	MPI_Aint	 extent;
	
	/* Setup description of int */
	disp[0] = 0;
	/* Setup description of double */
	MPI_Type_extent(MPI_INT, &extent);
	disp[1] = 2 * extent;
	/* Setup description of char */
	MPI_Type_extent(MPI_DOUBLE, &extent);
	disp[2] = disp[1] + 1 * extent;
	
	MPI_Type_struct(3, blocklen, disp, type, &timeevent_type); 
	MPI_Type_commit(&timeevent_type);
}

void create_mpitype_coretimeevent() {
	MPI_Datatype type[4] = {MPI_INT, MPI_DOUBLE, MPI_LONG, MPI_CHAR};
	int          blocklen[4] = {2, 1, 1, NAME_SIZE};
	MPI_Aint	 disp[4];
	MPI_Aint	 extent;

	/* Setup description of int */
	disp[0] = 0;
	/* Setup description of double */
	MPI_Type_extent(MPI_INT, &extent);
	disp[1] = 2 * extent;
	/* Setup description of long */
	MPI_Type_extent(MPI_DOUBLE, &extent);
	disp[2] = disp[1] + 1 * extent;
	/* Setup description of char */
	MPI_Type_extent(MPI_LONG, &extent);
	disp[3] = disp[2] + 1 * extent;

	MPI_Type_struct(4, blocklen, disp, type, &coretimeevent_type);
	MPI_Type_commit(&coretimeevent_type);
}

void gather_timeevents() {
	// TODO: MPI_Gatherv
	int i, j;
	MPI_Status stat;
	TimeEvent* buf;
	int num;
	GSList *iter;
	
	if(rank == MASTER) {
		for(i=1; i<size; i++) {
			MPI_Recv(&num, 1, MPI_INT, i, 1, MPI_COMM_WORLD, &stat);
			//printf("received num = %d from rank %d\n", num, i);
			
			for(j=0; j<num; j++) {
				buf = (TimeEvent*) malloc(sizeof(TimeEvent));
				
				MPI_Recv(buf, 1, timeevent_type, i, 2, MPI_COMM_WORLD, &stat);
				//printf("TimeEvent(%d, %d, %s, %f)\n", bufr->proc, bufr->id, bufr->name, bufr->value);
				timeList = g_slist_prepend(timeList, buf);
			}
		}
	}
	else {
		num = g_slist_length(timeList);
		MPI_Send(&num, 1, MPI_INT, MASTER, 1, MPI_COMM_WORLD);
		
		iter = timeList;
		for(;iter;iter=g_slist_next(iter)) {
			buf = (TimeEvent*) iter->data;
			MPI_Send(buf, 1, timeevent_type, MASTER, 2, MPI_COMM_WORLD);
		}
	}
}

void gather_coretimeevents() {
	// TODO: MPI_Gatherv
	int i, j;
	MPI_Status stat;
	CoreTimeEvent* buf;
	int num;
	GSList *iter;

	if(rank == MASTER) {
		for(i=1; i<size; i++) {
			MPI_Recv(&num, 1, MPI_INT, i, 1, MPI_COMM_WORLD, &stat);
			//printf("received num = %d from rank %d\n", num, i);

			for(j=0; j<num; j++) {
				buf = (CoreTimeEvent*) malloc(sizeof(CoreTimeEvent));

				MPI_Recv(buf, 1, timeevent_type, i, 2, MPI_COMM_WORLD, &stat);
				//printf("CoreTimeEvent(%d, %d, %s, %f, %ld)\n", buf->proc, buf->id,
				//		buf->name, buf->coreTime.time, buf->coreTime.data);
				coreTimeList = g_slist_prepend(coreTimeList, buf);
			}
		}
	}
	else {
		num = g_slist_length(coreTimeList);
		MPI_Send(&num, 1, MPI_INT, MASTER, 1, MPI_COMM_WORLD);

		iter = coreTimeList;
		for(;iter;iter=g_slist_next(iter)) {
			buf = (CoreTimeEvent*) iter->data;
			MPI_Send(buf, 1, timeevent_type, MASTER, 2, MPI_COMM_WORLD);
		}
	}
}

void gather_commandstats() {
	// TODO: MPI_Gather
	int i, j;
	MPI_Status stat;
	int sSucceed[NUM_TRAC_STATEMENTS];
	int sFail[NUM_TRAC_STATEMENTS];
	
	if(rank == MASTER) {
		for(i=1; i<size; i++) {
			MPI_Recv(sSucceed, NUM_TRAC_STATEMENTS, MPI_INT, i, 3, MPI_COMM_WORLD, &stat);
			MPI_Recv(sFail, NUM_TRAC_STATEMENTS, MPI_INT, i, 3, MPI_COMM_WORLD, &stat);
			
			for(j=0; j<NUM_TRAC_STATEMENTS; j++) {
				statementsSucceed[j] += sSucceed[j];
				statementsFail[j] += sFail[j];
			}
		}
	}
	else {
		MPI_Send(statementsSucceed, NUM_TRAC_STATEMENTS, MPI_INT, MASTER, 3, MPI_COMM_WORLD);
		MPI_Send(statementsFail, NUM_TRAC_STATEMENTS, MPI_INT, MASTER, 3, MPI_COMM_WORLD);
	}
}
#endif

void export_time_csv() {
	GSList *list = g_slist_copy(timeList);
	list = g_slist_sort(list, compare_time_events_full);

	iio_mkdir("./results");
		
	GSList *iter = list;
	gchar filename[NAME_SIZE+19];
	gchar *lastname = NULL;
	int cmp;
	FILE *fh = NULL;
	for(;iter;iter=g_slist_next(iter)) {
		TimeEvent* event = (TimeEvent*) iter->data;
		
		if((cmp = strcmp(event->name, lastname)) != 0) {
			// We need to open a new file, close the previous
			if(fh != NULL) {
				fclose(fh);
			}
		
			sprintf(filename, "./results/time_%s.txt", event->name);
			fh = g_fopen(filename, "w");
			
			if(fh == NULL) {
				g_printf("I couldn't open %s for writing.\n", filename);
				continue;
			}
		}
		
		// Format: <rank>;<event_id>;<time_value>
		g_fprintf(fh, "%d;%d;%.6f\n", event->proc, event->id, event->value);
		lastname = event->name;
	}
	
	// close the last handle
	if(fh != NULL) {
		fclose(fh);
	}
}

void export_coretime_csv() {
	GSList *list = g_slist_copy(coreTimeList);
	list = g_slist_sort(list, compare_coretime_events_full);

	iio_mkdir("./results_ct");

	GSList *iter = list;
	gchar filename[NAME_SIZE+19];
	gchar *lastname = NULL;
	int cmp;
	FILE *fh = NULL;
	for (;iter;iter=g_slist_next(iter)) {
		CoreTimeEvent* event = iter->data;

		if ((cmp = strcmp(event->name, lastname)) != 0) {
			// We need to open a new file, close the previous
			if (fh != NULL) {
				fclose(fh);
			}

			sprintf(filename, "./results_ct/ctime_%s.txt", event->name);
			fh = g_fopen(filename, "w");

			if (fh == NULL) {
				g_printf("I couldn't open %s for writing.\n", filename);
				continue;
			}
		}

		CoreTime avgCoreTime = event->avgCoreTime;
		CoreTime minCoreTime = event->minCoreTime;
		CoreTime maxCoreTime = event->maxCoreTime;
		gdouble avgTP = (avgCoreTime.time? avgCoreTime.data/avgCoreTime.time : 0);
		gdouble minTP = (minCoreTime.time? minCoreTime.data/minCoreTime.time : 0);
		gdouble maxTP = (maxCoreTime.time? maxCoreTime.data/maxCoreTime.time : 0);
		gdouble avgTime = (event->numCalls? event->avgCoreTime.time/event->numCalls : 0);
		gdouble minTime = event->minCallTime;
		gdouble maxTime = event->maxCallTime;

		// Format: <rank>;<event_id>;<time_value>
		g_fprintf(fh, "%d\t%d\t%f\t%f\t%f\t%f\t%f\t%f\n", event->proc, event->id, avgTP, minTP, maxTP, avgTime, minTime, maxTime);
		lastname = event->name;
	}

	// close the last handle
	if(fh != NULL) {
		fclose(fh);
	}
}

void export_xml(const gchar* kernelName) {
	gchar* date = date_str();
	gchar* time = time_str();

	XmlDocument* doc = xml_document_new();
	xml_start_element(doc, "Report");
	xml_add_attribute_string(doc, "date", date);
	xml_add_attribute_string(doc, "time", time);
	xml_add_attribute_int(doc, "size", size);
	xml_add_attribute_string(doc, "kernel", kernelName);

	g_free(date);
	g_free(time);


	/* write core time events */
	GSList* list = g_slist_copy(coreTimeList);
	list = g_slist_sort(list, compare_coretime_events_full);
	GSList *iter = list;

	xml_start_element(doc, "EventList");
	xml_add_attribute_string(doc, "type", "CoreTime");

	for (;iter;iter=g_slist_next(iter)) {
		CoreTimeEvent* event = iter->data;

		CoreTime avgCoreTime = event->avgCoreTime;
		CoreTime minCoreTime = event->minCoreTime;
		CoreTime maxCoreTime = event->maxCoreTime;
		gdouble avgTP = (avgCoreTime.time? avgCoreTime.data/avgCoreTime.time : 0);
		gdouble minTP = (minCoreTime.time? minCoreTime.data/minCoreTime.time : 0);
		gdouble maxTP = (maxCoreTime.time? maxCoreTime.data/maxCoreTime.time : 0);
		gdouble avgTime = (event->numCalls? event->avgCoreTime.time/event->numCalls : 0);
		gdouble minTime = event->minCallTime;
		gdouble maxTime = event->maxCallTime;
		glong ioops = (event->numCalls>0? event->numCalls/event->avgCoreTime.time : 0);

		/* write xml */
		xml_start_element(doc, "Event");
		xml_add_attribute_int(doc, "rank", event->proc);
		xml_add_attribute_int(doc, "id", event->id);
		xml_add_attribute_string(doc, "name", event->name);

		xml_start_element(doc, "Throughput");
		xml_add_attribute_double(doc, "avg", avgTP);
		xml_add_attribute_double(doc, "min", minTP);
		xml_add_attribute_double(doc, "max", maxTP);
		xml_end_element(doc);

		xml_start_element(doc, "Calltime");
		xml_add_attribute_double(doc, "avg", avgTime);
		xml_add_attribute_double(doc, "min", minTime);
		xml_add_attribute_double(doc, "max", maxTime);
		xml_end_element(doc);

		xml_start_element(doc, "Requests");
		xml_add_attribute_long(doc, "num", event->numCalls);
		xml_add_attribute_double(doc, "time", avgCoreTime.time);
		xml_add_attribute_long(doc, "ioops", ioops);
		xml_end_element(doc);

		xml_end_element(doc); // <Event>
	}
	xml_end_element(doc); // <EventList>
	g_slist_free(list);


	/* write time events */
	list = g_slist_copy(timeList);
	list = g_slist_sort(list, compare_time_events_full);
	iter = list;

	xml_start_element(doc, "EventList");
	xml_add_attribute_string(doc, "type", "Time");

	for (;iter;iter=g_slist_next(iter)) {
		TimeEvent* event = iter->data;

		/* write xml */
		xml_start_element(doc, "Event");
		xml_add_attribute_int(doc, "rank", event->proc);
		xml_add_attribute_int(doc, "id", event->id);
		xml_add_attribute_string(doc, "name", event->name);

		xml_start_element(doc, "Walltime");
		xml_add_attribute_double(doc, "value", event->value);
		xml_end_element(doc);

		xml_end_element(doc); // <Event>
	}
	xml_end_element(doc); // <EventList>
	g_slist_free(list);


	xml_end_element(doc); // <Report>

	xml_document_save(doc, "results.xml");
	xml_document_free(doc);
}

static void quit()
{
#ifdef HAVE_MPI
		MPI_Finalize();
#endif
		exit(0);
}

void sigfunc(int sig)
{
	if(sig == SIGUSR1) {
		Log("Start signal received, notifying world...");
		waitForStartSignal = FALSE;
	}
}

void delayStart()
{
	if (rank == MASTER) {
		Log("Master is waiting for signal SIGUSR1 to start...");
		while (waitForStartSignal) sleep(1);
	}
#ifdef HAVE_MPI
	MPI_Bcast(&waitForStartSignal, 1, MPI_INT, MASTER, MPI_COMM_WORLD);
	Log("Collective start initialized...");
	MPI_Barrier(MPI_COMM_WORLD);
#endif
}

int main(int argc, char **argv) {
	GTimer* timer = g_timer_new();
	gdouble setupTime, parserTime, interpreterTime, finalizeTime;
	
#ifdef HAVE_MPI
	// init mpi
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	
	MPI_Info_create(&info);
	create_mpitype_timeevent();

        patterns_init();

	MPI_Barrier(MPI_COMM_WORLD);
#else
	rank = 0;
	size = 1;
#endif
	
	// read parameters and init interpreter
#ifdef HAVE_MPI
	groups_init();//sizeGroupmap = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, & g_free);
#endif
	parse_parameters(&argc, &argv);

	if (waitForStartSignal && (rank == MASTER))
		signal(SIGUSR1, sigfunc);
	if (waitForStartSignal && (rank != MASTER))
		signal(SIGUSR1, SIG_IGN);
	
	if (version) {
		if(rank == MASTER) {
			printf("ParaBench v%s\n\n", VERSION);
		}
		quit();
	}
	
	if(argc < 2) {
		if(rank == MASTER) {
			printf("Invalid parameters - specify file!\n");
		}
		quit();
	}
	
	sourceFileName = argv[1];
	FILE *file = fopen(argv[1], "r");
	iiInit(file);
		
	if(file == NULL) {
		printf("[%d] File %s doesn't exist!\n", rank, argv[argc-1]);
		quit();
	}
	
#ifdef HAVE_MPI
	MPI_Barrier(MPI_COMM_WORLD);
#endif
	
	setupTime = g_timer_elapsed(timer, NULL);
	g_timer_start(timer);

	// run parser
	iiParse();
	
	parserTime = g_timer_elapsed(timer, NULL);

	fclose(file);
	
	iiSetParameters(argc - 2, & argv[2]);
	
	// wait for signal SIGUSR1
	if (waitForStartSignal)
		delayStart();

	g_timer_start(timer);

	iiStart();
	
	interpreterTime = g_timer_elapsed(timer, NULL);
	g_timer_start(timer);

#ifdef HAVE_MPI
	MPI_Barrier(MPI_COMM_WORLD);
	
	gather_timeevents();
	gather_coretimeevents();
	gather_commandstats();
#endif
	
	if(rank == MASTER) {
		if(!silent) {
			iiTimeReport();
			iiCoreTimeReport();
			iiCommandReport();
		}
		
		if(!parseOnly && (export || silent))
			export_xml(argv[1]);
	}
	
	if(!parseOnly && clean)
		clean_created_data();
	
#ifdef HAVE_MPI
	MPI_Barrier(MPI_COMM_WORLD);
	
	MPI_Type_free(&timeevent_type);
	MPI_Info_free(&info);

	MPI_Finalize();
#endif
	
	iiFree();
#ifdef HAVE_MPI
	groups_free();
#endif

	finalizeTime = g_timer_elapsed(timer, NULL);
	g_timer_destroy(timer);

	// runtime summary
	if(rank == MASTER) {
		gdouble globalTime = setupTime+parserTime+interpreterTime+finalizeTime;
		//g_printf("\n********************* Runtime *********************\n");
		g_printf("\n\n");
		g_printf("Runtime:      %14.6fs\n", globalTime);
		g_printf("Setup:        %14.6fs    %5.1f%%\n", setupTime, setupTime/globalTime*100);
		g_printf("Parser:       %14.6fs    %5.1f%%\n", parserTime, parserTime/globalTime*100);
		g_printf("Interpreter:  %14.6fs    %5.1f%%\n", interpreterTime, interpreterTime/globalTime*100);
		g_printf("Finalize:     %14.6fs    %5.1f%%\n", finalizeTime, finalizeTime/globalTime*100);
	}

	return 0;
}
