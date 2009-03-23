/****************************************************************************/
/* This file is part of the Simbatch project.                               */
/* written by Jean-Sebastien Gay and Ghislain Charrier, ENS Lyon.           */
/*                                                                          */
/* Copyright (c) 2007, Simbatch Team. All rights reserved.                  */
/*                                                                          */
/* This program is free software; you can redistribute it and/or modify it  */
/* under the terms of the license (GNU LGPL) which comes with this package. */
/****************************************************************************/


#ifndef _BATCH_H_
#define _BATCH_H_

/**
 * \file batch.h
 * Define the batch process.
 */


/**
 * Simulates the behavior of a Batch system.
 *
 * The behaviour of the batch process consists in responding to incoming 
 * messages and to schedule jobs sent by clients. Messages are MSG_task 
 * datatype provided by the simgrid library.
 *
 * Here is a short description of the different tasks received:
 * SB_TASK conatins the job to schedule.
 * SB_RES to make reservations.
 * SB_ACK when a task has been done on a cpu. 5 cpus for a task => 5 SB_ACK.
 * SB_DIET to allow Diet for using Simbatch.
 * SED_PRED to perform a prediction of when the task will be able to execute.
 * SED_HPF (work in progess)
 * PF_INIT to initialize the batch
 *
 * \param argc number of parameters transmitted to the SB_batch process.
 * \param **argv array containing the parameters. argc and argc are 
 * automacilly filled by simgrid when parsing the deployment.xml file.
 * \return an error code.
 */
int 
SB_batch(int argc, char **argv);

#endif
