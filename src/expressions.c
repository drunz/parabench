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
#include "expressions.h"
#include "interpreter.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <glib/gprintf.h>


glong expr_evaluate_to_int(Expression* expression, ExpressionStatus* status)
{
	switch (expression->type) {
		case EXPR_CONSTANT_INT:
			if (status) *status = STATUS_EVAL_OK;
			return *((glong*) expression->value);

		case EXPR_VARIABLE: {
			gchar* varName = (gchar*) expression->value;
			VarDesc* var = var_lookup(varName);

			/* user space variables */
			if (var && (var->type == VAR_INT)) {
				if (status) *status = STATUS_EVAL_OK;
				return *((glong*) var->value);
			}
			 // cross evaluate (overflow, errors not handled, returns 0 in this case)
			else if (var && (var->type == VAR_STRING)) {
				glong n = atol((gchar*) var->value);
				if (status) *status = STATUS_EVAL_OK;
				return n;
			}
			/* internal variables */
			else if(varName[0] == '$') {
				if (strstr(varName, "rank") != NULL) {
					if (status) *status = STATUS_EVAL_OK;
					return rank;
				}
				else if (strstr(varName, "rand") != NULL) {
					if (status) *status = STATUS_EVAL_OK;
					return g_random_int();
				}
				else if (strstr(varName, "crand") != NULL) {
#ifdef HAVE_MPI
					if (status) *status = STATUS_EVAL_OK;
					return getCollectiveRandomNumber();
#endif
				}
				else{
					if (status) *status = STATUS_INVALID_VARIABLE;
					return 0;
				}
			}
			else {
				if (status) *status = STATUS_INVALID_VARIABLE;
				return 0;
			}
			break;

		} case EXPR_UNARY_INT: {
			glong value = *((glong*) expression->value);

			if (status) *status = STATUS_EVAL_OK;

			switch (expression->operator) {
				case OP_ARITH_FAC: {
					glong i;
					glong result = 1;

					for (i=1; i<=value; i++)
						result *= i;

					return result;
				}

				default:
					if (status) *status = STATUS_INVALID_OPERATOR;
					return 0;
			}
			break;

		} case EXPR_RICH_INT: {
			ExpressionStatus sLeft, sRight;
			glong left  = expr_evaluate_to_int(expression->left, &sLeft);
			glong right = expr_evaluate_to_int(expression->right, &sRight);

			if (sLeft != STATUS_EVAL_OK) {
				if (status) *status = sLeft;
				if (sRight != STATUS_EVAL_OK) {
					if (status) *status = sRight;
				}
				return 0;
			}

			if (status) *status = STATUS_EVAL_OK;

			switch (expression->operator) {
				case OP_ARITH_ADD:
					return left + right;

				case OP_ARITH_SUB:
					return left - right;

				case OP_ARITH_MUL:
					return left * right;

				case OP_ARITH_DIV:
					if (right == 0) {
						if (status) *status = STATUS_DIVISION_BY_ZERO;
						return 0;
					}

					return left / right;

				case OP_ARITH_POW:
					return pow(left, right);

				case OP_ARITH_MOD:
					if (right == 0) {
						if (status) *status = STATUS_DIVISION_BY_ZERO;
						return 0;
					}
					return left % right;

				case OP_BIN_AND:
					return left & right;

				case OP_BIN_OR:
					return left | right;

				default:
					if (status) *status = STATUS_INVALID_OPERATOR;
					return 0;
			}
		}

		default:
			if (status) *status = STATUS_INVALID_EXPRESSION;
			return 0;
	}

	if (status) *status = STATUS_EVAL_FAILED;
	return 0;
}

/**
 * Return value needs to be freed manually!
 */
gchar* expr_evaluate_to_string(Expression* expression, ExpressionStatus* status)
{
	GString* buffer = g_string_new("");

	switch (expression->type) {
		case EXPR_CONSTANT_INT:
			if (status) *status = STATUS_EVAL_OK;
			g_string_append_printf(buffer, "%ld", *((glong*) expression->value));
			break;

		case EXPR_CONSTANT_STRING:
			if (status) *status = STATUS_EVAL_OK;
			g_string_append(buffer, (gchar*) expression->value);
			break;

		case EXPR_VARIABLE: {
			gchar* varName = (gchar*) expression->value;
			VarDesc* var = var_lookup(varName);

			/* user space variables */
			if (var) {
				if (var->type == VAR_STRING) {
					if (status) *status = STATUS_EVAL_OK;
					g_string_append(buffer, (gchar*) var->value);
					break;
				}
				else if (var->type == VAR_INT) {
					if (status) *status = STATUS_EVAL_OK;
					g_string_append_printf(buffer, "%ld", *((glong*) var->value));
					break;
				}
				else {
					if (status) *status = STATUS_INVALID_VARIABLE;
					return "<error during string evaluation - unknown variable type>";
				}
			}
			/* internal variables */
			else if (varName[0] == '$') {
				if (strstr(varName, "rank") != NULL) {
					if (status) *status = STATUS_EVAL_OK;
					g_string_append_printf(buffer, "%d", rank);
				}
				else if (strstr(varName, "rand") != NULL) {
					if (status) *status = STATUS_EVAL_OK;
					g_string_append_printf(buffer, "%u", g_random_int());
				}
				else if (strstr(varName, "crand") != NULL) {
#ifdef HAVE_MPI
					if (status) *status = STATUS_EVAL_OK;
					g_string_append_printf(buffer, "%u", getCollectiveRandomNumber());
#endif
				}
				else {
					if (status) *status = STATUS_INVALID_VARIABLE;
					return "<error during string evaluation - unknown internal variable>";
				}
			}
			else {
				if (status) *status = STATUS_INVALID_VARIABLE;
				return "<error during string evaluation - invalid variable>";
			}
			break;
		}

		case EXPR_RICH_INT: {
			ExpressionStatus recStatus;
			glong result = expr_evaluate_to_int(expression, &recStatus);

			if (recStatus != STATUS_EVAL_OK) {
				if (status) *status = recStatus;
				return "";
			}
			else *status = STATUS_EVAL_OK;

			g_string_append_printf(buffer, "%ld", result);
			break;
		}

		default:
			if (status) *status = STATUS_INVALID_EXPRESSION;
			return "<error during string evaluation>";
	}

	return g_string_free(buffer, FALSE);
}

