/****************************************************************************/
/* This file is part of the Simbatch project                                */
/* written by Jean-Sebastien Gay, ENS Lyon                                  */
/*                                                                          */
/* Copyright (c) 2007 Jean-Sebastien Gay. All rights reserved.              */
/*                                                                          */
/* This program is free software; you can redistribute it and/or modify it  */
/* under the terms of the license (GNU LGPL) which comes with this package. */
/****************************************************************************/


/**
 * \file node.h
 * Define the SB_node process.
 * Every SB_process process is a MSG_process.
 */


#ifndef _NODE_H_
#define _NODE_H_

/**
 * SB_node process.
 * A node or comtpuation node represents a node of a cluster. Its role is to
 * execute jobs sent by the SB_batch process. Nodes are created by simgrid from
 * the deployments.xml and platform.xml files.
 * \param argc number of parmeters in **argv.
 * \param **argv array containing the parameters.
 */
int
SB_node(int argc, char ** argv);

#endif
