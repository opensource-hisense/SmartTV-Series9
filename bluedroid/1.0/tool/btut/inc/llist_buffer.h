#ifndef __LLIST_BUFFER_H__
#define __LLIST_BUFFER_H__

#ifndef TRUE
#define TRUE                                  1
#endif
#ifndef FALSE
#define FALSE                                 0
#endif
#ifndef NULL
#define NULL                                  0
#endif
#ifndef SUCCESS
#define SUCCESS                           0
#endif

#define LLIST_WRITE_MALLOC_ERR1               150
#define LLIST_WRITE_MALLOC_ERR2               151
#define LLIST_READ_INVALID_ARGUMENT_ERR               152
#define LLIST_DELETE_INVALID_ARGUMENT_ERR             153
#define LLIST_READ_INVALID_HANDLE_ERR                     154
#define LLIST_WRITE_INVALID_HANDLE_ERR                    155

typedef struct llist
{
    char *usrdata;
    struct llist *next;
}l_list;

l_list* llist_buffer_create();
unsigned char llist_buffer_write(l_list*, char*, unsigned int);
unsigned char llist_buffer_read_first_node(l_list*, char*, char**, unsigned int, unsigned int);
l_list* llist_buffer_delete_first_node_and_update_list(l_list*);
void llist_buffer_destroy(l_list*);
#endif
