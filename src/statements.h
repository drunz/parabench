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

#ifndef STATEMENTS_H_
#define STATEMENTS_H_

#include "parameters.h"

typedef enum {
    /* POSIX I/O Statements */
    STMT_FOPEN,   STMT_FCLOSE,
    STMT_FWRITE,  STMT_FREAD,
    STMT_WRITE,   STMT_APPEND,
    STMT_READ,    STMT_LOOKUP,
    STMT_DELETE,  STMT_MKDIR,
    STMT_RMDIR,   STMT_CREATE,
    STMT_STAT,    STMT_RENAME,
    STMT_FSEEK,   STMT_FCREAT,
    STMT_FSYNC,

    /* MPI I/O Statements */
    STMT_PFOPEN,  STMT_PFCLOSE,
    STMT_PFWRITE, STMT_PFREAD,
    STMT_PWRITE,  STMT_PREAD, //19
    STMT_PDELETE,

    /* Module Statements */
    /* ![ModuleHook] statement_enum */

    __STMT_NUM_REPORTED__, // Statements after this entry wont be reported

    /* Auxiliary Statements */
    STMT_REPEAT,  STMT_TIME,
    STMT_CTIME,   STMT_DEFINE,
    STMT_ASSIGN,  STMT_GROUP,
    STMT_MASTER,  STMT_BARRIER,
    STMT_SLEEP,   STMT_PRINT,
    STMT_BLOCK,
} StatementType;

typedef struct {
    StatementType type;
    ParameterList* parameters;
    gchar* label;
    guint line; // line in PPL file
} Statement;


Statement* stmt_new(StatementType  type, ParameterList* parameters, gchar* label, guint line);
void       stmt_free(Statement* statement);

gchar* stmt_get_string(StatementType type);

#endif /* STATEMENTS_H_ */
