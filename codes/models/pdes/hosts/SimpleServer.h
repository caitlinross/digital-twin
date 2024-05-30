//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#ifndef SIMPLE_SERVER_H
#define SIMPLE_SERVER_H

// this is based on the modelnet-simplep2p-test.c and modelnet-test.c tests
// from the original codes lib

#include <ross.h>

#include "codes/model-net/model-net.h"

/* types of events that will constitute triton requests */
enum class SimpleServerEventTypes
{
  KICKOFF, /* initial event */
  REQ,     /* request event */
  ACK,     /* ack event */
  LOCAL    /* local event */
};

#ifdef __cplusplus
extern "C"
{
#endif

  struct SimpleServerState
  {
    int msg_sent_count;    /* requests sent */
    int msg_recvd_count;   /* requests recvd */
    int local_recvd_count; /* number of local messages received */
    tw_stime start_ts;     /* time that we started sending requests */
  };

  struct SimpleServerMsg
  {
    SimpleServerEventTypes EventType;
    tw_lpid src; /* source of this request or ack */
    model_net_event_return ret;
    int incremented_flag; /* helper for reverse computation */
  };

#ifdef __cplusplus
}
#endif

#endif // SIMPLE_SERVER_H
