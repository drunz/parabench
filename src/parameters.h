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

#ifndef PARAMETERS_H_
#define PARAMETERS_H_

#include "expressions.h"

#include <glib.h>

/* The parameter list is simply an GArray containing of pointers
 * to an expression tree root node. Constant values are also realized via expressions.
 */

/**
 * GPtrArray backend
 */
//typedef GPtrArray ParameterList;
//
//
//#define param_list_size(l) (l)->len
//
//#define param_list_new() g_ptr_array_new()
//#define param_list_append(l,v) g_ptr_array_add((l), (v))
//#define param_list_free(l) g_ptr_array_free((l), TRUE)
//
//#define param_index_get(l,i) g_ptr_array_index((l), (i))
//
//inline glong  param_int_get(ParameterList* paramList, gint index, ExpressionStatus* status);
//inline gchar* param_string_get(ParameterList* paramList, gint index, ExpressionStatus* status);
////inline Pattern* param_pattern_get(ParameterList* paramList, gint index, ExpressionStatus* status);
//inline void param_list_assert_length(ParameterList* paramList, gint lengths[]);


/**
 * GArray backend
 */
typedef GArray ParameterList;


#define param_list_size(l) (l)->len

#define param_list_new() g_array_new(FALSE, TRUE, sizeof(Expression*))
#define param_list_append(l,v) g_array_append_val((l), (v))
#define param_list_prepend(l,v) g_array_prepend_val((l), (v))
#define param_list_insert(l,i,v) g_array_insert_val((l), (i), (v))
//#define param_list_free g_array_free
void param_list_free(ParameterList* paramList);

#define param_index_get(l,i) g_array_index((l), Expression*, (i))
//define param_int_get(l,i,s) expr_evaluate_to_int(g_array_index((l), Expression*, (i)), (s))
//define param_string_get(l,i,s) expr_evaluate_to_string(g_array_index((l), Expression*, (i)), (s))

inline glong  param_int_get(ParameterList* paramList, gint index, ExpressionStatus* status);
inline gchar* param_string_get(ParameterList* paramList, gint index, ExpressionStatus* status);
inline File*  param_file_get(ParameterList* paramList, gint index, ExpressionStatus* status);
inline gpointer param_value_get(ParameterList* paramList, gint index);

inline glong param_int_get_optional(ParameterList* paramList, gint index, ExpressionStatus* status, glong defaultValue);
inline gchar* param_string_get_optional(ParameterList* paramList, gint index, ExpressionStatus* status, gchar* defaultValue);

//inline Pattern* param_pattern_get(ParameterList* paramList, gint index, ExpressionStatus* status);
inline void param_list_assert_length(ParameterList* paramList, gint lengths[]);

/**
 * GSList backend
 */
//typedef GSList ParameterList;
//
//#define param_list_new() NULL
//#define param_list_append(l,v) (l) = g_slist_append((l), (v))
//#define param_list_free(l,f) g_slist_free(l)
//
//#define param_int_get(l,i,s) expr_evaluate_to_int((Expression*) g_slist_nth_data((l), (i)), (s))

#endif /* PARAMETERS_H_ */
