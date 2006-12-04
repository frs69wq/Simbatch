/*******************************/
/* Author : Jean-Sebastien Gay */
/* Date : 2006                 */
/*                             */
/* Project Name : SimBatch     */
/*******************************/

#ifndef _PARSER_H_
#define _PARSER_H_

#include <xbt/fifo.h>

void print(const int * t, const unsigned char);
xbt_fifo_t create_job_from_swf_file();

#endif
