#ifndef __QUEUE_H__
#define __QUEUE_H__

#include <string.h>
#include <stdlib.h>

//************************************ #define constants **********************************
#define     QUEUE_PUT_SYS_MEM_ERROR                          -203
#define     QUEUE_PUT_INVALID_INPUT_BUFFER            -204
#define     QUEUE_FULL_ERROR                                                -205
#define     QUEUE_EMPTY_ERROR                                           -206
#define     QUEUE_GET_SIZE_MISS_MATCH                        -207
#define     QUEUE_GET_UNIT_SIZE_FAILED                         -208

#ifndef      TRUE
#define     TRUE                                       1
#endif
#ifndef      FALSE
#define     FALSE                                      0
#endif

//************************************ typedef definitions ********************************
typedef char BOOLEAN; //True or False.
typedef struct cirqueue
{
    int head;
    int tail;
    char **array;
    unsigned int q_size;
}queue;

//*********************************** Functions declarations ******************************
/*********************************************************************************
* Function Name   : queue_create()                                               *
* Description     : creates a queue and initializes.                             *
* Return Parameter: returns created Queue address on success, if not NULL.       *
* Argument        : max_q_size  - number of elements in a queue.                 *
*********************************************************************************/
queue* queue_create(unsigned int max_q_size);

/*********************************************************************************
* Function Name   : queue_destroy()                                              *
* Description     : destroys a queue i.e., frees all allocated memory.           *
* Return Parameter: returns TRUE if the passed address is found and freed.       *
*                   returns FALSE if the passed address is NULL.                 *
* Argument        : q  - adress of the queue to be destroyed.                    *
*********************************************************************************/
BOOLEAN queue_destroy(queue *);

/*********************************************************************************
* Function Name   : queue_put()                                                  *
* Description     : pushes value into to the queue.                              *
* Return Parameter: returns 0 for success and -ve error code in case of error.   *
* Argument        : q    - address of the queue.                                 *
*                   buf  - memory pointer of a data to be pushed into queue.     *
*                   size - size to be pushed into queue.                         *
*********************************************************************************/
int queue_put(queue *q, char *buf, unsigned int buf_size);

/*********************************************************************************
* Function Name   : queue_get()                                                  *
* Description     : copy the buffer from the queue and delete it from the queue. *
* Return Parameter: returns 0 for success and -ve error code in case of error.   *
* Argument        : q             - address to a queue.                          *
*                   copy_buf_addr - memory pointer to which data to be copied.   *
*                   copy_buf_size - size of buffer.                              *
*********************************************************************************/
int queue_get(queue *q, char *copy_buf_addr, unsigned int copy_buf_size);

/*********************************************************************************
* Function Name   : queue_get_unit_size()                                        *
* Description     : copy the buffer from the queue and delete it from the queue. *
* Return Parameter: returns error code in case of error.                         *
* Argument        : q       - address to a queue.                                *
*                   size    - size of the data of first element of Queue.        *
*********************************************************************************/
int queue_get_unit_size(queue *q, unsigned int *size);

/*********************************************************************************
* Function Name   : queue_check_queuefull()                                      *
* Description     : checks whether the recieved queue is full.                   *
* Return Parameter: returns TRUE if queue is full else FALSE.                    *
* Argument        : q   - address to a queue.                                    *
*********************************************************************************/
BOOLEAN queue_check_queue_full(queue *q);

/**********************************************************************************
* Function Name   : queue_check_queue_empty()                                     *
* Description     : checks whether the recieved queue is empty.                   *
* Return Parameter: returns TRUE if queue is empty else FALSE.                    *
* Argument        : q - address to a queue                                        *
**********************************************************************************/
BOOLEAN queue_check_queue_empty(queue *q);

 #endif //__QUEUE_H__