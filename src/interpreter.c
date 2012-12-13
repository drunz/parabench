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
#include "interpreter.h"
#include "iio.h"
#include "iio_posix.h"
#include "iio_mpi.h"
#include "errtrace.h"

/* Third party modules */
/* ![ModuleHook] module_include */

void yyset_in(FILE *in_str);
int yyparse();


//
// Interpreter control functions
//

void iiInit(FILE* file)
{
	yyset_in(file);

	gint i;
	for(i=0; i < NUM_TRAC_STATEMENTS; i++) {
		statementsSucceed[i] = 0;
		statementsFail[i] = 0;
	}
	
	fileList = NULL;
	dirList = NULL;
	
	timing_init();
	ast_init();
	var_init();
	//groups_init();
}

void iiFree()
{
	if (fileList) {
		g_slist_foreach(fileList, (GFunc) g_free, NULL); //1
		g_slist_free(fileList);
	}
	
	if (dirList) {
		g_slist_foreach(dirList, (GFunc) g_free, NULL); //1
		g_slist_free(dirList);
	}
	
	if (paramList) {
		g_slist_foreach(paramList, (GFunc) g_free, NULL);
		g_slist_free(paramList);
	}

	timing_free();
	ast_free();
	var_free();
	//groups_free();
}

static void ExecuteStatement(GNode* node, gpointer data)
{
	Statement* stmt = (Statement*) node->data;
	Verbose("* Visiting GNode at level %d", g_node_depth(node));

	if (parseOnly && (stmt->type < STMT_REPEAT)) return;

	switch (stmt->type) {
		case STMT_ASSIGN: {
			ExpressionStatus status[2];
			ParameterList* paramList = stmt->parameters;
			gchar* varident = param_string_get(paramList, 0, &status[0]);
			Expression* expression = param_index_get(paramList, 1);

			// evaluator error check
			if (status[0] != STATUS_EVAL_OK) {
				Error("Malicious assign parameters! (status = %s)", expr_status_to_string(status[0]));
			}

			switch (expression->type) {
				case EXPR_RICH_INT:
				case EXPR_CONSTANT_INT: {
					Verbose("~ Executing STMT_ASSIGN: variable = %s, type = INT", varident);
					glong result = param_int_get(paramList, 1, &status[1]);

					// evaluator error check
					if (status[1] != STATUS_EVAL_OK) {
						Error("Malicious assign parameters! (status = %s)", expr_status_to_string(status[1]));
					}

					var_set_value(varident, VAR_INT, &result);
					break;
				}

				case EXPR_RICH_STRING:
				case EXPR_CONSTANT_STRING: {
					Verbose("~ Executing STMT_ASSIGN: variable = %s, type = STRING", varident);
					gchar* result_raw = param_string_get(paramList, 1, &status[1]);

					// evaluator error check
					if (status[1] != STATUS_EVAL_OK) {
						Error("Malicious assign parameters! (status = %s)", expr_status_to_string(status[1]));
					}

					gchar* result = var_replace_substrings(result_raw);

					var_set_value(varident, VAR_STRING, result);
					g_free(result_raw);
					g_free(result);
					break;
				}

				default: Error("Expression type %d not supported in assign statement!\n", expression->type);
			}

			g_free(varident);
			break;
		}

		case STMT_REPEAT: {
			ExpressionStatus status[2];
			ParameterList* paramList = stmt->parameters;
			gchar* varident = param_string_get(paramList, 0, &status[0]);
			glong i, loopCount = param_int_get(paramList, 1, &status[1]);

			Verbose("~ Executing STMT_REPEAT: variable = %s, loopCount = %ld", varident, loopCount);

			g_assert(loopCount >= 0);

			// evaluator error check
			if (!expr_status_assert(status, 2)) {
				backtrace(stmt);
				Error("Malicious repeat parameters! (%s:%d)", __FILE__, __LINE__);
			}

			for (i=0; i<loopCount; i++) {
				var_set_value(varident, VAR_INT, &i);
				g_node_children_foreach(node, G_TRAVERSE_ALL, &ExecuteStatement, NULL);
			}

			var_destroy(varident);
			g_free(varident);
			break;
		}

		case STMT_TIME: {
			Verbose("~ Executing STMT_TIME: label = %s", stmt->label);

			static gint timeId = 0;
			gdouble time;
			GTimer* timer = g_timer_new();

			g_node_children_foreach(node, G_TRAVERSE_ALL, &ExecuteStatement, NULL);

			g_timer_stop(timer);
			time = g_timer_elapsed(timer, NULL);
			g_timer_destroy(timer);

			gchar* label = var_replace_substrings(stmt->label);
			timeList = g_slist_prepend(timeList, timeevent_new(timeId++, label, time));
			g_free(label);
			break;
		}

		case STMT_CTIME: {
			Verbose("~ Executing STMT_CTIME: label = %s", stmt->label);

			static gint coreTimeId = 0;
			gchar* label = var_replace_substrings(stmt->label);
			CoreTimeEvent* coreTimeEvent = coretime_event_new(coreTimeId++, label, coretime_new(0, 0));
			coreTimeStack = g_list_prepend(coreTimeStack, coreTimeEvent);

			g_node_children_foreach(node, G_TRAVERSE_ALL, &ExecuteStatement, NULL);

			coreTimeList = g_slist_prepend(coreTimeList, coreTimeEvent);
			coreTimeStack = g_list_remove_link(coreTimeStack, g_list_first(coreTimeStack));
			g_free(label);
			break;
		}

#ifdef HAVE_MPI
		case STMT_GROUP: {
			Verbose("~ Executing STMT_GROUP: group = %s", stmt->label);

			GroupBlock* groupBlock = g_hash_table_lookup(groupMap, stmt->label);

			if(groupBlock && groupBlock->member) {
				groupStack = g_list_prepend(groupStack, groupBlock);
				g_node_children_foreach(node, G_TRAVERSE_ALL, &ExecuteStatement, NULL);
				groupStack = g_list_remove_link(groupStack, g_list_first(groupStack));
			}
			else if(!groupBlock) {
				backtrace(stmt);
				Error("Group \"%s\" doesn't exist!", stmt->label);
			}
			break;
		}

		case STMT_MASTER: {
			GroupBlock* groupBlock;
			MPI_Comm comm = MPI_COMM_WORLD;
			if(groupStack) {
				groupBlock = (GroupBlock*) g_list_first(groupStack)->data;
				comm = groupBlock->mpicomm;
			}

			gint groupRank;
			MPI_ASSERT(MPI_Comm_rank(comm, &groupRank), "Master Statement", TRUE)

			if(groupStack) Verbose("~ Executing STMT_MASTER: rank = %d, type = implicit group", groupRank);
			else           Verbose("~ Executing STMT_MASTER: rank = %d, type = world", groupRank);

			if(groupRank == MASTER) {
				g_node_children_foreach(node, G_TRAVERSE_ALL, &ExecuteStatement, NULL);
			}
			else Verbose("Im not the master here... groupRank = %d", groupRank);
			break;
		}

		case STMT_BARRIER: {
			ExpressionStatus status[1];
			ParameterList* paramList = stmt->parameters;
			gchar* groupName = param_string_get_optional(paramList, 0, &status[0], NULL);

			// evaluator error check
			if (!expr_status_assert(status, 1)) {
				backtrace(stmt);
				Error("Malicious statement parameters!");
			}

			GroupBlock* groupBlock = groupblock_get(groupName);

			if(groupName)  Verbose("~ Executing STMT_BARRIER: type = explicit group, name = %s", groupName);
			else           Verbose("~ Executing STMT_BARRIER: type = implicit active group");

			MPI_ASSERT(MPI_Barrier(groupBlock->mpicomm), "Barrier Statement", TRUE)

			g_free(groupName);
			break;
		}
#endif

		case STMT_SLEEP: {
			if (agileMode) return;
			Verbose("~ Executing STMT_SLEEP");

			ExpressionStatus status[1];
			ParameterList* paramList = stmt->parameters;
			glong time = param_int_get(paramList, 0, &status[0]);

			// evaluator error check
			if (!expr_status_assert(status, 1)) {
				backtrace(stmt);
				Error("Malicious statement parameters!");
			}

			g_usleep(time);
			//sleep(time);
			break;
		}

		case STMT_BLOCK: {
			Verbose("~ Executing STMT_BLOCK");
			g_node_children_foreach(node, G_TRAVERSE_ALL, &ExecuteStatement, NULL);
			break;
		}

		case STMT_PRINT: {
			Verbose("~ Executing STMT_PRINT");

			ParameterList* paramList = stmt->parameters;
			GString* buffer = g_string_new("");
			ExpressionStatus status;
			gint i;

			for (i=0; i<paramList->len; i++) {
				gchar* string = param_string_get(paramList, i, &status);
				if (status == STATUS_EVAL_OK) {
					g_string_append(buffer, string);
					g_free(string);
					if (i < (paramList->len-1))
						g_string_append(buffer, " ");
				}
				else {
					backtrace(stmt);
					Error("Error during print parameter evaluation (index=%d, status=%s).\n",
							i, expr_status_to_string(status));
				}
			}

			gchar* processed = var_replace_substrings(buffer->str);
			g_printf("[%d] %s\n", rank, processed);
			g_free(processed);
			g_string_free(buffer, TRUE);
			break;
		}

		case STMT_FCREAT: {
			ExpressionStatus status[2];
			ParameterList* paramList = stmt->parameters;
			gchar* fhname = param_string_get(paramList, 0, &status[0]);
			gchar* fname_raw = param_string_get(paramList, 1, &status[1]);

			Verbose("~ Executing STMT_FCREAT: fhname = %s, fname = %s", fhname, fname_raw);

			// evaluator error check
			if (!expr_status_assert(status, 2)) {
				backtrace(stmt);
				Error("Malicious statement parameters!");
			}

			gchar* fname = var_replace_substrings(fname_raw);

			File* file;
			IOStatus ioStatus = iio_fcreat(fname, &file);
			dump_coretime(coreTimeStack, ioStatus.coreTime);

			if (ioStatus.success) {
				Verbose("  > file = %p", file);
				var_set_value(fhname, VAR_FILE, &file);
				statementsSucceed[STMT_FCREAT]++;
			}
			else
				statementsFail[STMT_FCREAT]++;

			g_free(fhname);
			g_free(fname_raw);
			g_free(fname);
			break;
		}

		case STMT_FOPEN: {
			ExpressionStatus status[3];
			ParameterList* paramList = stmt->parameters;
			gchar* fhname = param_string_get(paramList, 0, &status[0]);
			gchar* fname_raw = param_string_get(paramList, 1, &status[1]);
			gint   flags = param_int_get(paramList, 2, &status[2]);

			Verbose("~ Executing STMT_FOPEN: fhname = %s, fname = %s, flags = %d", fhname, fname_raw, flags);

			// evaluator error check
			if (!expr_status_assert(status, 3)) {
				backtrace(stmt);
				Error("Malicious statement parameters!");
			}

			gchar* fname = var_replace_substrings(fname_raw);

			File* file;
			IOStatus ioStatus = iio_fopen(fname, flags, &file);
			dump_coretime(coreTimeStack, ioStatus.coreTime);

			if (ioStatus.success) {
				Verbose("  > file = %p", file);
				var_set_value(fhname, VAR_FILE, &file);
				statementsSucceed[STMT_FOPEN]++;
			}
			else
				statementsFail[STMT_FOPEN]++;

			g_free(fhname);
			g_free(fname_raw);
			g_free(fname);
			break;
		}

		case STMT_FCLOSE: {
			ExpressionStatus status[1];
			ParameterList* paramList = stmt->parameters;
			File* file = param_file_get(paramList, 0, &status[0]);

			Verbose("~ Executing STMT_FCLOSE: file = %p", file);

			// evaluator error check
			if (!expr_status_assert(status, 1)) {
				backtrace(stmt);
				Error("Malicious statement parameters!");
			}

			g_assert(file);

			IOStatus ioStatus = iio_fclose(file);
			dump_coretime(coreTimeStack, ioStatus.coreTime);

			if (ioStatus.success) {
				gchar* fhname = (gchar*) param_value_get(paramList, 0);
				var_destroy(fhname);
				statementsSucceed[STMT_FCLOSE]++;
			}
			else
				statementsFail[STMT_FCLOSE]++;
			break;
		}

		case STMT_FREAD: {
			ExpressionStatus status[3];
			ParameterList* paramList = stmt->parameters;
			File* file = param_file_get(paramList, 0, &status[0]);
			glong dataSize = param_int_get_optional(paramList, 1, &status[1], READALL);
			glong offset = param_int_get_optional(paramList, 2, &status[2], OFFSET_CUR);

			Verbose("~ Executing STMT_FREAD: file = %p, dataSize = %ld, offset = %ld", file, dataSize, offset);

			// evaluator error check
			if (!expr_status_assert(status, 3)) {
				backtrace(stmt);
				Error("Malicious statement parameters!");
			}

			IOStatus ioStatus = iio_fread(file, dataSize, offset);
			dump_coretime(coreTimeStack, ioStatus.coreTime);

			if (ioStatus.success)
				statementsSucceed[STMT_FREAD]++;
			else
				statementsFail[STMT_FREAD]++;
			break;
		}

		case STMT_FWRITE: {
			ExpressionStatus status[3];
			ParameterList* paramList = stmt->parameters;
			File* file = param_file_get(paramList, 0, &status[0]);
			glong dataSize = param_int_get(paramList, 1, &status[1]);
			glong offset = param_int_get_optional(paramList, 2, &status[2], -1);

			Verbose("~ Executing STMT_FWRITE: file = %p, dataSize = %ld, offset = %ld", file, dataSize, offset);

			// evaluator error check
			if (!expr_status_assert(status, 3)) {
				backtrace(stmt);
				Error("Malicious statement parameters!");
			}

			IOStatus ioStatus = iio_fwrite(file, dataSize, offset);
			dump_coretime(coreTimeStack, ioStatus.coreTime);

			if (ioStatus.success)
				statementsSucceed[STMT_FWRITE]++;
			else
				statementsFail[STMT_FWRITE]++;
			break;
		}

		case STMT_FSEEK: {
			ExpressionStatus status[3];
			ParameterList* paramList = stmt->parameters;
			File* file = param_file_get(paramList, 0, &status[0]);
			glong offset = param_int_get(paramList, 1, &status[1]);
			gint whence = param_int_get_optional(paramList, 2, &status[2], SEEK_SET);

			Verbose("~ Executing STMT_FSEEK: file = %p, offset = %ld, whence = %d", file, offset, whence);

			// evaluator error check
			if (!expr_status_assert(status, 3)) {
				backtrace(stmt);
				Error("Malicious statement parameters!");
			}

			IOStatus ioStatus = iio_fseek(file, offset, whence);
			dump_coretime(coreTimeStack, ioStatus.coreTime);

			if (ioStatus.success)
				statementsSucceed[STMT_FSEEK]++;
			else
				statementsFail[STMT_FSEEK]++;
			break;
		}

		case STMT_FSYNC: {
			ExpressionStatus status[1];
			ParameterList* paramList = stmt->parameters;
			File* file = param_file_get(paramList, 0, &status[0]);

			// evaluator error check
			if (!expr_status_assert(status, 1)) {
				backtrace(stmt);
				Error("Malicious statement parameters!");
			}

			IOStatus ioStatus = iio_fsync(file);
			dump_coretime(coreTimeStack, ioStatus.coreTime);

			if (ioStatus.success)
				statementsSucceed[STMT_FSYNC]++;
			else {
				statementsFail[STMT_FSYNC]++;
			}
			break;
		}

		case STMT_WRITE: {
			ExpressionStatus status[3];
			ParameterList* paramList = stmt->parameters;
			gchar* fname_raw = param_string_get(paramList, 0, &status[0]);
			glong  dataSize = param_int_get(paramList, 1, &status[1]);
			glong  offset = param_int_get_optional(paramList, 2, &status[2], 0);

			Verbose("~ Executing STMT_WRITE: file = %s, dataSize = %ld, offset = %ld", fname_raw, dataSize, offset);

			// evaluator error check
			if (!expr_status_assert(status, 3)) {
				backtrace(stmt);
				Error("Malicious statement parameters!");
			}

			gchar* fname = var_replace_substrings(fname_raw);

			IOStatus ioStatus = iio_write(fname, dataSize, offset);
			dump_coretime(coreTimeStack, ioStatus.coreTime);

			if (ioStatus.success)
				statementsSucceed[STMT_WRITE]++;
			else
				statementsFail[STMT_WRITE]++;

			g_free(fname_raw);
			g_free(fname);
			break;
		}

		case STMT_APPEND: {
			ExpressionStatus status[2];
			ParameterList* paramList = stmt->parameters;
			gchar* fname_raw = param_string_get(paramList, 0, &status[0]);
			glong  dataSize = param_int_get(paramList, 1, &status[1]);

			Verbose("~ Executing STMT_APPEND: file = %s, dataSize = %ld", fname_raw, dataSize);

			// evaluator error check
			if (!expr_status_assert(status, 2)) {
				backtrace(stmt);
				Error("Malicious statement parameters!");
			}

			gchar* fname = var_replace_substrings(fname_raw);

			IOStatus ioStatus = iio_append(fname, dataSize);
			dump_coretime(coreTimeStack, ioStatus.coreTime);

			if (ioStatus.success)
				statementsSucceed[STMT_APPEND]++;
			else
				statementsFail[STMT_APPEND]++;

			g_free(fname_raw);
			g_free(fname);
			break;
		}

		case STMT_READ: {
			ExpressionStatus status[3];
			ParameterList* paramList = stmt->parameters;
			gchar* fname_raw = param_string_get(paramList, 0, &status[0]);
			glong  dataSize = param_int_get_optional(paramList, 1, &status[1], READALL);
			glong  offset = param_int_get_optional(paramList, 2, &status[2], 0);

			Verbose("~ Executing STMT_READ: file = %s, dataSize = %ld, offset = %ld",
					fname_raw, dataSize, offset);

			// evaluator error check
			if (!expr_status_assert(status, 2)) {
				backtrace(stmt);
				Error("Malicious statement parameters!");
			}

			gchar* fname = var_replace_substrings(fname_raw);

			IOStatus ioStatus = iio_read(fname, dataSize, offset);
			dump_coretime(coreTimeStack, ioStatus.coreTime);

			if (ioStatus.success)
				statementsSucceed[STMT_READ]++;
			else
				statementsFail[STMT_READ]++;

			g_free(fname_raw);
			g_free(fname);
			break;
		}

		case STMT_LOOKUP: {
			ExpressionStatus status[1];
			ParameterList* paramList = stmt->parameters;
			gchar* fname_raw = param_string_get(paramList, 0, &status[0]);

			Verbose("~ Executing STMT_LOOKUP: file = %s", fname_raw);

			// evaluator error check
			if (!expr_status_assert(status, 1)) {
				backtrace(stmt);
				Error("Malicious statement parameters!");
			}

			gchar* fname = var_replace_substrings(fname_raw);

			IOStatus ioStatus = iio_lookup(fname);
			dump_coretime(coreTimeStack, ioStatus.coreTime);

			if (ioStatus.success)
				statementsSucceed[STMT_LOOKUP]++;
			else
				statementsFail[STMT_LOOKUP]++;

			g_free(fname_raw);
			g_free(fname);
			break;
		}

		case STMT_DELETE: {
			ExpressionStatus status[1];
			ParameterList* paramList = stmt->parameters;
			gchar* fname_raw = param_string_get(paramList, 0, &status[0]);

			Verbose("~ Executing STMT_DELETE: file = %s", fname_raw);

			// evaluator error check
			if (!expr_status_assert(status, 1)) {
				backtrace(stmt);
				Error("Malicious statement parameters!");
			}

			gchar* fname = var_replace_substrings(fname_raw);

			IOStatus ioStatus = iio_delete(fname);
			dump_coretime(coreTimeStack, ioStatus.coreTime);

			if (ioStatus.success)
				statementsSucceed[STMT_DELETE]++;
			else
				statementsFail[STMT_DELETE]++;

			g_free(fname_raw);
			g_free(fname);
			break;
		}

		case STMT_MKDIR: {
			ExpressionStatus status[1];
			ParameterList* paramList = stmt->parameters;
			gchar* fname_raw = param_string_get(paramList, 0, &status[0]);

			Verbose("~ Executing STMT_MKDIR: file = %s", fname_raw);

			// evaluator error check
			if (!expr_status_assert(status, 1)) {
				backtrace(stmt);
				Error("Malicious statement parameters!");
			}

			gchar* fname = var_replace_substrings(fname_raw);

			IOStatus ioStatus = iio_mkdir(fname);
			dump_coretime(coreTimeStack, ioStatus.coreTime);

			if (ioStatus.success)
				statementsSucceed[STMT_MKDIR]++;
			else
				statementsFail[STMT_MKDIR]++;

			g_free(fname_raw);
			g_free(fname);
			break;
		}

		case STMT_RMDIR: {
			ExpressionStatus status[1];
			ParameterList* paramList = stmt->parameters;
			gchar* fname_raw = param_string_get(paramList, 0, &status[0]);

			Verbose("~ Executing STMT_RMDIR: file = %s", fname_raw);

			// evaluator error check
			if (!expr_status_assert(status, 1)) {
				backtrace(stmt);
				Error("Malicious statement parameters!");
			}

			gchar* fname = var_replace_substrings(fname_raw);

			IOStatus ioStatus = iio_rmdir(fname);
			dump_coretime(coreTimeStack, ioStatus.coreTime);

			if (ioStatus.success)
				statementsSucceed[STMT_RMDIR]++;
			else
				statementsFail[STMT_RMDIR]++;

			g_free(fname_raw);
			g_free(fname);
			break;
		}

		case STMT_CREATE: {
			ExpressionStatus status[1];
			ParameterList* paramList = stmt->parameters;
			gchar* fname_raw = param_string_get(paramList, 0, &status[0]);

			Verbose("~ Executing STMT_CREATE: file = %s", fname_raw);

			// evaluator error check
			if (!expr_status_assert(status, 1)) {
				backtrace(stmt);
				Error("Malicious statement parameters!");
			}

			gchar* fname = var_replace_substrings(fname_raw);

			IOStatus ioStatus = iio_create(fname);
			dump_coretime(coreTimeStack, ioStatus.coreTime);

			if (ioStatus.success)
				statementsSucceed[STMT_CREATE]++;
			else
				statementsFail[STMT_CREATE]++;

			g_free(fname_raw);
			g_free(fname);
			break;
		}

		case STMT_STAT: {
			ExpressionStatus status[1];
			ParameterList* paramList = stmt->parameters;
			gchar* fname_raw = param_string_get(paramList, 0, &status[0]);

			Verbose("~ Executing STMT_STAT: file = %s", fname_raw);

			// evaluator error check
			if (!expr_status_assert(status, 1)) {
				backtrace(stmt);
				Error("Malicious statement parameters!");
			}

			gchar* fname = var_replace_substrings(fname_raw);

			IOStatus ioStatus = iio_stat(fname);
			dump_coretime(coreTimeStack, ioStatus.coreTime);

			if (ioStatus.success)
				statementsSucceed[STMT_STAT]++;
			else
				statementsFail[STMT_STAT]++;

			g_free(fname_raw);
			g_free(fname);
			break;
		}

		case STMT_RENAME: {
			ExpressionStatus status[2];
			ParameterList* paramList = stmt->parameters;
			gchar* oldname_raw = param_string_get(paramList, 0, &status[0]);
			gchar* newname_raw = param_string_get(paramList, 1, &status[1]);

			Verbose("~ Executing STMT_RENAME: oldname = %s, newname = %s", oldname_raw, oldname_raw);

			// evaluator error check
			if (!expr_status_assert(status, 2)) {
				backtrace(stmt);
				Error("Malicious statement parameters!");
			}

			gchar* oldname = var_replace_substrings(oldname_raw);
			gchar* newname = var_replace_substrings(newname_raw);

			IOStatus ioStatus = iio_rename(oldname, newname);
			dump_coretime(coreTimeStack, ioStatus.coreTime);

			if (ioStatus.success)
				statementsSucceed[STMT_RENAME]++;
			else
				statementsFail[STMT_RENAME]++;

			g_free(oldname_raw);
			g_free(newname_raw);
			g_free(oldname);
			g_free(newname);
			break;
		}

#ifdef HAVE_MPI
		case STMT_PFOPEN: {
			ExpressionStatus status[3];
			ParameterList* paramList = stmt->parameters;
			gchar* fhname = param_string_get(paramList, 0, &status[0]);
			gchar* fname_raw = param_string_get(paramList, 1, &status[1]);
			gchar* mode = param_string_get(paramList, 2, &status[2]);

			Verbose("~ Executing STMT_FOPEN: fhname = %s, fname = %s mode = %s", fhname, fname_raw, mode);

			// evaluator error check
			if (!expr_status_assert(status, 3)) {
				backtrace(stmt);
				Error("Malicious statement parameters!");
			}

			gchar* fname = var_replace_substrings(fname_raw);
			MPI_Comm comm = MPI_COMM_WORLD;
			if(groupStack)
				comm = ((GroupBlock*) g_list_first(groupStack)->data)->mpicomm;

			File* file;
			if (iio_pfopen(fname, mode, comm, &file)) {
				Verbose("  > file = %p", file);
				var_set_value(fhname, VAR_FILE, &file);
				statementsSucceed[STMT_PFOPEN]++;
			}
			else
				statementsFail[STMT_PFOPEN]++;

			g_free(fhname);
			g_free(fname_raw);
			g_free(fname);
			g_free(mode);
			break;
		}

		case STMT_PFCLOSE: {
			ExpressionStatus status[1];
			ParameterList* paramList = stmt->parameters;
			File* file = param_file_get(paramList, 0, &status[0]);

			Verbose("~ Executing STMT_FCLOSE: file = %p", file);

			// evaluator error check
			if (!expr_status_assert(status, 1)) {
				backtrace(stmt);
				Error("Malicious statement parameters!");
			}

			if (file && iio_pfclose(file)) {
				gchar* fhname = (gchar*) param_value_get(paramList, 0);
				var_destroy(fhname);
				statementsSucceed[STMT_PFCLOSE]++;
			}
			else
				statementsFail[STMT_PFCLOSE]++;
			break;
		}

		case STMT_PFWRITE: {
			ExpressionStatus status[2];
			ParameterList* paramList = stmt->parameters;
			File*  file = param_file_get(paramList, 0, &status[0]);
			gchar* pname = param_string_get(paramList, 1, &status[1]);

			Verbose("~ Executing STMT_PFWRITE: file = %p, pattern = %s", file, pname);

			// evaluator error check
			if (!expr_status_assert(status, 2)) {
				backtrace(stmt);
				Error("Malicious statement parameters!");
			}

			Pattern* pattern = g_hash_table_lookup(patternMap, pname);
			if (!pattern) {
				g_printf("Invalid pattern parameter in statement pfwrite.\n");
				Error("Malicious statement parameters!");
				break;
			}

			MPI_Comm comm = MPI_COMM_WORLD;
			if(groupStack)
				comm = ((GroupBlock*) g_list_first(groupStack)->data)->mpicomm;

			IOStatus ioStatus;
			switch(pattern->level) {
				/* Level 0: non-collective, contiguous */
				case 0:
					Verbose("  > level = 0");
					ioStatus = iio_pfwrite_level0(file, pattern);
					break;

				/* Level 1: collective, contiguous */
				case 1:
					Verbose("  > level = 1");
					ioStatus = iio_pfwrite_level1(file, pattern);
					break;

				/* Level 2: non-collective, non-contiguous */
				case 2:
					Verbose("  > level = 2");
					ioStatus = iio_pfwrite_level2(file, pattern);
					break;

				/* Level 3: collective, non-contiguous */
				case 3:
					Verbose("  > level = 3");
					ioStatus = iio_pfwrite_level3(file, pattern);
					break;

				default: Error("Invalid level (%d) for statement pfwrite!", pattern->level);
			}

			dump_coretime(coreTimeStack, ioStatus.coreTime);

			if (ioStatus.success)
				statementsSucceed[STMT_PFWRITE]++;
			else
				statementsFail[STMT_PFWRITE]++;

			g_free(pname);
			break;
		}

		case STMT_PFREAD: {
			ExpressionStatus status[2];
			ParameterList* paramList = stmt->parameters;
			File*  file = param_file_get(paramList, 0, &status[0]);
			gchar* pname = param_string_get(paramList, 1, &status[1]);

			Verbose("~ Executing STMT_PFREAD: file = %p, pattern = %s", file, pname);

			// evaluator error check
			if (!expr_status_assert(status, 2)) {
				backtrace(stmt);
				Error("Malicious statement parameters!");
			}

			Pattern* pattern = g_hash_table_lookup(patternMap, pname);
			if (!pattern) {
				g_printf("Invalid pattern parameter in statement pread.\n");
				Error("Malicious statement parameters!");
				break;
			}

			MPI_Comm comm = MPI_COMM_WORLD;
			if(groupStack)
				comm = ((GroupBlock*) g_list_first(groupStack)->data)->mpicomm;

			IOStatus ioStatus;
			switch(pattern->level) {
				/* Level 0: non-collective, contiguous */
				case 0:
					Verbose("  > level = 0");
					ioStatus = iio_pfread_level0(file, pattern);
					break;

				/* Level 1: collective, contiguous */
				case 1:
					Verbose("  > level = 1");
					ioStatus = iio_pfread_level1(file, pattern);
					break;

				/* Level 2: non-collective, non-contiguous */
				case 2:
					Verbose("  > level = 2");
					ioStatus = iio_pfread_level2(file, pattern);
					break;

				/* Level 3: collective, non-contiguous */
				case 3:
					Verbose("  > level = 3");
					ioStatus = iio_pfread_level3(file, pattern);
					break;

				default: Error("Invalid level (%d) for statement pfread!", pattern->level);
			}

			dump_coretime(coreTimeStack, ioStatus.coreTime);

			if (ioStatus.success)
				statementsSucceed[STMT_PFREAD]++;
			else
				statementsFail[STMT_PFREAD]++;

			g_free(pname);
			break;
		}

		case STMT_PWRITE: {
			ExpressionStatus status[2];
			ParameterList* paramList = stmt->parameters;
			gchar* fname_raw = param_string_get(paramList, 0, &status[0]);
			gchar* pname = param_string_get(paramList, 1, &status[1]);

			Verbose("~ Executing STMT_PWRITE: file = %s, pattern = %s", fname_raw, pname);

			// evaluator error check
			if (!expr_status_assert(status, 2)) {
				backtrace(stmt);
				Error("Malicious statement parameters!");
			}

			gchar* fname = var_replace_substrings(fname_raw);

			Pattern* pattern = g_hash_table_lookup(patternMap, pname);
			if (!pattern) {
				g_printf("Invalid pattern parameter in statement pwrite.\n");
				Error("Malicious statement parameters!");
				break;
			}

			MPI_Comm comm = MPI_COMM_WORLD;
			if(groupStack)
				comm = ((GroupBlock*) g_list_first(groupStack)->data)->mpicomm;

			IOStatus ioStatus;
			switch(pattern->level) {
				/* Level 0: non-collective, contiguous */
				case 0:
					Verbose("  > level = 0");
					ioStatus = iio_pwrite_level0(fname, pattern, comm);
					break;

				/* Level 1: collective, contiguous */
				case 1:
					Verbose("  > level = 1");
					ioStatus = iio_pwrite_level1(fname, pattern, comm);
					break;

				/* Level 2: non-collective, non-contiguous */
				case 2:
					Verbose("  > level = 2");
					ioStatus = iio_pwrite_level2(fname, pattern, comm);
					break;

				/* Level 3: collective, non-contiguous */
				case 3:
					Verbose("  > level = 3");
					ioStatus = iio_pwrite_level3(fname, pattern, comm);
					break;

				default: Error("Invalid level (%d) for statement pwrite!", pattern->level);
			}

			dump_coretime(coreTimeStack, ioStatus.coreTime);

			if (ioStatus.success)
				statementsSucceed[STMT_PWRITE]++;
			else
				statementsFail[STMT_PWRITE]++;

			g_free(fname_raw);
			g_free(fname);
			g_free(pname);
			break;
		}

		case STMT_PREAD: {
			ExpressionStatus status[2];
			ParameterList* paramList = stmt->parameters;
			gchar* fname_raw = param_string_get(paramList, 0, &status[0]);
			gchar* pname = param_string_get(paramList, 1, &status[1]);

			Verbose("~ Executing STMT_PREAD: file = %s, pattern = %s", fname_raw, pname);

			// evaluator error check
			if (!expr_status_assert(status, 2)) {
				backtrace(stmt);
				Error("Malicious statement parameters!");
			}

			gchar* fname = var_replace_substrings(fname_raw);

			Pattern* pattern = g_hash_table_lookup(patternMap, pname);
			if (!pattern) {
				g_printf("Invalid pattern parameter in statement pread.\n");
				g_free(fname);
				Error("Malicious statement parameters!");
				break;
			}

			MPI_Comm comm = MPI_COMM_WORLD;
			if(groupStack)
				comm = ((GroupBlock*) g_list_first(groupStack)->data)->mpicomm;

			IOStatus ioStatus;
			switch(pattern->level) {
				/* Level 0: non-collective, contiguous */
				case 0:
					Verbose("  > level = 0");
					ioStatus = iio_pread_level0(fname, pattern, comm);
					break;

				/* Level 1: collective, contiguous */
				case 1:
					Verbose("  > level = 1");
					ioStatus = iio_pread_level1(fname, pattern, comm);
					break;

				/* Level 2: non-collective, non-contiguous */
				case 2:
					Verbose("  > level = 2");
					ioStatus = iio_pread_level2(fname, pattern, comm);
					break;

				/* Level 3: collective, non-contiguous */
				case 3:
					Verbose("  > level = 3");
					ioStatus = iio_pread_level3(fname, pattern, comm);
					break;

				default: Error("Invalid level (%d) for statement pread!", pattern->level);
			}

			dump_coretime(coreTimeStack, ioStatus.coreTime);

			if (ioStatus.success)
				statementsSucceed[STMT_PREAD]++;
			else
				statementsFail[STMT_PREAD]++;

			g_free(fname_raw);
			g_free(fname);
			g_free(pname);
			break;
		}

		case STMT_PDELETE: {
			ExpressionStatus status[1];
			ParameterList* paramList = stmt->parameters;
			gchar* fname_raw = param_string_get(paramList, 0, &status[0]);

			Verbose("~ Executing STMT_PDELETE: file = %s", fname_raw);

			// evaluator error check
			if (!expr_status_assert(status, 1)) {
				backtrace(stmt);
				Error("Malicious statement parameters!");
			}

			gchar* fname = var_replace_substrings(fname_raw);

			if (iio_pdelete(fname))
				statementsSucceed[STMT_PDELETE]++;
			else
				statementsFail[STMT_PDELETE]++;

			g_free(fname_raw);
			g_free(fname);
			break;
		}
#endif

		/* ![ModuleHook] statement_exec */

		default: Error("Invalid statement! (id=%d)\n", stmt->type);
	}
}

