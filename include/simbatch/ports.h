/*******************************/
/* Author : Jean-Sebastien Gay */
/* Date : 2006                 */
/*                             */
/* Project Name : SimBatch     */
/*******************************/

#ifndef _PORTS_H_
#define _PORTS_H_

/*** Batch ports ***/ 
#define CLIENT_PORT 0
#define RSC_MNG_PORT 1 // Resource manager port
#define NODE_PORT 2
#define BATCH_OUT 3 // communication with the rest of the world

/* Ports > 1000 reserved for the pool of supervisors */
#define SUPERVISOR_PORT 1000

#endif
