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
#include <debugging.h>
#include <defines.h>
#include <enums.h>
#include <ms2_gui_handlers.h>
#include <firmware.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include <user_outputs.h>


gconstpointer *global_data;


G_MODULE_EXPORT gboolean ecu_entry_handler(GtkWidget *widget, gpointer data)
{
	dbg_func_f(CRITICAL,g_strdup(__FILE__": ecu_entry_handler()\n\tERROR handler NOT found, command aborted! BUG!!!\n"));
	return TRUE;

}


G_MODULE_EXPORT gboolean ecu_button_handler(GtkWidget *widget, gpointer data)
{
	dbg_func_f(CRITICAL,g_strdup(__FILE__": ecu_button_handler()\n\tERROR handler NOT found, command aborted! BUG!!!\n"));
	return TRUE;
}


G_MODULE_EXPORT gboolean ecu_combo_handler(GtkWidget *widget, gpointer data)
{
	GtkTreeIter iter;
	GtkTreeModel *model = NULL;
	gboolean state = FALSE;
	gint handler = 0;
	gint bitmask = 0;
	gint bitshift = 0;
	gint total = 0;
	guchar bitval = 0;
	gchar * set_labels = NULL;
	gchar * swap_list = NULL;
	gchar * tmpbuf = NULL;
	gchar * table_2_update = NULL;
	gchar * group_2_update = NULL;
	gchar * lower = NULL;
	gchar * upper = NULL;
	gchar * dl_conv = NULL;
	gchar * ul_conv = NULL;
	gint precision = 0;
	gchar ** vector = NULL;
	guint i = 0;
	gint tmpi = 0;
	gint page = 0;
	gint offset = 0;
	gint canID = 0;
	gint table_num = 0;
	gchar * range = NULL;
	DataSize size = MTX_U08;
	gchar * choice = NULL;
	guint8 tmp = 0;
	gint dload_val = 0;
	gint dl_type = 0;
	gfloat tmpf = 0.0;
	gfloat tmpf2 = 0.0;
	Deferred_Data *d_data = NULL;
	GtkWidget *tmpwidget = NULL;
	void *eval = NULL;
	GHashTable **interdep_vars = NULL;
	GHashTable *sources_hash = NULL;
	Firmware_Details *firmware = NULL;
	void (*check_limits)(gint) = NULL;

	firmware = DATA_GET(global_data,"firmware");
	handler = (GINT)OBJ_GET(widget,"handler");
	dl_type = (GINT)OBJ_GET(widget,"dl_type");
	page = (GINT)OBJ_GET(widget,"page");
	offset = (GINT)OBJ_GET(widget,"offset");
	canID = (GINT)OBJ_GET(widget,"canID");
	if (!OBJ_GET(widget,"size"))
		size = MTX_U08 ;        /* default! */
	else
		size = (DataSize)OBJ_GET(widget,"size");
	bitval = (GINT)OBJ_GET(widget,"bitval");
	bitmask = (GINT)OBJ_GET(widget,"bitmask");
	bitshift = get_bitshift_f(bitmask);


	switch (handler)
	{
		case MS2_USER_OUTPUTS:
			/* Send the offset */
			tmp = ms_get_ecu_data_f(canID,page,offset,size);
			tmp = tmp & ~bitmask;   /*clears bits */
			tmp = tmp | (bitval << bitshift);
			ms_send_to_ecu_f(canID, page, offset, size, tmp, TRUE);
			/* Get the rest of the data from the combo */
			gtk_tree_model_get(model,&iter,UO_SIZE_COL,&size,UO_LOWER_COL,&lower,UO_UPPER_COL,&upper,UO_RANGE_COL,&range,UO_PRECISION_COL,&precision,UO_DL_CONV_COL,&dl_conv,UO_UL_CONV_COL,&ul_conv,-1);

			/* Send the "size" of the offset to the ecu */
			offset = (gint)strtol(OBJ_GET(widget,"size_offset"),NULL,10);
			ms_send_to_ecu_f(canID, page, offset, MTX_U08,size, TRUE);

			tmpbuf = (gchar *)OBJ_GET(widget,"range_label");
			if (tmpbuf)
				tmpwidget = lookup_widget_f(tmpbuf);
			if (GTK_IS_LABEL(tmpwidget))
				gtk_label_set_text(GTK_LABEL(tmpwidget),range);

			tmpbuf = (gchar *)OBJ_GET(widget,"thresh_widget");
			if (tmpbuf)
				tmpwidget = lookup_widget_f(tmpbuf);
			if (GTK_IS_WIDGET(tmpwidget))
			{
				eval = NULL;
				eval = OBJ_GET(tmpwidget,"dl_evaluator");
				if (eval)
				{
					evaluator_destroy_f(eval);
					OBJ_SET(tmpwidget,"dl_evaluator",NULL);
					eval = NULL;
				}
				if (dl_conv)
				{
					eval = evaluator_create_f(dl_conv);
					OBJ_SET(tmpwidget,"dl_evaluator",eval);
					if (upper)
					{
						tmpf2 = g_ascii_strtod(upper,NULL);
						tmpf = evaluator_evaluate_x_f(eval,tmpf2);
						tmpbuf = OBJ_GET(tmpwidget,"raw_upper");
						if (tmpbuf)
							g_free(tmpbuf);
						OBJ_SET(tmpwidget,"raw_upper",g_strdup_printf("%f",tmpf));
						/*printf("combo_handler thresh has dl conv expr and upper limit of %f\n",tmpf);*/
					}
					if (lower)
					{
						tmpf2 = g_ascii_strtod(lower,NULL);
						tmpf = evaluator_evaluate_x_f(eval,tmpf2);
						tmpbuf = OBJ_GET(tmpwidget,"raw_lower");
						if (tmpbuf)
							g_free(tmpbuf);
						OBJ_SET(tmpwidget,"raw_lower",g_strdup_printf("%f",tmpf));
						/*printf("combo_handler thresh has dl conv expr and lower limit of %f\n",tmpf);*/
					}
				}
				else
					OBJ_SET(tmpwidget,"raw_upper",upper);

				eval = NULL;
				eval = OBJ_GET(tmpwidget,"ul_evaluator");
				if (eval)
				{
					evaluator_destroy_f(eval);
					OBJ_SET(tmpwidget,"ul_evaluator",NULL);
					eval = NULL;
				}
				if (ul_conv)
				{
					eval = evaluator_create_f(ul_conv);
					OBJ_SET(tmpwidget,"ul_evaluator",eval);
				}
				OBJ_SET(tmpwidget,"size",GINT_TO_POINTER(size));
				OBJ_SET(tmpwidget,"dl_conv_expr",dl_conv);
				OBJ_SET(tmpwidget,"ul_conv_expr",ul_conv);
				OBJ_SET(tmpwidget,"precision",GINT_TO_POINTER(precision));
				/*printf ("combo_handler thresh widget to size '%i', dl_conv '%s' ul_conv '%s' precision '%i'\n",size,dl_conv,ul_conv,precision);*/
				update_widget_f(tmpwidget,NULL);
			}
			tmpbuf = (gchar *)OBJ_GET(widget,"hyst_widget");
			if (tmpbuf)
				tmpwidget = lookup_widget_f(tmpbuf);
			if (GTK_IS_WIDGET(tmpwidget))
			{
				eval = NULL;
				eval = OBJ_GET(tmpwidget,"dl_evaluator");
				if (eval)
				{
					evaluator_destroy_f(eval);
					OBJ_SET(tmpwidget,"dl_evaluator",NULL);
					eval = NULL;
				}
				if (dl_conv)
				{
					eval = evaluator_create_f(dl_conv);
					OBJ_SET(tmpwidget,"dl_evaluator",eval);
					if (upper)
					{
						tmpf2 = g_ascii_strtod(upper,NULL);
						tmpf = evaluator_evaluate_x_f(eval,tmpf2);
						tmpbuf = OBJ_GET(tmpwidget,"raw_upper");
						if (tmpbuf)
							g_free(tmpbuf);
						OBJ_SET(tmpwidget,"raw_upper",g_strdup_printf("%f",tmpf));
						/*printf("combo_handler hyst has dl conv expr and upper limit of %f\n",tmpf);*/
					}
					if (lower)
					{
						tmpf2 = g_ascii_strtod(lower,NULL);
						tmpf = evaluator_evaluate_x_f(eval,tmpf2);
						tmpbuf = OBJ_GET(tmpwidget,"raw_lower");
						if (tmpbuf)
							g_free(tmpbuf);
						OBJ_SET(tmpwidget,"raw_lower",g_strdup_printf("%f",tmpf));
						/*printf("combo_handler hyst has dl conv expr and lower limit of %f\n",tmpf);*/
					}
				}
				else
					OBJ_SET(tmpwidget,"raw_upper",upper);

				eval = NULL;
				eval = OBJ_GET(tmpwidget,"ul_evaluator");
				if (eval)
				{
					evaluator_destroy_f(eval);
					OBJ_SET(tmpwidget,"ul_evaluator",NULL);
					eval = NULL;
				}
				if (ul_conv)
				{
					eval = evaluator_create_f(ul_conv);
					OBJ_SET(tmpwidget,"ul_evaluator",eval);
				}
				OBJ_SET(tmpwidget,"size",GINT_TO_POINTER(size));
				OBJ_SET(tmpwidget,"dl_conv_expr",dl_conv);
				OBJ_SET(tmpwidget,"ul_conv_expr",ul_conv);
				OBJ_SET(tmpwidget,"precision",GINT_TO_POINTER(precision));
				/*printf ("combo_handler hyst widget to size '%i', dl_conv '%s' ul_conv '%s' precision '%i'\n",size,dl_conv,ul_conv,precision);*/
				update_widget_f(tmpwidget,NULL);
			}
			return TRUE;
			break;

		default:
			printf("Handler is %s\n",translate_enum_f(handler));
			dbg_func_f(CRITICAL,g_strdup(__FILE__": ecu_combo_handler()\n\tdefault case reached,  i.e. handler not found in global, common or ECU plugins, BUG!\n"));
			break;
	}
	if (dl_type == IMMEDIATE)
	{
		dload_val = convert_before_download_f(widget,dload_val);
		ms_send_to_ecu_f(canID, page, offset, size, dload_val, TRUE);
	}
	return TRUE;
}
