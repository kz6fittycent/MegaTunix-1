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
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/poll.h>
#include <defines.h>
#include <protos.h>
#include <globals.h>

static GtkWidget *ms_reset_entry;	/* MS reset count */
static GtkWidget *ms_sioerr_entry;	/* MS Serial I/O error count */
static GtkWidget *ms_readcount_entry;	/* MS Good read counter */
static GtkWidget *ms_ve_readcount_entry;	/* MS Good read counter */
gint ser_context_id;			/* for ser_statbar */
GtkWidget *ser_statbar;			/* serial statusbar */ 
extern int read_wait_time;
extern int raw_reader_running;
extern int ms_reset_count;
extern int ms_goodread_count;
extern int ms_ve_goodread_count;
int poll_min;
int poll_step;
int poll_max;
int interval_min;
int interval_step;
int interval_max;

int build_comms(GtkWidget *parent_frame)
{
	extern GtkTooltips *tip;
	GtkWidget *frame;
	GtkWidget *hbox;
	GtkWidget *hbox2;
	GtkWidget *vbox;
	GtkWidget *vbox2;
	GtkWidget *table;
	GtkWidget *button;
	GtkWidget *label;
	GtkWidget *spinner;
	GtkAdjustment *adj;

	vbox = gtk_vbox_new(FALSE,0);
	gtk_container_add(GTK_CONTAINER(parent_frame),vbox);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);

	frame = gtk_frame_new("Serial Status Messages");
	gtk_box_pack_end(GTK_BOX(vbox),frame,FALSE,FALSE,0);

	vbox2 = gtk_vbox_new(FALSE,0);
	gtk_container_add(GTK_CONTAINER(frame),vbox2);
	gtk_container_set_border_width(GTK_CONTAINER(vbox2),5);

	ser_statbar = gtk_statusbar_new();
	gtk_statusbar_set_has_resize_grip(GTK_STATUSBAR(ser_statbar),FALSE);
	gtk_box_pack_start(GTK_BOX(vbox2),ser_statbar,TRUE,TRUE,0);
	ser_context_id = gtk_statusbar_get_context_id(
			GTK_STATUSBAR(ser_statbar),
			"Serial Status");

	hbox = gtk_hbox_new(TRUE,5);
	gtk_box_pack_start(GTK_BOX(vbox),hbox,FALSE,FALSE,0);

	frame = gtk_frame_new("Select Communications Port");
	gtk_box_pack_start(GTK_BOX(hbox),frame,FALSE,TRUE,0);

	hbox2 = gtk_hbox_new(TRUE,0);
	gtk_container_add(GTK_CONTAINER(frame),hbox2);

	table = gtk_table_new(2,3,FALSE);
	gtk_table_set_col_spacings(GTK_TABLE(table),10);
	gtk_container_set_border_width(GTK_CONTAINER(table),5);
	gtk_box_pack_start(GTK_BOX(hbox2),table,FALSE,TRUE,20);


	label = gtk_label_new("Communications Port");
	gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
                        (GtkAttachOptions) (GTK_FILL),
                        (GtkAttachOptions) (0), 0, 0);

	adj = (GtkAdjustment *) gtk_adjustment_new(1,1,8,1,1,0);
	spinner = gtk_spin_button_new(adj,0,0);
	gtk_tooltips_set_tip(tip,spinner,
	"Sets the comm port to use.  MegaTunix use DOS/Win32 style 
	port numbers like 1 for COM1, 2 for COM2 and so on.\n",NULL);
	gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinner), FALSE);
	g_signal_connect (G_OBJECT(spinner), "value_changed",
			G_CALLBACK (spinner_changed),
			GINT_TO_POINTER(SET_SER_PORT));
	gtk_adjustment_set_value(GTK_ADJUSTMENT(adj),serial_params.comm_port);
	gtk_table_attach (GTK_TABLE (table), spinner, 1, 2, 0, 1,
                        (GtkAttachOptions) (GTK_EXPAND),
                        (GtkAttachOptions) (0), 0, 0);

	frame = gtk_frame_new("Verify ECU Communication");
	gtk_box_pack_start(GTK_BOX(hbox),frame,FALSE,TRUE,0);

	hbox2 = gtk_hbox_new(TRUE,0);
	gtk_container_add(GTK_CONTAINER(frame),hbox2);
	gtk_container_set_border_width(GTK_CONTAINER(hbox2),5);
	button = gtk_button_new_with_label("Test ECU Communication...");
	gtk_tooltips_set_tip(tip,button,
	"Attempts to communicate with the MegaSquirt Controller.  
	Check the statusbar at the bottom of the window for the 
	results of this test.\n",NULL);
	gtk_box_pack_start(GTK_BOX(hbox2),button,FALSE,FALSE,0);
	g_signal_connect(G_OBJECT (button), "clicked",
			G_CALLBACK (check_ecu_comms), \
			NULL);

	frame = gtk_frame_new("Runtime Parameters Configuration");
	gtk_box_pack_start(GTK_BOX(vbox),frame,FALSE,FALSE,0);

	hbox = gtk_hbox_new(TRUE,0);
	gtk_container_add(GTK_CONTAINER(frame),hbox);

	table = gtk_table_new(2,3,FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(table),10);
	gtk_container_set_border_width(GTK_CONTAINER(table),5);
	gtk_box_pack_start(GTK_BOX(hbox),table,FALSE,TRUE,20);


	label = gtk_label_new("Polling Timeout (ms)");
	gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_CENTER);
	gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
                        (GtkAttachOptions) (GTK_FILL),
                        (GtkAttachOptions) (0), 0, 0);

	adj = (GtkAdjustment *) gtk_adjustment_new(
			100,poll_min,poll_max,poll_step,poll_step,0);
	spinner = gtk_spin_button_new(adj,0,0);
	gtk_tooltips_set_tip(tip,spinner,
	"Sets the time delay when waiting from data from the MS, 
	typically should be set under 100 milliseconds. This 
	partially determines the max rate at which RT variable 
	can be read from the MS box.\n",NULL);
	g_signal_connect (G_OBJECT(spinner), "value_changed",
			G_CALLBACK (spinner_changed),
			GINT_TO_POINTER(SER_POLL_TIMEO));
	gtk_adjustment_set_value(GTK_ADJUSTMENT(adj),serial_params.poll_timeout);
	gtk_table_attach (GTK_TABLE (table), spinner, 1, 2, 0, 1,
                        (GtkAttachOptions) (GTK_EXPAND),
                        (GtkAttachOptions) (0), 0, 0);

	button = gtk_button_new_with_label("Start Reading RT Vars");
	gtk_tooltips_set_tip(tip,button,
	"Starts reading the RT variables from the MS.  This will 
	cause the Runtime Disp. page to begin updating continuously.",NULL);
	g_signal_connect(G_OBJECT (button), "clicked",
			G_CALLBACK (std_button_handler), \
			GINT_TO_POINTER(START_REALTIME));
	gtk_table_attach (GTK_TABLE (table), button, 2, 3, 0, 1,
                        (GtkAttachOptions) (GTK_EXPAND),
                        (GtkAttachOptions) (0), 0, 0);

	label = gtk_label_new("Serial Interval Delay\nBetween Reads(ms)");
	gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_CENTER);
	gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
                        (GtkAttachOptions) (GTK_FILL),
                        (GtkAttachOptions) (0), 0, 0);

	adj = (GtkAdjustment *) gtk_adjustment_new(
			100,interval_min,interval_max,
			interval_step,interval_step,0);
	spinner = gtk_spin_button_new(adj,0,0);
	gtk_tooltips_set_tip(tip,spinner,
	"Sets the time delay between read attempts for getting 
	the RT variables from the MS, typically should be set 
	around 50 for about 12-18 reads per second from the MS. 
	This will control the rate at which the Runtime Display 
	page updates.\n",NULL);
	g_signal_connect (G_OBJECT(spinner), "value_changed",
			G_CALLBACK (spinner_changed),
			GINT_TO_POINTER(SER_INTERVAL_DELAY));
	gtk_adjustment_set_value(GTK_ADJUSTMENT(adj),serial_params.read_wait);
	gtk_table_attach (GTK_TABLE (table), spinner, 1, 2, 1, 2,
                        (GtkAttachOptions) (GTK_EXPAND),
                        (GtkAttachOptions) (0), 0, 0);

	button = gtk_button_new_with_label("Stop Reading RT vars");
	gtk_tooltips_set_tip(tip,button,
	"Stops reading the RT variables from the MS.  NOTE: you 
	don't have to stop reading the RT vars to read the 
	VEtable and Constants.  It is handled automatically for you.\n",NULL);
	g_signal_connect(G_OBJECT (button), "clicked",
			G_CALLBACK (std_button_handler), \
			GINT_TO_POINTER(STOP_REALTIME));
	gtk_table_attach (GTK_TABLE (table), button, 2, 3, 1, 2,
                        (GtkAttachOptions) (GTK_EXPAND),
                        (GtkAttachOptions) (0), 0, 0);

	frame = gtk_frame_new("MegaSquirt I/O Status");
	gtk_box_pack_start(GTK_BOX(vbox),frame,FALSE,TRUE,0);

	hbox = gtk_hbox_new(TRUE,0);
	gtk_container_add(GTK_CONTAINER(frame),hbox);

	table = gtk_table_new(3,4,FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(table),5);
	gtk_table_set_col_spacings(GTK_TABLE(table),5);
	gtk_container_set_border_width(GTK_CONTAINER(table),5);
	gtk_box_pack_start(GTK_BOX(hbox),table,FALSE,TRUE,20);

	label = gtk_label_new("Good VE/Constants Reads");
	gtk_misc_set_alignment(GTK_MISC(label),0.0,0.5);
	gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
                        (GtkAttachOptions) (GTK_FILL),
                        (GtkAttachOptions) (0), 0, 0);

	ms_ve_readcount_entry = gtk_entry_new();
	gtk_entry_set_width_chars (GTK_ENTRY (ms_ve_readcount_entry), 5);
	gtk_table_attach (GTK_TABLE (table), ms_ve_readcount_entry, 1, 2, 0, 1,
                        (GtkAttachOptions) (GTK_EXPAND),
                        (GtkAttachOptions) (0), 0, 0);

     
	label = gtk_label_new("Good RealTime Reads");
	gtk_misc_set_alignment(GTK_MISC(label),0.0,0.5);
	gtk_table_attach (GTK_TABLE (table), label, 2, 3, 0, 1,
                        (GtkAttachOptions) (GTK_FILL),
                        (GtkAttachOptions) (GTK_FILL), 0, 0);
     
	ms_readcount_entry = gtk_entry_new();
	gtk_entry_set_width_chars (GTK_ENTRY (ms_readcount_entry), 5);
	gtk_table_attach (GTK_TABLE (table), ms_readcount_entry, 3, 4, 0, 1,
                        (GtkAttachOptions) (GTK_EXPAND),
                        (GtkAttachOptions) (0), 0, 0);

	label = gtk_label_new("Hard Reset Count");
	gtk_misc_set_alignment(GTK_MISC(label),0.0,0.5);
	gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
                        (GtkAttachOptions) (GTK_FILL),
                        (GtkAttachOptions) (GTK_FILL), 0, 0);

	ms_reset_entry = gtk_entry_new();
	gtk_entry_set_width_chars (GTK_ENTRY (ms_reset_entry), 5);
	gtk_table_attach (GTK_TABLE (table), ms_reset_entry, 1, 2, 1, 2,
                        (GtkAttachOptions) (GTK_EXPAND),
                        (GtkAttachOptions) (0), 0, 0);

	label = gtk_label_new("Serial I/O Error Count");
	gtk_misc_set_alignment(GTK_MISC(label),0.0,0.5);
	gtk_table_attach (GTK_TABLE (table), label, 2, 3, 1, 2,
                        (GtkAttachOptions) (GTK_FILL),
                        (GtkAttachOptions) (GTK_FILL), 0, 0);

	ms_sioerr_entry = gtk_entry_new();
	gtk_entry_set_width_chars (GTK_ENTRY (ms_sioerr_entry), 5);
	gtk_table_attach (GTK_TABLE (table), ms_sioerr_entry, 3, 4, 1, 2,
                        (GtkAttachOptions) (GTK_EXPAND),
                        (GtkAttachOptions) (0), 0, 0);

	gtk_widget_show_all(vbox);
	return(0);
}

void update_errcounts()
{
	char buff[10];

	g_snprintf(buff,10,"%i",ms_ve_goodread_count);
	gtk_entry_set_text(GTK_ENTRY(ms_ve_readcount_entry),buff);

	g_snprintf(buff,10,"%i",ms_goodread_count);
	gtk_entry_set_text(GTK_ENTRY(ms_readcount_entry),buff);

	g_snprintf(buff,10,"%i",ms_reset_count);
	gtk_entry_set_text(GTK_ENTRY(ms_reset_entry),buff);

	g_snprintf(buff,10,"%i",serial_params.errcount);
	gtk_entry_set_text(GTK_ENTRY(ms_sioerr_entry),buff);

	return;
}
