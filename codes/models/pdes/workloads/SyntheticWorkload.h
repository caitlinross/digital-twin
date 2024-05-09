//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#ifndef SYNTHETIC_WORKLOAD_H
#define SYNTHETIC_WORKLOAD_H

#include <ross.h>

#include "codes/model-net/model-net.h"

// TODO: make configurable
#define PAYLOAD_SZ 2048

/* type of events */
enum class SyntheticWorkloadEventTypes
{
  KICKOFF, /* kickoff event */
  REMOTE,  /* remote event */
  LOCAL    /* local event */
};

#ifdef __cplusplus
extern "C"
{
#endif

  struct SyntheticWorkloadState
  {
    int msg_sent_count;    /* requests sent */
    int msg_recvd_count;   /* requests recvd */
    int local_recvd_count; /* number of local messages received */
    tw_stime start_ts;     /* time that we started sending requests */
    tw_stime end_ts;       /* time that we ended sending requests */
  };

  struct SyntheticWorkloadMsg
  {
    SyntheticWorkloadEventTypes svr_event_type;
    tw_lpid src;                     /* source of this request or ack */
    int incremented_flag;            /* helper for reverse computation */
    model_net_event_return event_rc; /* model-net event reverse computation flag */
  };

#ifdef __cplusplus
}
#endif

#endif // SYNTHETIC_WORKLOAD_H
