typedef struct {
  const char *oid;
  const char *readcomm;
  const char *writecomm;
  const char *host;
  const char *inst;
  const char *value;
  int  save_sess;
  int  snmp_ver;
  int  snmp_timeout;
  int  snmp_retries;
  int  snmp_port;
  int  tree_expander;
  int  tree_line;
  GtkWrapMode out_wrap_mode;
  int  save_window_size;
  int  window_width;
  int  window_height;
  int  window_maximized;
} config_t;

config_t *get_config(void);
void save_config(void);
void load_config(void);
void create_config_tab(int tab_pos);
