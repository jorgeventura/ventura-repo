/* Hold needed data from description tab */

typedef struct {
  GtkWidget *label;
  GtkWidget *type;
  GtkWidget *access;
  GtkWidget *hint;
  GtkWidget *status;
  GtkWidget *units;
  GtkWidget *description;
} details_t;

/* Function Prototypes */

void create_detail_tab(void);
void populate_detail_tab(struct tree *node);
const char *find_val_from_enum(const char *label);
