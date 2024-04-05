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

// this is based on the modelnet-simplep2p-test.c test

#include <ross.h>

#include "codes/config/configuration.h"
#include "codes/mapping/codes-mapping.h"
#include "codes/model-net/codes.h"
#include "codes/model-net/lp-io.h"
#include "codes/model-net/lp-type-lookup.h"
#include "codes/model-net/model-net.h"

#define NUM_REQS 3  /* number of requests sent by each server */
#define PAYLOAD_SZ 2048 /* size of simulated data payload, bytes  */

/* types of events that will constitute triton requests */
enum class SimpleServerEventTypes
{
    KICKOFF,    /* initial event */
    REQ,        /* request event */
    ACK,        /* ack event */
    LOCAL      /* local event */
};

#ifdef __cplusplus
extern "C" {
#endif

struct SimpleServerState
{
    int msg_sent_count;   /* requests sent */
    int msg_recvd_count;  /* requests recvd */
    int local_recvd_count; /* number of local messages received */
    tw_stime start_ts;    /* time that we started sending requests */
};

struct SimpleServerMsg
{
    SimpleServerEventTypes EventType;
    tw_lpid src;          /* source of this request or ack */
    model_net_event_return ret;
    int incremented_flag; /* helper for reverse computation */
};

extern void SimpleServerAddLPType();

// TODO just created to make this work for now, but need to improve this
extern void SimpleServerSetNetId(int net_id);

#ifdef __cplusplus
}
#endif

#endif // SIMPLE_SERVER_H
