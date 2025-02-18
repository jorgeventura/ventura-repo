/*
 * configuration.c
 *
 * Load and save configuration
 *
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <gtk/gtk.h>
#include "configuration.h"
#include "interface.h"
#include "callbacks.h"
#include "mibtree.h"
#include "os.h"

#define CONFIG_FILE 	PATH_SEPARATOR ".mbrowse"
#define CREAT_ERR	"Error creating "/**/CONFIG_FILE
#define BLANK		""

static int wrap_none = GTK_WRAP_NONE;
static int wrap_char = GTK_WRAP_CHAR;
static int wrap_word = GTK_WRAP_WORD;
static int wrap_word_char = GTK_WRAP_WORD_CHAR;

#define MAX_WRAPPING	(GTK_WRAP_NONE | \
			GTK_WRAP_CHAR | \
			GTK_WRAP_WORD | \
			GTK_WRAP_WORD_CHAR)+1

#define MAX_SNMP_VER	(SNMP_VERSION_1 | SNMP_VERSION_2c)+1

extern GdkPixbuf *child_pixbuf;
extern GdkPixbuf *end_pixbuf;

static GtkWidget *wrappings[MAX_WRAPPING];
static GtkWidget *snmp_ver[MAX_SNMP_VER];
static GtkWidget *test_tree;
static GtkWidget *snmp_port_entry;
static GtkWidget *save_sess_button;
static GtkWidget *save_window_size_button;
static GtkWidget *snmp_to_spin;
static GtkWidget *snmp_retry_spin;
static GtkWidget *line_show_butt;

union types {
  char *string;
  int integer;
};

typedef struct {
  char *option;
  int type;
  union types default_value;
  void *variable;
} config_opt_t;
 
static config_t config;

#define STRING 	0
#define INTEGER	1

config_opt_t config_opts[] = {
  { "oid",          STRING, {""},       &config.oid },
  { "readcomm",     STRING, {"public"}, &config.readcomm },
  { "writecomm",    STRING, {"private"},&config.writecomm },
  { "host",         STRING, {""},       &config.host },
  { "inst",         STRING, {""},       &config.inst },
  { "value",        STRING, {""},       &config.value },
  { "save_sess",    INTEGER,{integer:TRUE},&config.save_sess },
  { "snmp_ver",     INTEGER,{integer:SNMP_VERSION_1},&config.snmp_ver },
  { "snmp_timeout", INTEGER,{integer:2},&config.snmp_timeout },
  { "snmp_reties",  INTEGER,{integer:1},&config.snmp_retries },
  { "snmp_port",    INTEGER,{integer:161},&config.snmp_port },
  { "tree_line",    INTEGER,{integer:1},&config.tree_line },
  { "out_wrap_mode",INTEGER,{integer:GTK_WRAP_NONE},&config.out_wrap_mode },
  { "save_window_size", INTEGER, {integer:1}, &config.save_window_size },
  { "window_width", INTEGER, {integer:-1}, &config.window_width },
  { "window_height", INTEGER, {integer:-1}, &config.window_height },
  { "window_maximized", INTEGER, {integer:0}, &config.window_maximized },
  { NULL,           INTEGER, {integer:0},NULL },
};

config_t *get_config() {

  return(&config);
}

static void set_defaults(void) {

  config_opt_t *c;

  for(c=config_opts;c->option;c++) {
    switch(c->type) {
      case STRING:
        *((char **)c->variable) = c->default_value.string;
        break;
      case INTEGER:
        *((int *)c->variable) = c->default_value.integer;
        break;
      default:
        fprintf(stderr,"I should have never gotten here. Bad config type.\n");
        break;
    }
  }
};

static void apply_config(void) {

  char port[10];
  /* Set entries in the options tab */


  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(line_show_butt),config.tree_line);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(wrappings[config.out_wrap_mode]),TRUE);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(save_sess_button),config.save_sess);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(save_window_size_button),config.save_window_size);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(snmp_ver[config.snmp_ver]),TRUE);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(snmp_to_spin),(gfloat) config.snmp_timeout);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(snmp_retry_spin),(gfloat)config.snmp_retries);
  snprintf(port,6,"%u",config.snmp_port);
  gtk_entry_set_text(GTK_ENTRY(snmp_port_entry),port);

  /* Set entries in the main window */

  set_readcomm(config.readcomm);
  set_writecomm(config.writecomm);
  set_oid(config.oid);
  set_hostname(config.host);
  set_instance(config.inst);
  set_value(config.value);
  if (config.save_window_size) {
    if ((config.window_width != -1) && (config.window_height != -1)) {
      set_window_size(config.window_width, config.window_height);
    }
    if (config.window_maximized) {
      set_window_maximized();
    }
  }
}

