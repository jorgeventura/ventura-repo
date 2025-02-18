typedef struct {
  GtkWidget *set_button;
  GtkWidget *statusbar;
  GtkWidget *oid_entry;
  GtkWidget *host;
  GtkWidget *error_label;
  GtkWidget *notebook;
  GtkWidget *bookmark_menu;
  GtkAccelGroup *accels;
} main_widgets_t;

struct menu_items {
  char *item;
  GtkSignalFunc func;
  gpointer data;
  GtkWidget *mi;
};

void set_hostname(const char *host);
void set_oid(const char *oid);
void set_readcomm(const char *comm);
void set_writecomm(const char *comm);
void set_instance(const char *inst);
void set_value(const char *val);
const char *get_hostname(void);
const char *get_readcomm(void);
const char *get_writecomm(void);
const char *get_instance(void);
const char *get_value(void);
char get_as(void);
void set_as(const char *title);
void get_window_size(int *width, int *height);
void set_window_size(int width, int height);
void set_window_maximized();
void populate_value_list(GList *vals);
GtkWidget* create_main_window(void);
GtkWidget *create_file_selection(void);
main_widgets_t *get_main_widgets(void);
void variable_writable(gboolean p,gboolean a);
void set_out_box_wrap_mode(GtkWrapMode mode);

void handle_events(void);
