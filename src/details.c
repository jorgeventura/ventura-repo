/*
 * details.c
 *
 * Populate the details tab with useful information.
 *
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "callbacks.h"
#include "interface.h"
#include "details.h"

static details_t details;

/* Enumerations from the last selected node */

static struct enum_list *enums;

/* Need this to format sets.... */

static char *hint;

/* Find a value from an enum based on label */

static size_t min_size_t(size_t a, size_t b) {
  if (a > b)
    return b;
  else
    return a;
}

const char *find_val_from_enum(const char *label) {
  
  int len;
  const char *p;
  struct enum_list *e;
  static char ret[10];

  if (enums == NULL)
    return(NULL);
  while(*label == ' ' || *label == '\t')
    label++;
  len = strlen(label);
  p = label+len-1;
  while(*p == ' ' || *p == '\t') {
/*    *p = 0x00;*/
    p--;
    len--;
  }
  for(e=enums;e;e=e->next) {
    if (e->label == NULL || len != strlen(e->label))
      continue;
    if (strncmp(label,e->label,min_size_t(len,strlen(e->label))) == 0) {
      snprintf(ret,10,"%d",e->value);
      return(ret);
    }
  }
  /* Not Found */
  return(NULL);
}

void populate_detail_tab(struct tree *node) {

/* Defines for pad */

#  define LAST_FIELD_NOT_BLANK	0
#  define LAST_FIELD_BLANK	1

  static GdkColor red = { 54,32768,0,0 };
  static GdkColor green = { 54,0,32768,0 };

  struct enum_list *e;
  int i;
  char real[512];
  char *dp,*ndp;
  char *new_descr = NULL;
  int pad = LAST_FIELD_BLANK;
  gboolean wr = FALSE;
  gboolean as = FALSE;
  GList *vals = NULL;
  GtkTextBuffer *textBuf;
  GtkTextIter textIter;
  GtkTextTag *textTag;

  switch(node->access) {
    case MIB_ACCESS_READONLY:
      sprintf(real,"Access: ReadOnly");
      break;
    case MIB_ACCESS_READWRITE:
      sprintf(real,"Access: ReadWrite");
      wr = TRUE;
      break;
    case MIB_ACCESS_WRITEONLY:
      sprintf(real,"Access: WriteOnly");
      wr = TRUE;
      break;
    case MIB_ACCESS_NOACCESS:
      sprintf(real,"Access: NoAccess");
      break;
    case MIB_ACCESS_NOTIFY:
      sprintf(real,"Access: Notify");
      break;
    case MIB_ACCESS_CREATE:
      sprintf(real,"Access: Create");
      wr = TRUE;
      break;
    default:
      sprintf(real,"Access: Object has Children");
  }
  gtk_label_set_text(GTK_LABEL(details.access),real);

  textBuf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(details.description));
  gtk_text_buffer_set_text(textBuf,"",0);
  sprintf(real,"Label: %s",node->label);
  gtk_label_set_text(GTK_LABEL(details.label),real);
  if (node->description) {
    ndp = new_descr = alloca(strlen(node->description));
    dp = node->description;
    while(*dp) {
      if (*dp == '\n' || *dp == '\r') {
	*new_descr++ = *dp++;
	while(*dp && (*dp == ' ' || *dp == '\t'))
	  dp++;
      } else {
	pad = LAST_FIELD_NOT_BLANK;
	*new_descr++ = *dp++;
      }
    }
    *new_descr = 0x00;
    gtk_text_buffer_get_end_iter(textBuf,&textIter);
    gtk_text_buffer_insert(textBuf,&textIter,ndp,-1);
  }
  if ((e = enums = node->enums) != NULL) {
    if (pad == LAST_FIELD_NOT_BLANK) {
      gtk_text_buffer_get_end_iter(textBuf,&textIter);
      gtk_text_buffer_insert(textBuf,&textIter,"\n\n",-1);
    }
    pad=LAST_FIELD_NOT_BLANK;
    gtk_text_buffer_get_end_iter(textBuf,&textIter);
    textTag = gtk_text_buffer_create_tag(textBuf, NULL, "foreground-gdk", &red, NULL);
    gtk_text_buffer_insert_with_tags(textBuf,&textIter,"Enumerations:\n",-1, textTag, NULL);
    do {
      sprintf(real,"%d - %s\n",e->value,e->label);
      if (wr == TRUE)
	vals = g_list_append(vals,e->label);
      gtk_text_buffer_get_end_iter(textBuf,&textIter);
      gtk_text_buffer_insert(textBuf,&textIter,real,-1);
    } while ((e = e->next));
  }
  if (wr == TRUE)
    populate_value_list(vals);

  if ((hint = node->hint)) {
    sprintf(real,"Hint: %s",node->hint);
    gtk_label_set_text(GTK_LABEL(details.hint),real);
  } else {
    gtk_label_set_text(GTK_LABEL(details.hint),"");
  }
  if (node->units) {
    sprintf(real,"Units: %s",node->units);
    gtk_label_set_text(GTK_LABEL(details.units),real);
  }
  switch(node->status) {
    case MIB_STATUS_MANDATORY:
      sprintf(real,"Status: Mandatory");
      break;
    case MIB_STATUS_OPTIONAL:
      sprintf(real,"Status: Optional");
      break;
    case MIB_STATUS_OBSOLETE:
      sprintf(real,"Status: Obsolete");
      break;
    case MIB_STATUS_DEPRECATED:
      sprintf(real,"Status: Deprecated");
      break;
    case MIB_STATUS_CURRENT:
      sprintf(real,"Status: Current");
      break;
    default:
      sprintf(real,"Status: ");
  }
  gtk_label_set_text(GTK_LABEL(details.status),real);
  if (node->type <= TYPE_SIMPLE_LAST) {
    sprintf(real,"Type: Object (");
    switch(node->type) {
      case TYPE_OTHER:
        strcat(real,"Other)");
        break;
      case TYPE_OBJID:
        strcat(real,"Object Identifier)");
        break;
      case TYPE_OCTETSTR:
        strcat(real,"Octet String)");
	as = TRUE;
        break;
      case TYPE_INTEGER:
        strcat(real,"Integer)");
	as = TRUE;
        break;
      case TYPE_NETADDR:
        strcat(real,"Network Address)");
        break;
      case TYPE_IPADDR:
        strcat(real,"IP Address)");
        break;
      case TYPE_COUNTER:
        strcat(real,"Counter)");
        break;
      case TYPE_GAUGE:
        strcat(real,"Gauge)");
        break;
      case TYPE_TIMETICKS:
        strcat(real,"Timeticks)");
        break;
      case TYPE_OPAQUE:
        strcat(real,"Opaque)");
        break;
      case TYPE_NULL:
        strcat(real,"NULL)");
        break;
      case TYPE_COUNTER64:
        strcat(real,"64bit Counter)");
        break;
      case TYPE_BITSTRING:
        strcat(real,"Bit String)");
        break;
      case TYPE_NSAPADDRESS:
        strcat(real,"NSAP Address)");
        break;
      case TYPE_UINTEGER:
        strcat(real,"Unsigned Integer)");
	as = TRUE;
        break;
      case TYPE_UNSIGNED32:
        strcat(real,"32bit Unsigned Integer)");
	as = TRUE;
        break;
      case TYPE_INTEGER32:
        strcat(real,"32bit Integer)");
	as = TRUE;
        break;
    }
    variable_writable(wr,as);
  } else {
    switch(node->type) {
      case TYPE_TRAPTYPE:
        sprintf(real,"Type: Trap-Type");
        break;
      case TYPE_NOTIFTYPE:
        sprintf(real,"Type: Notification-Type");
        break;
      case TYPE_OBJGROUP:
        sprintf(real,"Type: Object-Group");
        break;
      case TYPE_AGENTCAP:
        sprintf(real,"Type: Agent-Capabilities");
        break;
      case TYPE_MODID:
        sprintf(real,"Type: Module-Identity");
        break;
      case TYPE_MODCOMP:
        sprintf(real,"Type: Module-Compliance");
        break;
      default:
        sprintf(real,"Type: %d",node->type);
    }
  }
  gtk_label_set_text(GTK_LABEL(details.type),real);
  if (node->number_modules > 0) {

    char modbuf[256];

    if (pad == LAST_FIELD_NOT_BLANK) {
      gtk_text_buffer_get_end_iter(textBuf,&textIter);
      textTag = gtk_text_buffer_create_tag(textBuf, NULL, "foreground-gdk", &green, NULL);
      gtk_text_buffer_insert_with_tags(textBuf,&textIter,"\n\nContained in Module(s):\n",-1, textTag, NULL);
    }
    for(i =  0; i < node->number_modules; i++) {
      sprintf(real,"%s\n", module_name(node->module_list[i], modbuf));
      gtk_text_buffer_get_end_iter(textBuf,&textIter);
      gtk_text_buffer_insert(textBuf,&textIter,real,-1);
    }
  }
}

