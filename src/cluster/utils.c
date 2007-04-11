#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <xbt/sysdep.h>
#include <xbt/fifo.h>
#include <msg/msg.h>

#include "utils.h"


void xbt_fifo_sort(xbt_fifo_t fifo) {
    auto int stringCmp(const void * t1, const void * t2);
    int stringCmp(const void * t1, const void * t2) { 
        return strcmp((*((m_task_t *) t1))->name, (*((m_task_t *) t2))->name);
    }

    int i = 0;
    m_task_t * messages =  (m_task_t *)xbt_fifo_to_array(fifo);
    
    // fprintf(stderr, "qsort %d\n", xbt_fifo_size(fifo)); 
    qsort(messages, xbt_fifo_size(fifo), sizeof(m_task_t), stringCmp);
    for (i=0; i<xbt_fifo_size(fifo); ++i) {
        // fprintf(stderr, "%s\t", messages[i]->name);
        /* ok, it's a bit dirty */
        xbt_fifo_shift(fifo);
        xbt_fifo_push(fifo, messages[i]);
    }
    xbt_free(messages);
}

