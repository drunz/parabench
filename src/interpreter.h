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

#ifndef INTERPRETER_H_
#define INTERPRETER_H_

#include "config.h"
#ifdef HAVE_MPI
  #include <mpi.h>
#endif
#include "timing.h"
#include "ast.h"
#include "statements.h"
#include "parameters.h"
#include "expressions.h"
#include "patterns.h"
#include "groups.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>


#define	NUM_TRAC_STATEMENTS __STMT_NUM_REPORTED__ // number of commands that will be tracked for succeed/fail (first n commands of CmdType)
#define ERROR g_printf("Runtime Error (%s:%d) ", __FILE__, __LINE__); iiError


typedef struct {
	gchar* param_name;
	gchar* var_name;
	gchar* default_value;
} param_list_entry;

typedef struct {
	gchar* env_name;
	gchar* env_value;
} env_list_entry;

/* Extended Functionality Data Structures */
GSList*     paramList;    // list with existing parameters
GSList*     fileList;     // list with created files (strings of filenames)
GSList*     dirList;      // list with created dirs (strings of dirnames)


// Important command line arguments
extern gboolean parseOnly;
extern gboolean agileMode;

gint statementsSucceed[NUM_TRAC_STATEMENTS]; // number of successful operations
gint statementsFail[NUM_TRAC_STATEMENTS];    // number of failed operations


//
// Control Functions
//
void iiInit(FILE* file);
void iiFree();
void iiParse();
void iiSetParameters(gint argc, gchar** argv);
void iiStart();
void iiTimeReport();
void iiCoreTimeReport();
void iiCommandReport();


//
// Helper Functions
//
void clean_created_data();
gint getCollectiveRandomNumber();


#endif /*INTERPRETER_H_*/