void load_config(void) {

# define KEY 0
# define VAL 1

  char line[1023];
  char *tok;
  FILE *fp;
  config_opt_t *c;
  int state;
  int lineno = 0;

#ifdef WIN32
  strncpy(line,".\\",1023); /* FIXME: hack */
#else
  strncpy(line,getenv("HOME"),1023);
#endif
  strncat(line,CONFIG_FILE,(1023-strlen(line)));
  set_defaults();
  if((fp = fopen(line,"r")) == NULL) {
    if((fp = fopen(line,"w")) == NULL) {
      perror(CREAT_ERR);
      apply_config();
      return;
    }
  } else {
    while(fgets(line,1023,fp)) {
      lineno++;
      state = KEY; 
      tok = strtok(line,"\n\r\t ");
      if(tok == NULL || *tok == 0x00 || *tok == '#')
        continue;
      for(c=config_opts;c->option;c++) {
        if(strncmp(tok,c->option,strlen(c->option)) == 0) {
          state=VAL;
          break;
        }
      }
      if(state == KEY) {
        fprintf(stderr,"Bad token at line %d in %s. Ignoring\n",lineno,CONFIG_FILE);
        continue;
      }
      tok = strtok(NULL,"\n\r\t ");
      if(tok == NULL || *tok == 0x00 || *tok == '#') {
        switch(c->type) {
          case STRING:
            *((char **)c->variable) = BLANK;
            break;
          case INTEGER:
           *((int *)c->variable) = 0;
            break;
          default:
            fprintf(stderr,"I should have never gotten here. Bad config type.\n");
            break;
        }
        continue;
      }
      switch(c->type) {
        case STRING:
          *((char **)c->variable) = strdup(tok);
          break;
        case INTEGER:
          *((int *)c->variable) = atoi(tok);
          break;
        default:
          fprintf(stderr,"I should have never gotten here. Bad config type.\n");
          break;
      }
    }
  }
  fclose(fp);
  apply_config();
}

void save_config(void) {

  FILE *fp;
  config_opt_t *c;
  char file[1024];

  /* Get some value that aren't set dynamically */

  config.readcomm = get_readcomm();
  config.writecomm = get_writecomm();
  config.host = get_hostname();
  config.inst = get_instance();
  config.value = get_value();

  if (config.save_window_size) {
    get_window_size(&config.window_width, &config.window_height);
  } else {
    /* Discard any previous values. */
    config.window_width = -1;
    config.window_height = -1;
    config.window_maximized = 0;
  }

  if(config.save_sess == FALSE)
    return;
#ifdef WIN32
  strncpy(file,".\\",1023); /* FIXME: hack */
#else
  strncpy(file,getenv("HOME"),1023);
#endif
  strncat(file,CONFIG_FILE,(1023-strlen(file)));
  if((fp = fopen(file,"w")) == NULL) {
    perror(CREAT_ERR);
    return;
  }
  for(c=config_opts;c->option;c++) {
    switch(c->type) {
      case STRING:
        fprintf(fp,"%s %s\n",c->option,*((char **)c->variable));
        break;
      case INTEGER:
        fprintf(fp,"%s %i\n",c->option,*((int *)c->variable));
        break;
      default:
        fprintf(stderr,"I should have never gotten here. Bad config type.\n");
        break;
    }
  }
  fclose(fp);

  /* Don't want others seeing our community strings */

  chmod(CONFIG_FILE,S_IWUSR | S_IRUSR);
}

static void show_line_toggled(GtkWidget *widget,gpointer data ) {

  config.tree_line = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
  gtk_tree_view_set_enable_tree_lines(GTK_TREE_VIEW(test_tree), config.tree_line);
  set_mibtree_style(config.tree_expander,config.tree_line);
}

static void set_snmp_version(GtkWidget *widget,gpointer data) {

  config.snmp_ver = (long) data;
}

static void set_timeout(GtkWidget *widget,GtkWidget *data) {

  config.snmp_timeout = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(data));
}