void iiParse()
{
	yyparse();
}

void iiSetParameters(int argc, char ** argv)
{
	// load default arguments into variables.
	{
		GSList* cur = paramList;

		while(cur != NULL){

			param_list_entry * param_entry = (param_list_entry*) cur->data;
			env_list_entry * env_entry = (env_list_entry*) malloc(sizeof(env_list_entry));

			env_entry->env_name = malloc(2048*sizeof(gchar));
			//env_entry->env_value = malloc(2048*sizeof(gchar));

			// Override default, with value of environment variable
			g_snprintf(env_entry->env_name, 2048, "PARABENCH_%s", param_entry->param_name);
			//sprintf(env_entry->env_name, "PARABENCH_%s", param_entry->param_name);
			env_entry->env_value = getenv(env_entry->env_name);
			

			// now set the values:
			// use environment as default if set
			if ( env_entry->env_value != NULL )
			{
				var_set_value(param_entry->var_name, VAR_STRING, env_entry->env_value);
				printf("Parameter: %s, %s=%s\n", param_entry->var_name, env_entry->env_name, env_entry->env_value );
			} else {
				var_set_value(param_entry->var_name, VAR_STRING, param_entry->default_value);
			}

			free(env_entry->env_name);
			free(env_entry);

			cur = cur->next;
		}
	}

	gint i;
	for (i=0; i < argc; i++){	 	  
		char * value=strstr(argv[i], "=");
		if(value == NULL){
			printf("Error, specify parameters with <PARAM>=<VALUE>\n");
			exit(1);
		}
		value[0] = 0;
		value++;
		char * param=argv[i];

		//printf("Setting: %s to %s\n", param, value);

		// check if paramList contains variable:
		GSList * cur = paramList;
		while(cur != NULL){

			param_list_entry * param_entry = (param_list_entry*) cur->data;

			if(strcmp(param_entry->param_name, param) == 0){
				// now set the values:
				//printf("Setting: %s to %s\n", param_entry->var_name, value);
				var_set_value(param_entry->var_name, VAR_STRING, value);
				break;
			}


			cur = cur->next;
		}

		if(cur == NULL){
			printf("Error, unknown parameter %s with value \"%s\"\n", param, value);
			exit(1);
		}
	}
}

