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


#include <glib.h>
#include <glib/gstdio.h>
#include <glib/gprintf.h>
#include <math.h>
#include <fcntl.h>
#include "../build/default/config.h"
#include "../variables.h"
#include "../interpreter.h"


gboolean parseOnly = FALSE;
gchar* sourceFileName = "";
//int yyparse() { return 0; }
//void yyset_in(FILE* file) {}


/**
 * Test suite helper functions
 */
void create_file(const gchar* fname)
{
	FILE* fh = fopen(fname, "w+");
	g_assert(fh);
	fclose(fh);
}

void delete_file(const gchar* fname)
{
	g_assert(g_remove(fname) == 0);
}


/**
 * Test cases
 */
void test_evaluator_unary_int_op_fac()
{
	/* create expression tree */
	// Expression: 17!
	const glong VALID_RESULT = 355687428096000;
	Expression* e = expr_unary_int_new(OP_ARITH_FAC, 17);

	g_assert(e);

	/* evaluate */
	ExpressionStatus status;
	glong result = expr_evaluate_to_int(e, &status);

	g_message("Status: %s", expr_status_to_string(status));
	g_assert(status == STATUS_EVAL_OK);
	g_assert_cmpint(result, ==, VALID_RESULT);
}

void test_evaluator_rich_int_op_add()
{
	int i, rep = 100;
	for (i=0; i<rep; i++) {
		const gint32 a = g_test_rand_int_range(0, G_MAXINT32/3 - 1);
		const gint32 b = g_test_rand_int_range(0, G_MAXINT32/3 - 1);
		const gint32 c = g_test_rand_int_range(0, G_MAXINT32/3 - 1);
		const glong VALID_RESULT = a+b+c;

		/* create expression tree */
		// Expression: a + b + c
		Expression* e1 = expr_constant_int_new(a);
		Expression* e2 = expr_constant_int_new(b);
		Expression* e3 = expr_rich_int_new(OP_ARITH_ADD, e1, e2);
		Expression* e4 = expr_constant_int_new(c);
		Expression* root = expr_rich_int_new(OP_ARITH_ADD, e3, e4);

		g_assert(e1);
		g_assert(e2);
		g_assert(e3);
		g_assert(e4);
		g_assert(root);

		/* evaluate */
		ExpressionStatus status;
		glong result = expr_evaluate_to_int(root, &status);

		g_message("Status: %s", expr_status_to_string(status));
		g_assert(status == STATUS_EVAL_OK);
		g_assert_cmpint(result, ==, VALID_RESULT);

		//expr_free(root);
	}
}

void test_evaluator_rich_int_op_sub()
{
	int i, rep = 100;
	for (i=0; i<rep; i++) {
		const gint32 a = g_test_rand_int_range(0, G_MAXINT32/3 - 1);
		const gint32 b = g_test_rand_int_range(0, G_MAXINT32/3 - 1);
		const gint32 c = g_test_rand_int_range(0, G_MAXINT32/3 - 1);
		const glong VALID_RESULT = a-b-c;

		/* create expression tree */
		// Expression: a - b - c
		Expression* e1 = expr_constant_int_new(a);
		Expression* e2 = expr_constant_int_new(b);
		Expression* e3 = expr_rich_int_new(OP_ARITH_SUB, e1, e2);
		Expression* e4 = expr_constant_int_new(c);
		Expression* root = expr_rich_int_new(OP_ARITH_SUB, e3, e4);

		g_assert(e1);
		g_assert(e2);
		g_assert(e3);
		g_assert(e4);
		g_assert(root);

		/* evaluate */
		ExpressionStatus status;
		glong result = expr_evaluate_to_int(root, &status);

		g_message("Status: %s", expr_status_to_string(status));
		g_assert(status == STATUS_EVAL_OK);
		g_assert_cmpint(result, ==, VALID_RESULT);

		//expr_free(root);
	}
}

