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

#include "parameters.h"

#include <assert.h>

inline glong param_int_get(ParameterList* paramList, gint index, ExpressionStatus* status)
{
	g_assert(paramList);

	if (param_list_size(paramList) > index)
		return expr_evaluate_to_int(param_index_get(paramList, index), status);

	if (status) *status = STATUS_INVALID_EXPRESSION;
	return 0;
}

inline gchar* param_string_get(ParameterList* paramList, gint index, ExpressionStatus* status)
{
	if (param_list_size(paramList) > index)
		return expr_evaluate_to_string(param_index_get(paramList, index), status);

	if (status) *status = STATUS_INVALID_EXPRESSION;
	return "";
}

inline File* param_file_get(ParameterList* paramList, gint index, ExpressionStatus* status)
{
	g_assert(paramList);

	if (param_list_size(paramList) > index)
		return expr_evaluate_to_handle(param_index_get(paramList, index), status);

	if (status) *status = STATUS_INVALID_EXPRESSION;
	return NULL;
}

inline gpointer param_value_get(ParameterList* paramList, gint index)
{
	g_assert(paramList);

	if (param_list_size(paramList) > index) {
		Expression* e = param_index_get(paramList, index);
		return (gpointer) e->value;
	}

	return NULL;
}

inline glong param_int_get_optional(ParameterList* paramList, gint index, ExpressionStatus* status, glong defaultValue)
{
	g_assert(paramList);

	if (param_list_size(paramList) > index)
		return expr_evaluate_to_int(param_index_get(paramList, index), status);
	else {
		if (status) *status = STATUS_EVAL_OK;
		return defaultValue;
	}
}

inline gchar* param_string_get_optional(ParameterList* paramList, gint index, ExpressionStatus* status, gchar* defaultValue)
{
	g_assert(paramList);

	if (param_list_size(paramList) > index)
		return expr_evaluate_to_string(param_index_get(paramList, index), status);
	else {
		if (status) *status = STATUS_EVAL_OK;
		return defaultValue;
	}
}

inline void param_list_assert_length(ParameterList* paramList, gint lengths[])
{
	g_assert(paramList);

	gint i, size = sizeof(lengths)/sizeof(gint);
	gboolean valid = FALSE;

	for (i=0; i<size; i++)
		if (param_list_size(paramList) == lengths[i])
			valid = TRUE;

	assert(valid);
}

void param_list_free(ParameterList* paramList)
{
	if (!paramList) return;

	gint i, size = param_list_size(paramList);

	for (i=0; i<size; i++) {
		Expression* expr = g_array_index(paramList, Expression*, i);
		expr_free(expr);
	}
	g_array_free(paramList, TRUE);
}
