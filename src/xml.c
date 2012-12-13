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

#include "xml.h"
#include <string.h>
#include <stdio.h>
#include <glib/gstdio.h>


static const gchar* indentString = "  ";

static XmlStackFrame* xml_new_stackframe(const gchar* name, guint level)
{
	gint stringLength = strlen(name) + 1;

	XmlStackFrame* frame = g_malloc0(sizeof(XmlStackFrame) + stringLength);
	frame->name = (gchar*) frame + sizeof(XmlStackFrame);
	frame->hasContent = FALSE;
	frame->level = level;
	memcpy(frame->name, name, stringLength);
	return frame;
}

XmlDocument* xml_document_new()
{
	XmlDocument* doc = g_malloc0(sizeof(XmlDocument));
	doc->buffer = g_string_new("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	doc->stack = NULL;
	doc->level = 1;
	return doc;
}

void xml_document_free(XmlDocument* doc)
{
	g_string_free(doc->buffer, TRUE);
	g_list_free(doc->stack);
	g_free(doc);
}

void xml_document_save(XmlDocument* doc, const gchar* fileName)
{
	FILE* fh = g_fopen(fileName, "w");
	g_fprintf(fh, "%s", doc->buffer->str);
	fclose(fh);
}

static void xml_indent(XmlDocument* doc)
{
	guint i, level = g_list_length(doc->stack);
	for (i=0; i<level; i++)
		g_string_append(doc->buffer, indentString);
}

void xml_start_element(XmlDocument* doc, const gchar* name)
{
	if (doc->stack) {
		XmlStackFrame* frame = g_list_first(doc->stack)->data;

		// if the last element has no content,
		// we can add the new element
		if (!frame->hasContent) {
			g_string_append(doc->buffer, ">\n");
			frame->hasContent = TRUE;
			doc->level++;
		}
	}

	xml_indent(doc);

	// append element
	g_string_append_printf(doc->buffer, "<%s", name);

	doc->stack = g_list_prepend(doc->stack, xml_new_stackframe(name, doc->level));
}

void xml_set_content(XmlDocument* doc, const gchar* value)
{
	if (doc->stack) {
		XmlStackFrame* frame = g_list_first(doc->stack)->data;

		if (!frame->hasContent) {
			g_string_append_printf(doc->buffer, ">%s", value);
			frame->hasContent = TRUE;
		}
	}
}

void xml_end_element(XmlDocument* doc)
{
	if (doc->stack) {
		XmlStackFrame* frame = g_list_first(doc->stack)->data;
		doc->stack = g_list_remove_link(doc->stack, g_list_first(doc->stack));

		if (frame->hasContent) {
			// indentation depends on whether the open tag
			// of the element is at the same level (line)
			// with the current level (line) in the document
			if (doc->level > frame->level)
				xml_indent(doc);

			g_string_append_printf(doc->buffer, "</%s>\n", frame->name);
		}
		else
			g_string_append(doc->buffer, "/>\n");

		doc->level++;
		g_free(frame);
	}
}

void xml_add_attribute(XmlDocument* doc, const gchar* key, const gchar* value)
{
	if (doc->stack) {
		XmlStackFrame* frame = g_list_first(doc->stack)->data;

		if (!frame->hasContent)
			g_string_append_printf(doc->buffer, " %s=\"%s\"", key, value);
	}
}

void xml_add_attribute_string(XmlDocument* doc, const gchar* key, const gchar* value)
{
	if (doc->stack) {
		XmlStackFrame* frame = g_list_first(doc->stack)->data;

		if (!frame->hasContent)
			g_string_append_printf(doc->buffer, " %s=\"%s\"", key, value);
	}
}

void xml_add_attribute_int(XmlDocument* doc, const gchar* key, gint value)
{
	if (doc->stack) {
		XmlStackFrame* frame = g_list_first(doc->stack)->data;

		if (!frame->hasContent)
			g_string_append_printf(doc->buffer, " %s=\"%d\"", key, value);
	}
}

void xml_add_attribute_long(XmlDocument* doc, const gchar* key, glong value)
{
	if (doc->stack) {
		XmlStackFrame* frame = g_list_first(doc->stack)->data;

		if (!frame->hasContent)
			g_string_append_printf(doc->buffer, " %s=\"%ld\"", key, value);
	}
}

void xml_add_attribute_double(XmlDocument* doc, const gchar* key, gdouble value)
{
	if (doc->stack) {
		XmlStackFrame* frame = g_list_first(doc->stack)->data;

		if (!frame->hasContent)
			g_string_append_printf(doc->buffer, " %s=\"%f\"", key, value);
	}
}

void xml_add_element_with_content(XmlDocument* doc, const gchar* name, const gchar* value)
{
	xml_start_element(doc, name);
	xml_set_content(doc, value);
	xml_end_element(doc);
}
