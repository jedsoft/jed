#ifndef JED_DFA_SYNTAX_H_
#define JED_DFA_SYNTAX_H_
typedef struct Highlight Highlight;
extern int jed_init_dfa_syntax (void);
extern void jed_dfa_free_highlight_table (Highlight *);
#endif