void test_evaluator_rich_int_op_mul()
{
	int i, rep = 100;
	for (i=0; i<rep; i++) {
		const gint32 a = g_test_rand_int_range(-100, 100);
		const gint32 b = g_test_rand_int_range(-100, 100);
		const gint32 c = g_test_rand_int_range(-100, 100);
		const glong VALID_RESULT = a*b*c;

		/* create expression tree */
		// Expression: a * b * c
		Expression* e1 = expr_constant_int_new(a);
		Expression* e2 = expr_constant_int_new(b);
		Expression* e3 = expr_rich_int_new(OP_ARITH_MUL, e1, e2);
		Expression* e4 = expr_constant_int_new(c);
		Expression* root = expr_rich_int_new(OP_ARITH_MUL, e3, e4);

		g_assert(e1);
		g_assert(e2);
		g_assert(e3);
		g_assert(e4);
		g_assert(root);

		/* evaluate */
		ExpressionStatus status;
		glong result = expr_evaluate_to_int(root, &status);

		g_message("Status: %s", expr_status_to_string(status));
		g_assert(status == STATUS_EVAL_OK);
		g_assert_cmpint(result, ==, VALID_RESULT);

		//expr_free(root);
	}
}

void test_evaluator_rich_int_op_div()
{
	int i, rep = 100;
	for (i=0; i<rep; i++) {
		const gint32 a = g_test_rand_int_range(-1000, 1000);
		const gint32 b = g_test_rand_int_range(1, 8);
		const gint32 c = g_test_rand_int_range(1, 5);
		const glong VALID_RESULT = a/b/c;

		/* create expression tree */
		// Expression: a / b / c
		Expression* e1 = expr_constant_int_new(a);
		Expression* e2 = expr_constant_int_new(b);
		Expression* e3 = expr_rich_int_new(OP_ARITH_DIV, e1, e2);
		Expression* e4 = expr_constant_int_new(c);
		Expression* root = expr_rich_int_new(OP_ARITH_DIV, e3, e4);

		g_assert(e1);
		g_assert(e2);
		g_assert(e3);
		g_assert(e4);
		g_assert(root);

		/* evaluate */
		ExpressionStatus status;
		glong result = expr_evaluate_to_int(root, &status);

		g_message("Status: %s", expr_status_to_string(status));
		//g_message("%d/%d/%d = %d == %d ?", a, b, c, result, VALID_RESULT);
		g_assert(status == STATUS_EVAL_OK);
		g_assert_cmpint(result, ==, VALID_RESULT);

		//expr_free(root);
	}
}

void test_evaluator_rich_int_op_pow()
{
	int i, rep = 100;
	for (i=0; i<rep; i++) {
		const gint32 a = g_test_rand_int_range(-2, 2);
		const gint32 b = g_test_rand_int_range(-2, 2);
		const gint32 c = g_test_rand_int_range(-2, 2);
		const glong VALID_RESULT = (glong) pow(a, (glong) pow(b, c));

		/* create expression tree */
		// Expression: a ^ b ^ c
		Expression* e1 = expr_constant_int_new(a);
		Expression* e2 = expr_constant_int_new(b);
		Expression* e3 = expr_rich_int_new(OP_ARITH_POW, e1, e2);
		Expression* e4 = expr_constant_int_new(c);
		Expression* root = expr_rich_int_new(OP_ARITH_POW, e4, e3);

		g_assert(e1);
		g_assert(e2);
		g_assert(e3);
		g_assert(e4);
		g_assert(root);

		/* evaluate */
		ExpressionStatus status;
		glong result = expr_evaluate_to_int(root, &status);

		g_message("Status: %s", expr_status_to_string(status));
		g_assert(status == STATUS_EVAL_OK);
		g_assert_cmpint(result, ==, VALID_RESULT);

		//expr_free(root);
	}
}

