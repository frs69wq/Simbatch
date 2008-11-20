/****************************************************************************/
/* This file is part of the Simbatch project                                */
/* written by Jean-Sebastien Gay, ENS Lyon                                  */
/*                                                                          */
/* Copyright (c) 2007 Jean-Sebastien Gay. All rights reserved.              */
/*                                                                          */
/* This program is free software; you can redistribute it and/or modify it  */
/* under the terms of the license (GNU LGPL) which comes with this package. */
/****************************************************************************/


#ifndef _BATCH_H_
#define _BATCH_H_

/*
 * Simulates the behavior of a Batch system
 * MSG_tasks to use when calling the function:
 * SB_TASK
 * SB_RES to make reservations
 * SB_ACK when a task has been done
 * SB_DIET when working with DIET
 * SED_PRED to perform a prediction of when the task will 
 *   be able to execute
 * SED_HPF 
 * PF_INIT to initialize the batch
 */
int SB_batch(int argc, char ** argv);

#endif