/* Build the details tab on the main window */

void create_detail_tab(void) {

  GtkWidget *vbox1;
  GtkWidget *table1;
  GtkWidget *frame;
  GtkWidget *vbox2;
  GtkWidget *vbox3;
  GtkWidget *label12;
  GtkWidget *scrolleddetail_window;
  GtkWidget *label2;
  main_widgets_t *mwid;

  mwid = get_main_widgets();

  vbox1 = gtk_vbox_new(FALSE,0);
  gtk_widget_show(vbox1);

  gtk_container_add(GTK_CONTAINER(mwid->notebook),vbox1);

  label2 = gtk_label_new("Details");
  gtk_widget_show (label2);
  gtk_notebook_set_tab_label(GTK_NOTEBOOK(mwid->notebook),
      gtk_notebook_get_nth_page(GTK_NOTEBOOK(mwid->notebook),1),
      label2);

  table1 = gtk_table_new (3, 2, FALSE);
  gtk_widget_show(table1);
  gtk_box_pack_start(GTK_BOX (vbox1), table1,FALSE,FALSE,0);

  frame = gtk_frame_new (NULL);
  gtk_widget_show (frame);
  gtk_table_attach (GTK_TABLE (table1), frame, 0, 1, 0, 1,
      (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
      (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);

  details.label = gtk_label_new ("");
  gtk_widget_show (details.label);
  gtk_container_add (GTK_CONTAINER (frame), details.label);
  gtk_label_set_justify (GTK_LABEL (details.label), GTK_JUSTIFY_LEFT);

  frame = gtk_frame_new (NULL);
  gtk_widget_show (frame);
  gtk_table_attach (GTK_TABLE (table1), frame, 0, 1, 1, 2,
      (GtkAttachOptions) (GTK_FILL),
      (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);

  details.type = gtk_label_new ("");
  gtk_widget_show (details.type);
  gtk_container_add (GTK_CONTAINER (frame), details.type);
  gtk_label_set_justify (GTK_LABEL (details.type), GTK_JUSTIFY_LEFT);

  frame = gtk_frame_new (NULL);
  gtk_widget_show (frame);
  gtk_table_attach (GTK_TABLE (table1), frame, 1, 2, 0, 1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);

  details.access = gtk_label_new ("");
  gtk_widget_show (details.access);
  gtk_container_add (GTK_CONTAINER (frame), details.access);
  gtk_label_set_justify (GTK_LABEL (details.access), GTK_JUSTIFY_LEFT);

  frame = gtk_frame_new (NULL);
  gtk_widget_show (frame);
  gtk_table_attach (GTK_TABLE (table1), frame, 0, 1, 2, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);

  details.hint = gtk_label_new ("");
  gtk_widget_show (details.hint);
  gtk_container_add (GTK_CONTAINER (frame), details.hint);
  gtk_label_set_justify (GTK_LABEL (details.hint), GTK_JUSTIFY_LEFT);

  frame = gtk_frame_new (NULL);
  gtk_widget_show (frame);
  gtk_table_attach (GTK_TABLE (table1), frame, 1, 2, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);

  details.status = gtk_label_new ("");
  gtk_widget_show (details.status);
  gtk_container_add (GTK_CONTAINER (frame), details.status);
  gtk_label_set_justify (GTK_LABEL (details.status), GTK_JUSTIFY_LEFT);

  frame = gtk_frame_new (NULL);
  gtk_widget_show (frame);
  gtk_table_attach (GTK_TABLE (table1), frame, 1, 2, 2, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);

  details.units = gtk_label_new ("");
  gtk_widget_show (details.units);
  gtk_container_add (GTK_CONTAINER (frame), details.units);
  gtk_label_set_justify (GTK_LABEL (details.units), GTK_JUSTIFY_LEFT);

  frame = gtk_frame_new (NULL);
  gtk_widget_show (frame);
  gtk_box_pack_start (GTK_BOX (vbox1), frame, TRUE, TRUE, 0);

  vbox2 = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vbox2);
  gtk_container_add (GTK_CONTAINER (frame), vbox2);

  frame = gtk_frame_new(NULL);
  gtk_widget_show(frame);
  gtk_box_pack_start(GTK_BOX(vbox2),frame,TRUE,TRUE,0);

  vbox3 = gtk_vbox_new(FALSE, 0);
  gtk_widget_show(vbox3);
  gtk_container_add(GTK_CONTAINER(frame),vbox3);

  label12 = gtk_label_new("Description");
  gtk_widget_show(label12);
  gtk_box_pack_start(GTK_BOX(vbox3),label12,FALSE,FALSE,0);

  scrolleddetail_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolleddetail_window),
      GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
  gtk_widget_show(scrolleddetail_window);
  gtk_box_pack_start(GTK_BOX(vbox3),scrolleddetail_window,TRUE, TRUE, 0);

  details.description = gtk_text_view_new();
  gtk_widget_show(details.description);
  gtk_container_add(GTK_CONTAINER(scrolleddetail_window),details.description);
}