void test_evaluator_rich_int_op_mod()
{
	int i, rep = 100;
	for (i=0; i<rep; i++) {
		const gint32 a = g_test_rand_int_range(-G_MAXINT32, G_MAXINT32);
		const gint32 b = g_test_rand_int_range(-G_MAXINT32, G_MAXINT32);
		const glong VALID_RESULT = a % b;

		/* create expression tree */
		// Expression: a % b
		Expression* e1 = expr_constant_int_new(a);
		Expression* e2 = expr_constant_int_new(b);
		Expression* root = expr_rich_int_new(OP_ARITH_MOD, e1, e2);

		g_assert(e1);
		g_assert(e2);
		g_assert(root);

		/* evaluate */
		ExpressionStatus status;
		glong result = expr_evaluate_to_int(root, &status);

		g_message("Status: %s", expr_status_to_string(status));
		g_assert(status == STATUS_EVAL_OK);
		g_assert_cmpint(result, ==, VALID_RESULT);

		//expr_free(root);
	}
}

void test_evaluator_rich_int_op_and()
{
	int i, rep = 100;
	for (i=0; i<rep; i++) {
		const gint32 a = g_test_rand_int_range(-G_MAXINT32, G_MAXINT32);
		const gint32 b = g_test_rand_int_range(-G_MAXINT32, G_MAXINT32);
		const glong VALID_RESULT = a & b;

		/* create expression tree */
		// Expression: a & b
		Expression* e1 = expr_constant_int_new(a);
		Expression* e2 = expr_constant_int_new(b);
		Expression* root = expr_rich_int_new(OP_BIN_AND, e1, e2);

		g_assert(e1);
		g_assert(e2);
		g_assert(root);

		/* evaluate */
		ExpressionStatus status;
		glong result = expr_evaluate_to_int(root, &status);

		g_message("Status: %s", expr_status_to_string(status));
		g_assert(status == STATUS_EVAL_OK);
		g_assert_cmpint(result, ==, VALID_RESULT);

		//expr_free(root);
	}
}

void test_evaluator_rich_int_op_or()
{
	int i, rep = 100;
	for (i=0; i<rep; i++) {
		const gint32 a = g_test_rand_int_range(-G_MAXINT32, G_MAXINT32);
		const gint32 b = g_test_rand_int_range(-G_MAXINT32, G_MAXINT32);
		const glong VALID_RESULT = a | b;

		/* create expression tree */
		// Expression: a | b
		Expression* e1 = expr_constant_int_new(a);
		Expression* e2 = expr_constant_int_new(b);
		Expression* root = expr_rich_int_new(OP_BIN_OR, e1, e2);

		g_assert(e1);
		g_assert(e2);
		g_assert(root);

		/* evaluate */
		ExpressionStatus status;
		glong result = expr_evaluate_to_int(root, &status);

		g_message("Status: %s", expr_status_to_string(status));
		g_assert(status == STATUS_EVAL_OK);
		g_assert_cmpint(result, ==, VALID_RESULT);

		//expr_free(root);
	}
}

void test_evaluator_rich_int_op_mixed()
{
	int i, rep = 100;
	for (i=0; i<rep; i++) {
		const gint32 a = g_test_rand_int_range(-7, 7);
		const gint32 b = g_test_rand_int_range(-8, 8);
		const gint32 c = g_test_rand_int_range(-5, 5);
		const gint32 d = g_test_rand_int_range(-7, 7);
		const gint32 e = g_test_rand_int_range(-3, 3);
		const gint32 f = g_test_rand_int_range(-4, 4);
		const glong VALID_RESULT = ((a + b) * c - d) ^ e / f;

		/* create expression tree */
		// Expression: ((a + b) * c - d) ^ e / f
		Expression* e1 = expr_constant_int_new(a);
		Expression* e2 = expr_constant_int_new(b);
		Expression* e3 = expr_rich_int_new(OP_ARITH_ADD, e1, e2);
		Expression* e4 = expr_constant_int_new(c);
		Expression* e5 = expr_rich_int_new(OP_ARITH_MUL, e3, e4);
		Expression* e6 = expr_constant_int_new(d);
		Expression* e7 = expr_rich_int_new(OP_ARITH_SUB, e5, e6);
		Expression* e8 = expr_constant_int_new(e);
		Expression* e9 = expr_rich_int_new(OP_ARITH_POW, e7, e8);
		Expression* e10 = expr_constant_int_new(f);
		Expression* root = expr_rich_int_new(OP_ARITH_DIV, e9, e10);

		g_assert(e1);
		g_assert(e2);
		g_assert(e3);
		g_assert(e4);
		g_assert(e5);
		g_assert(e6);
		g_assert(e7);
		g_assert(e8);
		g_assert(e9);
		g_assert(e10);
		g_assert(root);

		/* evaluate */
		ExpressionStatus status;
		glong result = expr_evaluate_to_int(root, &status);

		g_message("Status: %s", expr_status_to_string(status));
		g_assert(status == STATUS_EVAL_OK);
		g_assert_cmpint(result, ==, VALID_RESULT);

		//expr_free(root);
	}
}

