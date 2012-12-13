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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <glib.h>
#include <glib/gprintf.h>

void _log(const gchar* prefix, const gchar* message, ...)
{
	/* format message */
        gchar buffer[255];

        va_list args;
        va_start(args, message);
        vsprintf(buffer, message, args);
        va_end(args);

        /* get current time of day */
        gchar* time = time_str();

        g_printf("[%d / %s] %s%s\n", rank, time, prefix, buffer);
        g_free(time);
}

void _error(const gchar* message, ...)
{
	 /* format message */
        gchar buffer[255];

        va_list args;
        va_start(args, message);
        vsprintf(buffer, message, args);
        va_end(args);

	_log("Runtime Error: ", buffer);

	#ifdef HAVE_MPI
        	MPI_Abort(MPI_COMM_WORLD, -1);
	#else
	        exit(-1);
	#endif
}

#ifdef HAVE_MPI
void ErrorMPI(gchar* location, gint error_code, gboolean quit)
{
	char error_string[BUFSIZ];
	gint length_of_error_string;//, error_class;

//	MPI_Error_class(error_code, &error_class);
//	MPI_Error_string(error_class, error_string, &length_of_error_string);
//	g_printf("[%d] Runtime error (%s): %s\n", rank, location, error_string);

	MPI_Error_string(error_code, error_string, &length_of_error_string);
	_log("MPI Runtime Error: ", error_string);

	/* get current time of day */
	/*gchar* time = time_str();

	MPI_Error_string(error_code, error_string, &length_of_error_string);
	if (location != NULL)
		g_printf("[%d / %s] MPI Runtime Error (%s): %s\n", rank, time, location, error_string);
	else
		g_printf("[%d / %s] MPI Runtime Error: %s\n", rank, time, error_string);

	g_free(time);*/

	if (quit) {
		MPI_Abort(MPI_COMM_WORLD, error_code);
		exit(-1);
	}
}
#endif

gchar* time_str()
{
	/* get current time of day */
	GString* timeBuffer = g_string_new("");
	gint time_hh, time_mm, time_ss, time_ms;
	GTimeVal timeval;
	g_get_current_time(&timeval);
	time_hh=1+(timeval.tv_sec/3600)%24;
	time_mm=(timeval.tv_sec/60)%60;
	time_ss=(timeval.tv_sec)%60;
	time_ms=(timeval.tv_usec)/1000;
	g_string_append_printf(timeBuffer, "%02d:%02d:%02d.%03d", time_hh, time_mm, time_ss, time_ms);
	return g_string_free(timeBuffer, FALSE);
}

gchar* date_str()
{
	/* get current time of day */
	//GTimeVal timeval;
	//g_get_current_time(&timeval);

	/* get current date */
	GString* dateBuffer = g_string_new("");
	GDate* date = g_date_new();

	g_date_set_time(date, time(NULL));
	//g_date_set_time_val(date, &timeval); // Not available in glib 2.8

	gint day = g_date_get_day(date);
	gint month = g_date_get_month(date);
	gint year = g_date_get_year(date);
	g_string_append_printf(dateBuffer, "%02d.%02d.%04d", day, month, year);
	g_date_free(date);
	return g_string_free(dateBuffer, FALSE);
}
