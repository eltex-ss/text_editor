#pragma once

#define LINE_LENGHT 79

struct List {
  char line[LINE_LENGHT];
  struct List *next;
};

/*  Create the linked list */
struct List* CreateList(void);

/*  Delete the linked list */
void RemoveList(struct List *list);

/*  Add leaf by line */
void AddLeaf(const char *line, struct List *head);

/*  Remove leaf by name */
void RemoveLeafByNumber(int num, struct List *head);

/*  Clear the list */
void ClearList(struct List *head);