void test_evaluator_constant_int()
{
	int i, rep = 100;
	for (i=0; i<rep; i++) {
		const gint32 a = g_test_rand_int_range(-G_MAXINT32, -1);
		const gint32 b = g_test_rand_int_range(1, G_MAXINT32);

		/* create expressions */
		Expression* e1 = expr_constant_int_new(G_MINLONG);
		Expression* e2 = expr_constant_int_new(G_MAXLONG);
		Expression* e3 = expr_constant_int_new(0);
		Expression* e4 = expr_constant_int_new(a);
		Expression* e5 = expr_constant_int_new(b);

		g_assert(e1);
		g_assert(e2);
		g_assert(e3);
		g_assert(e4);
		g_assert(e5);

		/* evaluate */
		ExpressionStatus status[5];
		glong result1 = expr_evaluate_to_int(e1, &status[0]);
		glong result2 = expr_evaluate_to_int(e2, &status[1]);
		glong result3 = expr_evaluate_to_int(e3, &status[2]);
		glong result4 = expr_evaluate_to_int(e4, &status[3]);
		glong result5 = expr_evaluate_to_int(e5, &status[4]);

		g_message("Status: %s", expr_status_to_string(status[0]));
		g_message("Status: %s", expr_status_to_string(status[1]));
		g_message("Status: %s", expr_status_to_string(status[2]));
		g_message("Status: %s", expr_status_to_string(status[3]));
		g_message("Status: %s", expr_status_to_string(status[4]));
		g_assert(status[0] == STATUS_EVAL_OK);
		g_assert(status[1] == STATUS_EVAL_OK);
		g_assert(status[2] == STATUS_EVAL_OK);
		g_assert(status[3] == STATUS_EVAL_OK);
		g_assert(status[4] == STATUS_EVAL_OK);
		g_assert_cmpint(result1, ==, G_MINLONG);
		g_assert_cmpint(result2, ==, G_MAXLONG);
		g_assert_cmpint(result3, ==, 0);
		g_assert_cmpint(result4, ==, a);
		g_assert_cmpint(result5, ==, b);
	}
}

void test_evaluator_string_crosseval_constant_int()
{
	/* create expression */
	const gchar* VALID_RESULT = "1250486";
	Expression* e1 = expr_constant_int_new(1250486);

	g_assert(e1);

	/* evaluate */
	ExpressionStatus status;
	gchar* result = expr_evaluate_to_string(e1, &status);

	g_message("Status: %s", expr_status_to_string(status));
	g_assert(status == STATUS_EVAL_OK);
	g_assert_cmpstr(result, ==, VALID_RESULT);
}