static void set_retries(GtkWidget *widget,GtkWidget *data) {

  config.snmp_retries = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(data));
}
static void set_port(GtkWidget *widget,GtkWidget *data) {

  config.snmp_port = atoi(gtk_entry_get_text(GTK_ENTRY(data)));
}

static void set_out_wrap_mode(GtkWidget *widget,GtkWidget *data) {
  if (widget)
    if (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)))
      return;
  config.out_wrap_mode = *(int *)data;
  set_out_box_wrap_mode(config.out_wrap_mode);
}

static void save_sess_toggled(GtkButton *button,gpointer user_data) {

  FILE *fp;
  char file[1024];

  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) == TRUE)
    config.save_sess = TRUE;
  else {
    config.save_sess = FALSE;
#ifdef WIN32
    strncpy(file,".\\",1023); /* FIXME: hack */
#else
    strncpy(file,getenv("HOME"),1023);
#endif
    strncat(file,CONFIG_FILE,(1023-strlen(file)));
    if((fp = fopen(file,"w")) == NULL) {
      perror(CREAT_ERR);
      return;
    }
    fprintf(fp,"save_sess 0\n");
    fclose(fp);
  }
}

static void save_window_size_toggled(GtkButton *button, gpointer user_data) {
  config.save_window_size = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button));
}

