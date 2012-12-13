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

#include "ast.h"
#include "statements.h"

void ast_init()
{
	ast = NULL;
}

gboolean data_free_func(GNode *node, gpointer data)
{
	stmt_free(node->data);
	return FALSE;
}

void ast_free()
{
	if (ast) {
		g_node_traverse(ast, G_POST_ORDER, G_TRAVERSE_ALL, -1, data_free_func, NULL);
		g_node_destroy(ast);
	}
}
