//
// Copyright (C) 2004 OpenSim Ltd.
// Copyright (C) 2009-2010 Thomas Reschka
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

#ifndef __INET_TCPCONGESTIONCONTROLBSD_H
#define __INET_TCPCONGESTIONCONTROLBSD_H

#include "inet/transportlayer/tcp/TcpAlgorithm.h"

namespace inet {
namespace tcp {

/**
 * Abstract base class for TCP algorithms which encapsulate all behaviour
 * during data transfer state: flavour of congestion control, fast
 * retransmit/recovery, selective acknowledgement etc. Subclasses
 * may implement various sets and flavours of the above algorithms.
 */
class INET_API TcpCongestionControlBsd : protected TcpAlgorithm
{
  protected:
    enum CcVarFlags {
        CCF_ABC_SENTAWND = 0x0001,  // ABC counted cwnd worth of bytes?
        CCF_CWND_LIMITED = 0x0002,  /* Are we currently cwnd limited? */
        CCF_USE_LOCAL_ABC = 0x0004,  /* Dont use the system l_abc val */
        CCF_ACKNOW = 0x0008,  /* Will this ack be sent now? */
        CCF_IPHDR_CE = 0x0010,  /* Does this packet set CE bit? */
        CCF_TCPHDR_CWR = 0x0020,  /* Does this packet set CWR bit? */
        CCF_MAX_CWND = 0x0040,  /* Have we reached maximum cwnd? */
        CCF_CHG_MAX_CWND = 0x0080,  /* CUBIC max_cwnd changed, for K */
        CCF_USR_IWND = 0x0100,  /* User specified initial window */
        CCF_USR_IWND_INIT_NSEG = 0x0200,  /* Convert segs to bytes on conn init */
        CCF_HYSTART_ALLOWED = 0x0400,  /* If the CC supports it Hystart is allowed */
        CCF_HYSTART_CAN_SH_CWND = 0x0800,  /* Can hystart when going CSS -> CA slam the cwnd */
        CCF_HYSTART_CONS_SSTH = 0x1000  /* Should hystart use the more conservative ssthresh */
    };
    /*
     * Wrapper around transport structs that contain same-named congestion
     * control variables. Allows algos to be shared amongst multiple CC aware
     * transprots.
     */
    struct cc_var {
        void *cc_data; /* Per-connection private CC algorithm data. */
        int bytes_this_ack; /* # bytes acked by the current ACK. */
        tcp_seq curack; /* Most recent ACK. */
        uint32_t flags; /* Flags for cc_var (see below) */
        int type; /* Indicates which ptr is valid in ccvc. */
        union ccv_container {
            struct tcpcb        *tcp;
            struct sctp_nets    *sctp;
        } ccvc;
        uint16_t nsegs; /* # segments coalesced into current chain. */
        uint8_t labc;  /* Dont use system abc use passed in */
    };


/////////////////////////////////////////////////////////////////////////////////////////////////

  public:
    /**
     * Ctor.
     */
    TcpCongestionControlBsd() {}

    /**
     * Virtual dtor.
     */
    virtual ~TcpCongestionControlBsd() {}

    /**
     * Assign this object to a TcpConnection. Its sendQueue and receiveQueue
     * must be set already at this time, because we cache their pointers here.
     */
    void setConnection(TcpConnection *_conn) { }

    /**
     * Creates and returns the TCP state variables.
     */
    TcpStateVariables *getStateVariables()
    {
        if (!state)
            state = createStateVariables();

        return state;
    }

    /**
     * Should be redefined to initialize the object: create timers, etc.
     * This method is necessary because the TcpConnection ptr is not
     * available in the constructor yet.
     */
    virtual void initialize() {}

    /**
     * Called when the connection is going to ESTABLISHED from SYN_SENT or
     * SYN_RCVD. This is a place to initialize some variables (e.g. set
     * cwnd to the MSS learned during connection setup). If we are on the
     * active side, here we also have to finish the 3-way connection setup
     * procedure by sending an ACK, possibly piggybacked on data.
     */
    virtual void established(bool active) = 0;

    /**
     * Called when the connection closes, it should cancel all running timers.
     */
    virtual void connectionClosed() = 0;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    /* Init global module state on kldload. */
    virtual int mod_init();

    /* Cleanup global module state on kldunload. */
    int mod_destroy();

    /* Return the size of the void pointer the CC needs for state */
    size_t  cc_data_sz();

    /*
     * Init CC state for a new control block. The CC
     * module may be passed a NULL ptr indicating that
     * it must allocate the memory. If it is passed a
     * non-null pointer it is pre-allocated memory by
     * the caller and the cb_init is expected to use that memory.
     * It is not expected to fail if memory is passed in and
     * all currently defined modules do not.
     */
    int cb_init(struct cc_var *ccv, void *ptr);

    /* Cleanup CC state for a terminating control block. */
    void cb_destroy(struct cc_var *ccv);

    /* Init variables for a newly established connection. */
    void conn_init(struct cc_var *ccv);

    /* Called on receipt of an ack. */
    void ack_received(struct cc_var *ccv, uint16_t type);

    /* Called on detection of a congestion signal. */
    void cong_signal(struct cc_var *ccv, uint32_t type);

    /* Called after exiting congestion recovery. */
    void post_recovery(struct cc_var *ccv);

    /* Called when data transfer resumes after an idle period. */
    void after_idle(struct cc_var *ccv);

    /* Called for an additional ECN processing apart from RFC3168. */
    void ecnpkt_handler(struct cc_var *ccv);

    /* Called when a new "round" begins, if the transport is tracking rounds.  */
    void newround(struct cc_var *ccv, uint32_t round_cnt);

    /*
     *  Called when a RTT sample is made (fas = flight at send, if you dont have it
     *  send the cwnd in).
     */
    void    (*rttsample)(struct cc_var *ccv, uint32_t usec_rtt, uint32_t rxtcnt, uint32_t fas);

    /* Called for {get|set}sockopt() on a TCP socket with TCP_CCALGOOPT. */
    int     (*ctl_output)(struct cc_var *, struct sockopt *, void *);

};

} // namespace tcp
} // namespace inet

#endif

