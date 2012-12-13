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

#ifndef XML_H_
#define XML_H_

#include <glib.h>

/**
 * Provides a streaming like interface to properly format XML output.
 */

typedef struct {
	GString* buffer;
	GList* stack;
	guint level;	// current level (row number)
} XmlDocument;

typedef struct {
	gchar* name;
	gboolean hasContent;
	guint level;	// level of the open element
} XmlStackFrame;


XmlDocument* xml_document_new();
void xml_document_free(XmlDocument* doc);
void xml_document_save(XmlDocument* doc, const gchar* fileName);

void xml_start_element(XmlDocument* doc, const gchar* name);
void xml_set_content(XmlDocument* doc, const gchar* value);
void xml_end_element(XmlDocument* doc);

void xml_add_attribute_string(XmlDocument* doc, const gchar* key, const gchar* value);
void xml_add_attribute_int(XmlDocument* doc, const gchar* key, gint value);
void xml_add_attribute_long(XmlDocument* doc, const gchar* key, glong value);
void xml_add_attribute_double(XmlDocument* doc, const gchar* key, gdouble value);

void xml_add_element_with_content(XmlDocument* doc, const gchar* name, const gchar* value);

#endif /* XML_H_ */
