/* Hold needed data from the search tab */

typedef struct {
  GtkWidget *list;
  GtkWidget *criteria;
  GtkWidget *entry;
} search_t;

/* Function Prototypes */

void create_search_tab(void);