void iiStart()
{
	if (ast) g_node_children_foreach(ast, G_TRAVERSE_ALL, &ExecuteStatement, NULL);
}

void iiTimeReport()
{
	g_printf("\n********************** Time Report **********************\n");
	
	if(g_slist_length(timeList) > 0) {
		// sort events by global occurence
		timeList = g_slist_sort(timeList, compare_time_events);
		GSList* iter = timeList;
		gint lastprocid = 0;
		
		g_printf(" [P]   [#]                       [event]        [seconds]\n");
		
		for(;iter;iter=g_slist_next(iter)) {
			TimeEvent* event = (TimeEvent*) iter->data;
			
			if(event->proc != lastprocid) {
				g_printf("---------------------------------------------------------\n");
			}
			
			g_printf(" %2d   %3d   %28s   %13.6fs\n", event->proc, event->id, event->name, event->value);
			lastprocid = event->proc;
		}
		
		g_printf("\n");
		g_printf("[P]          - Process rank\n");
		g_printf("[#]          - Execution order of the time command\n");
		g_printf("[event]      - Time event label\n");
		g_printf("[seconds]    - Duration of the time event\n");
	}
	else {
		g_printf("No time events.\n");
	}
}

void iiCoreTimeReport()
{
	g_printf("\n******************* Core Time Report ********************\n");

	if(g_slist_length(coreTimeList) > 0) {
		// sort events by global occurence
		coreTimeList = g_slist_sort(coreTimeList, compare_coretime_events);
		GSList* iter = coreTimeList;
		gint lastprocid = 0;

		g_printf(" [P]   [#]                    [event]            [result]\n");

		for(;iter;iter=g_slist_next(iter)) {
			CoreTimeEvent* event = (CoreTimeEvent*) iter->data;

			if(event->proc != lastprocid) {
				g_printf("---------------------------------------------------------\n");
			}

			gchar* avg = format_coretime_throughput(event->avgCoreTime);
			gchar* min = format_coretime_throughput(event->minCoreTime);
			gchar* max = format_coretime_throughput(event->maxCoreTime);
			glong ioops = (event->numCalls>0? event->numCalls / event->avgCoreTime.time : 0);

			gchar* total = format_data_size(event->avgCoreTime.data);

			g_printf(" %2d   %3d   %25s   avg %12s\n", event->proc, event->id, event->name, avg);
			g_printf(" %36s   min %12s\n", "", min);
			g_printf(" %36s   max %12s\n", "", max);
			g_printf("\n");
			g_printf(" %36s   avg %11.6f s\n", "", (event->numCalls? event->avgCoreTime.time / event->numCalls : 0));
			g_printf(" %36s   min %11.6f s\n", "", (event->minCallTime<G_MAXDOUBLE? event->minCallTime : 0));
			g_printf(" %36s   max %11.6f s\n", "", (event->maxCallTime>G_MINDOUBLE? event->maxCallTime : 0));
			g_printf("\n");
			g_printf(" %36s  %10ld IOops/s\n", "", ioops);
			g_printf("\n");
			g_printf(" %24s Total: %10s / %.6f s\n", "", total, event->avgCoreTime.time);
			g_printf("\n");

			lastprocid = event->proc;
			g_free(avg);
			g_free(min);
			g_free(max);
		}

		g_printf("[results]\n");
		g_printf("- Core time I/O throughput (average, min, max)\n");
		g_printf("- Calltime (average, min, max) for all statements\n  during this CoreTime Event\n");
		g_printf("- Total data processed per time in seconds\n  during this CoreTime event\n");
	}
	else {
		g_printf("No coretime events.\n");
	}
}

