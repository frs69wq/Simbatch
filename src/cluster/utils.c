/****************************************************************************/
/* This file is part of the Simbatch project                                */
/* written by Jean-Sebastien Gay, ENS Lyon                                  */
/*                                                                          */
/* Copyright (c) 2007 Jean-Sebastien Gay. All rights reserved.              */
/*                                                                          */
/* This program is free software; you can redistribute it and/or modify it  */
/* under the terms of the license (GNU LGPL) which comes with this package. */
/****************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <xbt/sysdep.h>
#include <xbt/fifo.h>
#include <msg/msg.h>

#include "utils.h"

typedef struct _async_param_t { 
    m_task_t task;
    char * mailbox;
} _async_param_t;

void
xbt_fifo_alphabetically_sort(xbt_fifo_t fifo)
{
    auto int
    stringCmp(const void *t1, const void *t2);
    
    int 
    stringCmp(const void *t1, const void *t2) { 
        return strcmp((*((m_task_t *)t1))->name, (*((m_task_t *)t2))->name);
    }

    int i = 0;
    m_task_t *messages = (m_task_t *)xbt_fifo_to_array(fifo);
    
    qsort(messages, xbt_fifo_size(fifo), sizeof(m_task_t), stringCmp);
    for (i=0; i<xbt_fifo_size(fifo); ++i) {
        /* ok, it's a bit dirty */
        xbt_fifo_shift(fifo);
        xbt_fifo_push(fifo, messages[i]);
    }
    xbt_free(messages);
}

static int
_MSG_task_send(int argc, char **argv)
{
    int err;
    _async_param_t *p = NULL;
    
    p = (_async_param_t *)MSG_process_get_data(MSG_process_self());
    err = MSG_task_send(p->task, p->mailbox);
    xbt_free(p->mailbox);
    xbt_free(p);
    
    return err;
}


m_process_t
MSG_task_async_send(m_task_t task, char * mailbox)
{
    /* can't use a nested function for this :( */
    _async_param_t *param = xbt_malloc(sizeof(*param));
    param->task = task;
    param->mailbox = xbt_malloc(256 * sizeof(char)); 
    strcpy(param->mailbox, mailbox);
    return MSG_process_create("asyncSend", _MSG_task_send, (void *)param,
                              MSG_host_self()); 
}


