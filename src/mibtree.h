void set_mibtree_style(int exp_style,int line_style);
void select_oid(oid poid[],size_t size);
void populate_mib_tree(struct tree *child,GtkTreeIter *tree);
void create_mibtree_tab(void);
void retree(struct tree *mibtree);