void iiCommandReport()
{
	g_printf("\n******************** Command Report *********************\n");
	
	gint i;
	for (i=0; i<NUM_TRAC_STATEMENTS; i++) {
		if ((statementsSucceed[i] > 0) || (statementsFail[i] > 0)) {
			break;
		}
	}

	if (i == NUM_TRAC_STATEMENTS) {
		g_printf("No I/O commands executed.\n");
		return;
	}
	
	for (i=0; i<NUM_TRAC_STATEMENTS; i++) {
		if(statementsSucceed[i]!=0 || statementsFail[i]!=0)
			g_printf(" %-7s  %13d successful / %13d failed\n", stmt_get_string(i), statementsSucceed[i],  statementsFail[i]);
	}
}

void clean_created_data()
{
	GSList* fiter = fileList;
	GSList* diter = dirList;
	gboolean iosucc;
	gint errnum = 0;
	
	for(;fiter;fiter=g_slist_next(fiter)) {
		iosucc = iio_delete((gchar*) fiter->data).success;
		
		if(iosucc)
			errnum++;
	}
	
	for(;diter;diter=g_slist_next(diter)) {
		iosucc = iio_rmdir((gchar*) diter->data).success;
		
		if(!iosucc)
			errnum++;
	}
	
	if((size == 1) && (errnum > 0))
		g_printf("\nCleaning for %d items failed.\n", errnum);
}

#ifdef HAVE_MPI
gint getCollectiveRandomNumber()
{
	gint irand;
	gint groupRank;
	MPI_Comm comm = MPI_COMM_WORLD;

	// fetch communicator if we are withing a group block
	// if not use world communicator
	if(groupStack)
		comm = ((GroupBlock*) g_list_first(groupStack)->data)->mpicomm;

	MPI_Comm_rank(comm, &groupRank);

	// broadcast random value within group
	if(groupRank == MASTER) {
		irand = g_random_int();
		MPI_Bcast(&irand, 1, MPI_INT, MASTER, comm);
	}
	else{
		MPI_Bcast(&irand, 1, MPI_INT, MASTER, comm);
	}
	return irand;
}
#endif