File* expr_evaluate_to_handle(Expression* expression, ExpressionStatus* status)
{
	if (expression->type == EXPR_VARIABLE) {
		gchar* varName = (gchar*) expression->value;
		VarDesc* var = var_lookup(varName);

		/* user space variables */
		if (var && (var->type == VAR_FILE)) {
			if (status) *status = STATUS_EVAL_OK;
			return *((File**) var->value);
		}
		else {
			if (status) *status = STATUS_INVALID_VARIABLE;
			return NULL;
		}
	}
	else {
		if (status) *status = STATUS_EVAL_FAILED;
		return NULL;
	}
}

gboolean expr_evaluate_to_bool(Expression* expression, ExpressionStatus* status)
{
	if (status) *status = STATUS_EVAL_FAILED;
	return FALSE;
}

Expression* expr_new(ExpressionType type, gpointer value, ExpressionOperator operator, Expression* left, Expression* right)
{
	Expression *e = g_malloc0(sizeof(Expression));
	e->type = type;
	e->value = value;
	e->operator = operator;
	e->left = left;
	e->right = right;
	return e;
}

Expression* expr_unary_int_new(ExpressionOperator operator, glong value)
{
	//memalign: <struct><glong value>
	Expression *e = g_malloc0(sizeof(Expression) + sizeof(glong));
	e->type = EXPR_UNARY_INT;
	e->value = (gpointer) e + sizeof(Expression);
	e->operator = operator;
	e->left = NULL;
	e->right = NULL;
	memcpy((gpointer) e->value, &value, sizeof(glong));
	return e;
}

Expression* expr_constant_int_new(glong value)
{
	//memalign: <struct><glong value>
	Expression *e = g_malloc0(sizeof(Expression) + sizeof(glong));
	e->type = EXPR_CONSTANT_INT;
	e->value = (gpointer) e + sizeof(Expression);
	e->operator = NOP;
	e->left = NULL;
	e->right = NULL;
	memcpy((gpointer) e->value, &value, sizeof(glong));
	return e;
}

Expression* expr_constant_string_new(const gchar* string)
{
	gint stringLength = strlen(string) + 1;

	//memalign: <struct><gchar string>
	Expression *e = g_malloc0(sizeof(Expression) + stringLength);
	e->type = EXPR_CONSTANT_STRING;
	e->value = (gpointer) e + sizeof(Expression);
	e->operator = NOP;
	e->left = NULL;
	e->right = NULL;
	memcpy((gpointer) e->value, string, stringLength);
	return e;
}

Expression* expr_variable_new(const gchar* varName)
{
	gint stringLength = strlen(varName) + 1;

	//memalign: <struct><gchar string>
	Expression *e = g_malloc0(sizeof(Expression) + stringLength);
	e->type = EXPR_VARIABLE;
	e->value = (gpointer) e + sizeof(Expression);
	e->operator = NOP;
	e->left = NULL;
	e->right = NULL;
	memcpy((gpointer) e->value, varName, stringLength);
	return e;
}

void expr_free(Expression* e)
{
	if (!e) return;

	expr_free(e->left);
	expr_free(e->right);

	g_free(e);
}

gchar* expr_status_to_string(ExpressionStatus status)
{
	switch (status) {
		case STATUS_EVAL_OK:
			return "SATUS_EVAL_OK";

		case STATUS_EVAL_FAILED:
			return "STATUS_EVAL_FAILED";

		case STATUS_INVALID_EXPRESSION:
			return "STATUS_INVALID_EXPRESSION";

		case STATUS_INVALID_OPERATOR:
			return "STATUS_INVALID_OPERATOR";

		case STATUS_DIVISION_BY_ZERO:
			return "SATUS_DIVISION_BY_ZERO";

		case STATUS_INVALID_VARIABLE:
			return "STATUS_INVALID_VARIABLE";

		default:
			return "Unknown Status";
	}
}

gboolean expr_status_assert(ExpressionStatus* status, int len)
{
	int i;
	gboolean passed = TRUE;
	for (i=0; i<len; i++)
		if (status[i] != STATUS_EVAL_OK) {
			Log("Error during expression evaluation (index=%d, status=%s)",
					i, expr_status_to_string(status[i]));
			passed = FALSE;
		}

	return passed;
}
