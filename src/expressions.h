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

#ifndef EXPRESSIONS_H_
#define EXPRESSIONS_H_

#include "variables.h"
#include "iio.h"

#include <glib.h>
#include <stdio.h>

typedef enum {
	EXPR_UNARY_INT,
    EXPR_CONSTANT_INT,  EXPR_CONSTANT_STRING,
    EXPR_CONSTANT_BOOL, EXPR_VARIABLE,
    EXPR_RICH_STRING,   EXPR_RICH_INT
} ExpressionType;

typedef enum {
	NOP,
	OP_ARITH_ADD, OP_ARITH_SUB,
	OP_ARITH_MUL, OP_ARITH_DIV,
	OP_ARITH_POW, OP_ARITH_FAC,
	OP_ARITH_MOD,
	OP_BIN_AND, OP_BIN_OR,
	OP_COMP_EQ,
	OP_COMP_LT, OP_COMP_LEQ,
	OP_COMP_GT, OP_COMP_GEQ
} ExpressionOperator;

typedef enum {
	STATUS_EVAL_FAILED,
	STATUS_INVALID_EXPRESSION,
	STATUS_INVALID_OPERATOR,
	STATUS_INVALID_VARIABLE,
	STATUS_DIVISION_BY_ZERO,
	STATUS_EVAL_OK
} ExpressionStatus;

typedef struct _Expression Expression;
struct _Expression {
	ExpressionType type;
	gconstpointer value;
	ExpressionOperator operator;
	Expression* left;
	Expression* right;
};

glong    expr_evaluate_to_int(Expression* expression, ExpressionStatus* status);
gchar*   expr_evaluate_to_string(Expression* expression, ExpressionStatus* status);
File*    expr_evaluate_to_handle(Expression* expression, ExpressionStatus* status);
gboolean expr_evaluate_to_bool(Expression* expression, ExpressionStatus* status);

Expression* expr_new(ExpressionType type, gpointer value, ExpressionOperator operator, Expression* left, Expression* right);
#define expr_rich_int_new(o,l,r)    expr_new(EXPR_RICH_INT, NULL, (o), (l), (r));
#define expr_rich_string_new(o,l,r) expr_new(EXPR_RICH_STRING, NULL, (o), (l), (r));
#define expr_constant_bool_new(v)   expr_new(EXPR_CONSTANT_BOOL, GINT_TO_POINTER((v)), NOP, NULL, NULL)
Expression* expr_unary_int_new(ExpressionOperator operator, glong value);
Expression* expr_constant_int_new(glong value);
Expression* expr_constant_string_new(const gchar* string);
Expression* expr_variable_new(const gchar* varName);

void expr_free(Expression* e);

gchar*   expr_status_to_string(ExpressionStatus status);
gboolean expr_status_assert(ExpressionStatus* status, int len);

#endif /* EXPRESSIONS_H_ */
