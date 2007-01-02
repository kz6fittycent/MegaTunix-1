/*
 * Copyright (C) 2003 by Dave J. Andruczyk <djandruczyk at yahoo dot com>
 *
 * Linux Megasquirt tuning software
 * 
 * 
 * This software comes under the GPL (GNU Public License)
 * You may freely copy,distribute etc. this as long as the source code
 * is made available for FREE.
 * 
 * No warranty is made or implied. You use this program at your own risk.
 */

#include <config.h>
#include <dashboard.h>
#include <debugging.h>
#include <defines.h>
#include <enums.h>
#include <getfiles.h>
#include <gauge.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <structures.h>


/*!
 \brief load_dashboard() loads hte specified dashboard configuration file
 and initializes the dash.
 \param  chooser, the fileshooser that triggered the signal
 \param data, user date
 */
void load_dashboard(GtkFileChooser *chooser, gpointer data)
{
	GtkWidget *window = NULL;
	GtkWidget *dash = NULL;
	xmlDoc *doc = NULL;
	xmlNode *root_element = NULL;
	gchar * filename = NULL;

	filename = gtk_file_chooser_get_filename(chooser);
	if (filename == NULL)
		return;

	LIBXML_TEST_VERSION

		/*parse the file and get the DOM */
		doc = xmlReadFile(filename, NULL, 0);

	if (doc == NULL)
	{
		printf("error: could not parse file %s\n",filename);
		return;
	}
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	dash = gtk_fixed_new();
	gtk_container_add(GTK_CONTAINER(window),dash);
	gtk_widget_show_all(window);

	/*Get the root element node */
	root_element = xmlDocGetRootElement(doc);
	load_elements(dash,root_element);

	link_dash_datasources(dash);

	gtk_widget_show_all(window);
}

void load_elements(GtkWidget *dash, xmlNode *a_node)
{
	xmlNode *cur_node = NULL;

	/* Iterate though all nodes... */
	for (cur_node = a_node;cur_node;cur_node = cur_node->next)
	{
		if (cur_node->type == XML_ELEMENT_NODE)
		{
			if (g_strcasecmp((gchar *)cur_node->name,"dash_geometry") == 0)
				load_geometry(dash,cur_node);
			if (g_strcasecmp((gchar *)cur_node->name,"gauge") == 0)
				load_gauge(dash,cur_node);
		}
		load_elements(dash,cur_node->children);

	}

}

void load_geometry(GtkWidget *dash, xmlNode *node)
{
	xmlNode *cur_node = NULL;
	gint width = 0;
	gint height = 0;
	if (!node->children)
	{
		printf("ERROR, load_geometry, xml node is empty!!\n");
		return;
	}
	cur_node = node->children;
	while (cur_node->next)
	{
		if (cur_node->type == XML_ELEMENT_NODE)
		{
			if (g_strcasecmp((gchar *)cur_node->name,"width") == 0)
				load_integer_from_xml(cur_node,&width);
			if (g_strcasecmp((gchar *)cur_node->name,"height") == 0)
				load_integer_from_xml(cur_node,&height);
		}
		cur_node = cur_node->next;

	}
	gtk_window_resize(GTK_WINDOW(gtk_widget_get_toplevel(dash)),width,height);

}

void load_gauge(GtkWidget *dash, xmlNode *node)
{
	xmlNode *cur_node = NULL;
	GtkWidget *gauge = NULL;
	gchar * filename = NULL;
	gint width = 0;
	gint height = 0;
	gint x_offset = 0;
	gint y_offset = 0;
	gchar *xml_name = NULL;
	gchar *datasource = NULL;
	if (!node->children)
	{
		printf("ERROR, load_gauge, xml node is empty!!\n");
		return;
	}
	cur_node = node->children;
	while (cur_node->next) { if (cur_node->type == XML_ELEMENT_NODE)
		{
			if (g_strcasecmp((gchar *)cur_node->name,"width") == 0)
				load_integer_from_xml(cur_node,&width);
			if (g_strcasecmp((gchar *)cur_node->name,"height") == 0)
				load_integer_from_xml(cur_node,&height);
			if (g_strcasecmp((gchar *)cur_node->name,"x_offset") == 0)
				load_integer_from_xml(cur_node,&x_offset);
			if (g_strcasecmp((gchar *)cur_node->name,"y_offset") == 0)
				load_integer_from_xml(cur_node,&y_offset);
			if (g_strcasecmp((gchar *)cur_node->name,"gauge_xml_name") == 0)
				load_string_from_xml(cur_node,&xml_name);
			if (g_strcasecmp((gchar *)cur_node->name,"datasource") == 0)
				load_string_from_xml(cur_node,&datasource);
		}
		cur_node = cur_node->next;

	}
	if (xml_name && datasource)
	{
		gauge = mtx_gauge_face_new();
		gtk_fixed_put(GTK_FIXED(dash),gauge,x_offset,y_offset);
		filename = get_file(g_strconcat(GAUGES_DATA_DIR,PSEP,xml_name,NULL),NULL);
		mtx_gauge_face_import_xml(MTX_GAUGE_FACE(gauge),filename);
		gtk_widget_set_usize(gauge,width,height);
		g_free(filename);
		g_object_set_data(G_OBJECT(gauge),"datasource",g_strdup(datasource));
		/* Cheat to get property window created... */
//		create_preview_list(NULL,NULL);
//		update_properties(gauge,GAUGE_ADD);
		g_free(xml_name);
		g_free(datasource);
		gtk_widget_show_all(dash);
	}

}

