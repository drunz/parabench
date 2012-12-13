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

#ifndef AST_H_
#define AST_H_

#include <glib.h>


GNode* ast;


void ast_init();
void ast_free();

///**
// * Adds statement to existing node sibling as a sibling node.
// */
//GNode* ast_add_statement(GNode* sibling, Statement* statement);
//
///**
// * Adds statement to existing node parent as a child node.
// */
//GNode* ast_add_block(GNode* parent, Statement* statement);


#endif /* AST_H_ */
