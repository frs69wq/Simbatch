/****************************************************************************/
/* This file is part of the Simbatch project                                */
/* written by Jean-Sebastien Gay, ENS Lyon                                  */
/*                                                                          */
/* Copyright (c) 2007 Jean-Sebastien Gay. All rights reserved.              */
/*                                                                          */
/* This program is free software; you can redistribute it and/or modify it  */
/* under the terms of the license (GNU LGPL) which comes with this package. */
/****************************************************************************/


#ifndef _UTILS_H_
#define _UTILS_H_

#include <xbt/fifo.h>
#include <msg/msg.h>

/* Return the tab's size */
#define TAB_SIZE(t) sizeof(t)/sizeof(*t)

/* Return the name of the host calling the macro */
#define HOST_NAME() MSG_host_get_name(MSG_host_self())

/* Return the name of the process calling the macro */
#define PROCESS_NAME() MSG_process_get_name(MSG_process_self())

void
xbt_fifo_sort(xbt_fifo_t fifo);

m_process_t
MSG_task_async_put(m_task_t task, m_host_t host, m_channel_t channel);

m_process_t
MSG_task_async_send(m_task_t task, char * mailbox);

#endif