void create_config_tab(int tab_pos) {

  GtkWidget *option_vbox;
  GtkWidget *general_frame;
  GtkWidget *wrapping_frame;
  GtkWidget *snmp_config_frame;
  GtkWidget *general_frame_vbox;
  GtkWidget *snmp_config_hbox;
  GtkWidget *snmp_ver_vbox;
  GtkWidget *snmp_ver_label;
  GtkWidget *hbox2;
  GSList *snmp_ver_group = NULL;
  GtkWidget *hbox3;
  GtkWidget *snmp_to_vbox;
  GtkWidget *timeout_label;
  GtkWidget *snmp_to_hbox;
  GtkObject *snmp_to_spin_adj;
  GtkWidget *secs_label;
  GtkWidget *snmp_retry_vbox;
  GtkWidget *snmp_retry_label;
  GtkObject *snmp_retry_spin_adj;
  GtkWidget *snmp_port_vbox;
  GtkWidget *snmp_port_label;
  GtkWidget *tree_style_frame;
  GtkWidget *tree_style_hbox;
  GtkWidget *wrapping_vbox;
  GSList *wrapping_group = NULL;
  GtkWidget *wrapping_none_butt;
  GtkWidget *wrapping_char_butt;
  GtkWidget *wrapping_word_butt;
  GtkWidget *wrapping_word_char_butt;
  GtkWidget *config_tab;
  GtkWidget *hbox1;
  GtkWidget *notebook;
  GtkTreeViewColumn *tree_col;
  GtkCellRenderer *tree_renderer1;
  GtkCellRenderer *tree_renderer2;
  GtkTreeStore *tree_store;
  GtkTreeIter tree_parent;
  GtkTreeIter tree_child;
  GtkTreeIter tree_end;

  notebook = get_main_widgets()->notebook;
  option_vbox = gtk_vbox_new(FALSE,0);
  gtk_widget_show(option_vbox);
  gtk_container_add(GTK_CONTAINER(notebook),option_vbox);

  hbox1 = gtk_hbox_new(FALSE,0);
  gtk_widget_show(hbox1);
  gtk_box_pack_start(GTK_BOX(option_vbox),hbox1,FALSE,TRUE,0);

  hbox3 = gtk_hbox_new(FALSE,0);
  gtk_widget_show(hbox3);
  gtk_box_pack_start(GTK_BOX(option_vbox),hbox3,FALSE,TRUE,0);

  wrapping_frame = gtk_frame_new("Output wrapping");
  gtk_widget_show(wrapping_frame);
  gtk_box_pack_start(GTK_BOX(hbox3),wrapping_frame,FALSE,TRUE,5);
  gtk_container_set_border_width(GTK_CONTAINER(wrapping_frame),5);

  general_frame = gtk_frame_new("General");
  gtk_widget_show(general_frame);
  gtk_box_pack_start(GTK_BOX(hbox1),general_frame,FALSE,TRUE,0);
  gtk_container_set_border_width(GTK_CONTAINER(general_frame),5);

  general_frame_vbox = gtk_vbox_new(FALSE, 0);
  gtk_widget_show(general_frame_vbox);
  gtk_container_add(GTK_CONTAINER(general_frame), general_frame_vbox);

  save_sess_button = gtk_check_button_new_with_label("Save session on exit");
  gtk_widget_show(save_sess_button);
  gtk_box_pack_start(GTK_BOX(general_frame_vbox), save_sess_button, FALSE, FALSE, 0);
  gtk_signal_connect_after(GTK_OBJECT(save_sess_button),"toggled",
                           GTK_SIGNAL_FUNC(save_sess_toggled),NULL);

  save_window_size_button = gtk_check_button_new_with_label("Save window size");
  gtk_widget_show(save_window_size_button);
  gtk_box_pack_start(GTK_BOX(general_frame_vbox), save_window_size_button, FALSE, FALSE, 0);
  gtk_signal_connect_after(GTK_OBJECT(save_window_size_button),"toggled",
                           GTK_SIGNAL_FUNC(save_window_size_toggled),NULL);


  snmp_config_frame = gtk_frame_new("SNMP Configuration");
  gtk_widget_show(snmp_config_frame);
  gtk_box_pack_start(GTK_BOX(hbox1),snmp_config_frame,FALSE,TRUE,0);
  gtk_container_set_border_width(GTK_CONTAINER(snmp_config_frame),5);

  snmp_config_hbox = gtk_hbox_new(FALSE,0);
  gtk_widget_show(snmp_config_hbox);
  gtk_container_add(GTK_CONTAINER(snmp_config_frame),snmp_config_hbox);

  snmp_ver_vbox = gtk_vbox_new(FALSE,0);
  gtk_widget_show(snmp_ver_vbox);
  gtk_box_pack_start(GTK_BOX(snmp_config_hbox),snmp_ver_vbox,FALSE,TRUE,5);

  snmp_ver_label = gtk_label_new("Version");
  gtk_widget_show(snmp_ver_label);
  gtk_box_pack_start(GTK_BOX(snmp_ver_vbox),snmp_ver_label,FALSE,FALSE,0);

  hbox2 = gtk_hbox_new(FALSE,0);
  gtk_widget_show(hbox2);
  gtk_box_pack_start(GTK_BOX(snmp_ver_vbox),hbox2,FALSE,TRUE,0);

  snmp_ver[SNMP_VERSION_1] = gtk_radio_button_new_with_label(snmp_ver_group,"v1");
  snmp_ver_group = gtk_radio_button_group(GTK_RADIO_BUTTON(snmp_ver[SNMP_VERSION_1]));
  gtk_widget_show(snmp_ver[SNMP_VERSION_1]);
  gtk_box_pack_start(GTK_BOX(hbox2),snmp_ver[SNMP_VERSION_1],FALSE,FALSE,0);
  gtk_signal_connect(GTK_OBJECT(snmp_ver[SNMP_VERSION_1]),"clicked",GTK_SIGNAL_FUNC(set_snmp_version),(gpointer)SNMP_VERSION_1);

  snmp_ver[SNMP_VERSION_2c] = gtk_radio_button_new_with_label(snmp_ver_group,"v2c");
  snmp_ver_group = gtk_radio_button_group(GTK_RADIO_BUTTON(snmp_ver[SNMP_VERSION_2c]));
  gtk_widget_show(snmp_ver[SNMP_VERSION_2c]);
  gtk_box_pack_start(GTK_BOX(hbox2),snmp_ver[SNMP_VERSION_2c],FALSE,FALSE,0);
  gtk_signal_connect(GTK_OBJECT(snmp_ver[SNMP_VERSION_2c]),"clicked",GTK_SIGNAL_FUNC(set_snmp_version),(gpointer)SNMP_VERSION_2c);

  snmp_to_vbox = gtk_vbox_new(FALSE,0);
  gtk_widget_show(snmp_to_vbox);
  gtk_box_pack_start(GTK_BOX(snmp_config_hbox),snmp_to_vbox,FALSE,TRUE,5);

  timeout_label = gtk_label_new("Timeout");
  gtk_widget_show(timeout_label);
  gtk_box_pack_start(GTK_BOX(snmp_to_vbox),timeout_label,FALSE,FALSE,0);

  snmp_to_hbox = gtk_hbox_new(FALSE,0);
  gtk_widget_show(snmp_to_hbox);
  gtk_box_pack_start(GTK_BOX(snmp_to_vbox),snmp_to_hbox,FALSE,FALSE,0);

  snmp_to_spin_adj = gtk_adjustment_new(1,0,100,1,10,0);
  snmp_to_spin = gtk_spin_button_new(GTK_ADJUSTMENT(snmp_to_spin_adj),1,0);
  gtk_widget_show(snmp_to_spin);
  gtk_box_pack_start(GTK_BOX(snmp_to_hbox),snmp_to_spin,TRUE,TRUE,0);
  gtk_signal_connect (GTK_OBJECT(snmp_to_spin_adj),"value_changed",
                      GTK_SIGNAL_FUNC(set_timeout),
                      (gpointer) snmp_to_spin);

  secs_label = gtk_label_new("secs");
  gtk_widget_show(secs_label);
  gtk_box_pack_start(GTK_BOX(snmp_to_hbox),secs_label,FALSE,FALSE,0);

  snmp_retry_vbox = gtk_vbox_new(FALSE,0);
  gtk_widget_show(snmp_retry_vbox);
  gtk_box_pack_start(GTK_BOX(snmp_config_hbox),snmp_retry_vbox,FALSE,TRUE,5);

  snmp_retry_label = gtk_label_new("Retries");
  gtk_widget_show(snmp_retry_label);
  gtk_box_pack_start(GTK_BOX(snmp_retry_vbox),snmp_retry_label,FALSE,FALSE,0);

  snmp_retry_spin_adj = gtk_adjustment_new(1,0,100,1,10,0);
  snmp_retry_spin = gtk_spin_button_new(GTK_ADJUSTMENT(snmp_retry_spin_adj),1,0);
  gtk_widget_show(snmp_retry_spin);
  gtk_box_pack_start(GTK_BOX(snmp_retry_vbox),snmp_retry_spin,FALSE,FALSE,0);
  gtk_signal_connect (GTK_OBJECT(snmp_retry_spin_adj),"value_changed",
                      GTK_SIGNAL_FUNC(set_retries),
                      (gpointer) snmp_retry_spin);

  snmp_port_vbox = gtk_vbox_new(FALSE,0);
  gtk_widget_show(snmp_port_vbox);
  gtk_box_pack_start(GTK_BOX(snmp_config_hbox),snmp_port_vbox,FALSE,TRUE,5);

  snmp_port_label = gtk_label_new("Port");
  gtk_widget_show(snmp_port_label);
  gtk_box_pack_start(GTK_BOX(snmp_port_vbox),snmp_port_label,FALSE,FALSE,0);

  snmp_port_entry = gtk_entry_new_with_max_length(5);
  gtk_widget_show(snmp_port_entry);
  gtk_box_pack_start(GTK_BOX(snmp_port_vbox),snmp_port_entry,FALSE,FALSE,0);
  gtk_widget_set_usize(snmp_port_entry,80,-2);
  gtk_signal_connect_after(GTK_OBJECT(snmp_port_entry),"changed",
      GTK_SIGNAL_FUNC(set_port),snmp_port_entry);


  tree_style_frame = gtk_frame_new("Tree Style");
  gtk_widget_show(tree_style_frame);
  gtk_box_pack_start(GTK_BOX(hbox3),tree_style_frame,TRUE,TRUE,0);
  gtk_container_set_border_width(GTK_CONTAINER(tree_style_frame),5);

  tree_style_hbox = gtk_hbox_new(FALSE,0);
  gtk_widget_show(tree_style_hbox);
  gtk_container_add(GTK_CONTAINER(tree_style_frame),tree_style_hbox);

  wrapping_vbox = gtk_vbox_new(FALSE,0);
  gtk_widget_show(wrapping_vbox);
  gtk_container_add(GTK_CONTAINER(wrapping_frame),wrapping_vbox);

  wrapping_none_butt = gtk_radio_button_new_with_label(wrapping_group,"None");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(wrapping_none_butt),TRUE);
  wrappings[GTK_WRAP_NONE] = wrapping_none_butt;
  wrapping_group = gtk_radio_button_group(GTK_RADIO_BUTTON(wrapping_none_butt));
  gtk_widget_show(wrapping_none_butt);
  gtk_box_pack_start(GTK_BOX(wrapping_vbox),wrapping_none_butt,FALSE,FALSE,0);
  gtk_signal_connect(GTK_OBJECT(wrapping_none_butt),"clicked",GTK_SIGNAL_FUNC(set_out_wrap_mode),&wrap_none);

  wrapping_char_butt = gtk_radio_button_new_with_label(wrapping_group,"Characters");
  wrappings[GTK_WRAP_CHAR] = wrapping_char_butt;
  wrapping_group = gtk_radio_button_group(GTK_RADIO_BUTTON(wrapping_char_butt));
  gtk_widget_show(wrapping_char_butt);
  gtk_box_pack_start(GTK_BOX(wrapping_vbox),wrapping_char_butt,FALSE,FALSE,0);
  gtk_signal_connect(GTK_OBJECT(wrapping_char_butt),"clicked",GTK_SIGNAL_FUNC(set_out_wrap_mode),&wrap_char);

  wrapping_word_butt = gtk_radio_button_new_with_label(wrapping_group,"Words");
  wrappings[GTK_WRAP_WORD] = wrapping_word_butt;
  wrapping_group = gtk_radio_button_group(GTK_RADIO_BUTTON(wrapping_word_butt));
  gtk_widget_show(wrapping_word_butt);
  gtk_box_pack_start(GTK_BOX(wrapping_vbox),wrapping_word_butt,FALSE,FALSE,0);
  gtk_signal_connect(GTK_OBJECT(wrapping_word_butt),"clicked",GTK_SIGNAL_FUNC(set_out_wrap_mode),&wrap_word);

  wrapping_word_char_butt = gtk_radio_button_new_with_label(wrapping_group,"Words then Characters");
  wrappings[GTK_WRAP_WORD_CHAR] = wrapping_word_char_butt;
  wrapping_group = gtk_radio_button_group(GTK_RADIO_BUTTON(wrapping_word_char_butt));
  gtk_widget_show(wrapping_word_char_butt);
  gtk_box_pack_start(GTK_BOX(wrapping_vbox),wrapping_word_char_butt,FALSE,FALSE,0);
  gtk_signal_connect(GTK_OBJECT(wrapping_word_char_butt),"clicked",GTK_SIGNAL_FUNC(set_out_wrap_mode),&wrap_word_char);

  line_show_butt = gtk_check_button_new_with_label("Show lines");
  gtk_widget_show(line_show_butt);
  gtk_box_pack_start(GTK_BOX(tree_style_hbox),line_show_butt,FALSE,FALSE,0);
  gtk_signal_connect(GTK_OBJECT(line_show_butt),"toggled",GTK_SIGNAL_FUNC(show_line_toggled),NULL);

  test_tree = gtk_tree_view_new();
  gtk_widget_show(test_tree);
  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(test_tree), FALSE);
  gtk_box_pack_start(GTK_BOX(tree_style_hbox),test_tree,TRUE,TRUE,0);

  tree_col = gtk_tree_view_column_new();
  gtk_tree_view_append_column(GTK_TREE_VIEW(test_tree), tree_col);
  tree_renderer1 = gtk_cell_renderer_pixbuf_new();
  gtk_tree_view_column_pack_start(tree_col, tree_renderer1, FALSE);
  gtk_tree_view_column_add_attribute(tree_col, tree_renderer1, "pixbuf", 0);

  tree_renderer2 = gtk_cell_renderer_text_new();
  gtk_tree_view_column_pack_start(tree_col, tree_renderer2, TRUE);
  gtk_tree_view_column_add_attribute(tree_col, tree_renderer2, "text", 1);

  tree_store = gtk_tree_store_new(2, GDK_TYPE_PIXBUF, G_TYPE_STRING);

  gtk_tree_store_append(tree_store, &tree_parent, NULL);
  gtk_tree_store_set(tree_store, &tree_parent, 0, child_pixbuf, 1, "Parent", -1);
  gtk_tree_store_append(tree_store, &tree_child, &tree_parent);
  gtk_tree_store_set(tree_store, &tree_child, 0, child_pixbuf, 1, "Child", -1);
  gtk_tree_store_append(tree_store, &tree_end, &tree_child);
  gtk_tree_store_set(tree_store, &tree_end, 0, end_pixbuf, 1, "Leaf", -1);

  gtk_tree_view_set_model(GTK_TREE_VIEW(test_tree), GTK_TREE_MODEL(tree_store));
  gtk_tree_view_expand_all(GTK_TREE_VIEW(test_tree));

  config_tab = gtk_label_new("Options");
  gtk_widget_show(config_tab);
  gtk_notebook_set_tab_label(GTK_NOTEBOOK(notebook),gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook),tab_pos),config_tab);
}