void test_evaluator_string_crosseval_rich_int()
{
	/* create expression tree */
	// Expression: 1 + 2 + 3
	const gchar* VALID_RESULT = "6";
	Expression* e1 = expr_constant_int_new(1);
	Expression* e2 = expr_constant_int_new(2);
	Expression* e3 = expr_rich_int_new(OP_ARITH_ADD, e1, e2);
	Expression* e4 = expr_constant_int_new(3);
	Expression* root = expr_rich_int_new(OP_ARITH_ADD, e3, e4);

	g_assert(e1);
	g_assert(e2);
	g_assert(e3);
	g_assert(e4);
	g_assert(root);

	/* evaluate */
	ExpressionStatus status;
	gchar* result = expr_evaluate_to_string(root, &status);

	g_message("Status: %s", expr_status_to_string(status));
	g_assert(status == STATUS_EVAL_OK);
	g_assert_cmpstr(result, ==, VALID_RESULT);
}

void test_evaluator_constant_string()
{
	/* create expression tree */
	// Expression: 1 + 2 + 3
	const gchar* VALID_RESULT = "this is a test string";
	Expression* e1 = expr_constant_string_new("this is a test string");

	g_assert(e1);

	/* evaluate */
	ExpressionStatus status;
	gchar* result = expr_evaluate_to_string(e1, &status);

	g_message("Status: %s", expr_status_to_string(status));
	g_assert(status == STATUS_EVAL_OK);
	g_assert_cmpstr(result, ==, VALID_RESULT);
}

void test_evaluator_handle()
{
	GString* fname = g_string_new("testfile_");
	g_string_append_printf(fname, "%d", ABS(g_test_rand_int()));

	create_file(fname->str);

	g_message("File \"%s\" must exist to pass the test!", fname->str);

	iiInit(NULL);
	const gchar* fhname = "fh";
	FILE* fh = g_fopen(fname->str, "r");
	File* fhset = file_new(FILE_STDIO, &fh);
	var_set_value(fhname, VAR_FILE, &fhset);

	g_assert(fhset);

	ExpressionStatus status;
	Expression* e = expr_variable_new(fhname);
	File* fhget = expr_evaluate_to_handle(e, &status);

	g_message("Status: %s", expr_status_to_string(status));
	g_assert(status == STATUS_EVAL_OK);
	g_assert(fhget);
	g_assert(fhset == fhget);

	fclose(fh);
	iiFree();

	delete_file(fname->str);
}

int main(int argc, char *argv[])
{
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/Evaluator/Integer Evaluation - FAC Operator", test_evaluator_unary_int_op_fac);
	g_test_add_func("/Evaluator/Integer Evaluation - ADD Operator", test_evaluator_rich_int_op_add);
	g_test_add_func("/Evaluator/Integer Evaluation - SUB Operator", test_evaluator_rich_int_op_sub);
	g_test_add_func("/Evaluator/Integer Evaluation - MUL Operator", test_evaluator_rich_int_op_mul);
	g_test_add_func("/Evaluator/Integer Evaluation - DIV Operator", test_evaluator_rich_int_op_div);
	g_test_add_func("/Evaluator/Integer Evaluation - POW Operator", test_evaluator_rich_int_op_pow);
	g_test_add_func("/Evaluator/Integer Evaluation - MOD Operator", test_evaluator_rich_int_op_mod);
	g_test_add_func("/Evaluator/Integer Evaluation - AND Operator", test_evaluator_rich_int_op_and);
	g_test_add_func("/Evaluator/Integer Evaluation - OR Operator",  test_evaluator_rich_int_op_or);
	g_test_add_func("/Evaluator/Integer Evaluation - Mixed Operators", test_evaluator_rich_int_op_mixed);
	g_test_add_func("/Evaluator/Integer Constants (MaxMin Long)", test_evaluator_constant_int);
	g_test_add_func("/Evaluator/String Crossevaluation - Rich Integer", test_evaluator_string_crosseval_rich_int);
	g_test_add_func("/Evaluator/String Crossevaluation - Constant Integer", test_evaluator_string_crosseval_constant_int);
	g_test_add_func("/Evaluator/String Constants", test_evaluator_constant_string);
	g_test_add_func("/Evaluator/Handle Evaluation", test_evaluator_handle);

	return g_test_run();
}