void load_integer_from_xml(xmlNode *node, gint *dest)
{
	if (!node->children)
	{
		printf("ERROR, load_integer_from_xml, xml node is empty!!\n");
		return;
	}
	if (!(node->children->type == XML_TEXT_NODE))
		return;
	*dest = (gint)g_ascii_strtod((gchar*)node->children->content,NULL);

}

void load_string_from_xml(xmlNode *node, gchar **dest)
{
	if (!node->children) /* EMPTY node, thus, clear the var on the gauge */
	{
		if (*dest)
			g_free(*dest);
		*dest = g_strdup("");
		return;
	}
	if (!(node->children->type == XML_TEXT_NODE))
		return;

	if (*dest)
		g_free(*dest);
	if (node->children->content)
		*dest = g_strdup((gchar*)node->children->content);
	else
		*dest = g_strdup("");

}


void link_dash_datasources(GtkWidget *dash)
{
	struct Dash_Gauge *d_gauge = NULL;
	GtkFixedChild *child;
	GList *children = NULL;
	gint len = 0;
	gint i = 0;
	GObject * rtv_obj = NULL;
	gchar * source = NULL;
	extern GHashTable *dash_gauges;
	extern struct Rtv_Map *rtv_map;

	if(!GTK_IS_FIXED(dash))
		return;
	
	if (!dash_gauges);
		dash_gauges = g_hash_table_new(g_str_hash,g_str_equal);

	children = GTK_FIXED(dash)->children;
	len = g_list_length(children);

	for (i=0;i<len;i++)
	{
		child = g_list_nth_data(children,i);
		source = (gchar *)g_object_get_data(G_OBJECT(child->widget),"datasource");
		if (!source)
			continue;

		rtv_obj = g_hash_table_lookup(rtv_map->rtv_hash,source);
		if (!G_IS_OBJECT(rtv_obj))
		{
			dbg_func(g_strdup_printf(__FILE__": link_dash_datasourcesn\tBad things man, object doesn't exist for %s\n",source),CRITICAL);
			continue ;
		}
		d_gauge = g_new0(struct Dash_Gauge, 1);
		d_gauge->object = rtv_obj;
		d_gauge->source = source;
		d_gauge->gauge = child->widget;
		g_hash_table_insert(dash_gauges,g_strdup_printf("gauge_%i",i),(gpointer)d_gauge);

	}
}

void update_dash_gauge(gpointer key, gpointer value, gpointer user_data)
{
	struct Dash_Gauge *d_gauge = (struct Dash_Gauge *)value;
	extern GStaticMutex rtv_mutex;
	GArray *history;
	gint current_index = 0;
	gfloat current = 0.0;
	gfloat previous = 0.0;
	extern gboolean forced_update;

	GtkWidget *gauge = NULL;
	
	gauge = d_gauge->gauge;

	history = (GArray *)g_object_get_data(d_gauge->object,"history");
	current_index = (gint)g_object_get_data(d_gauge->object,"current_index");
	g_static_mutex_lock(&rtv_mutex);
	current = g_array_index(history, gfloat, current_index);
	if (current_index > 0)
		current_index-=1;
	previous = g_array_index(history, gfloat, current_index);
	g_static_mutex_unlock(&rtv_mutex);

	if ((current != previous) || (forced_update))
		mtx_gauge_face_set_value(MTX_GAUGE_FACE(gauge),current);

}