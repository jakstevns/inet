//
// Copyright (C) 2004 OpenSim Ltd.
// Copyright (C) 2009-2010 Thomas Reschka
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

#ifndef __INET_TCPCONGESTIONCONTROLBSD_H
#define __INET_TCPCONGESTIONCONTROLBSD_H

#include "inet/transportlayer/tcp/flavours/TcpBaseAlg.h"

namespace inet {
namespace tcp {

enum TcpCongState_t
{
    CA_OPEN,     //!< Normal state, no dubious events
    CA_DISORDER, //!< In all the respects it is "Open",
                 //!< but requires a bit more attention. It is entered when we see some SACKs or
                 //!< dupacks. It is split of "Open".
    CA_CWR,      //!< cWnd was reduced due to some congestion notification event, such as ECN,
                 //!< ICMP source quench, local device congestion.
    CA_RECOVERY, //!< CWND was reduced, we are fast-retransmitting.
    CA_LOSS,     //!< CWND was reduced due to RTO timeout or SACK reneging.
    CA_LAST_STATE //!< Used only in debug messages
};

// Note: "not triggered" events are currently not triggered by the code.
/**
 * \brief Congestion avoidance events
 */
enum TcpCAEvent_t
{
    CA_EVENT_TX_START,        //!< first transmit when no packets in flight
    CA_EVENT_CWND_RESTART,    //!< congestion window restart. Not triggered
    CA_EVENT_COMPLETE_CWR,    //!< end of congestion recovery
    CA_EVENT_LOSS,            //!< loss timeout
    CA_EVENT_ECN_NO_CE,       //!< ECT set, but not CE marked. Not triggered
    CA_EVENT_ECN_IS_CE,       //!< received CE marked IP packet. Not triggered
    CA_EVENT_DELAYED_ACK,     //!< Delayed ack is sent
    CA_EVENT_NON_DELAYED_ACK, //!< Non-delayed ack is sent
};

/**
 * \brief Parameter value related to ECN enable/disable functionality
 *        similar to sysctl for tcp_ecn. Currently value 2 from
 *        https://www.kernel.org/doc/Documentation/networking/ip-sysctl.txt
 *        is not implemented.
 */
enum UseEcn_t
{
    Off = 0,        //!< Disable
    On = 1,         //!< Enable
    AcceptOnly = 2, //!< Enable only when the peer endpoint is ECN capable
};

/**
 * \brief ECN code points
 */
enum EcnCodePoint_t
{
    NotECT = 0,  //!< Unmarkable
    Ect1 = 1,    //!< Markable
    Ect0 = 2,    //!< Markable
    CongExp = 3, //!< Marked
};

/**
 * \brief ECN Modes
 */
enum EcnMode_t
{
    ClassicEcn, //!< ECN functionality as described in RFC 3168.
    DctcpEcn,   //!< ECN functionality as described in RFC 8257. Note: this mode is specific to
                //!< DCTCP.
};

/**
 * \brief Definition of the Ecn state machine
 *
 */
enum EcnState_t
{
    ECN_DISABLED = 0, //!< ECN disabled traffic
    ECN_IDLE, //!< ECN is enabled  but currently there is no action pertaining to ECE or CWR to
              //!< be taken
    ECN_CE_RCVD,     //!< Last packet received had CE bit set in IP header
    ECN_SENDING_ECE, //!< Receiver sends an ACK with ECE bit set in TCP header
    ECN_ECE_RCVD,    //!< Last ACK received had ECE bit set in TCP header
    ECN_CWR_SENT //!< Sender has reduced the congestion window, and sent a packet with CWR bit
                 //!< set in TCP header. This state is used for tracing.
};

/**
 * Abstract base class for TCP algorithms which encapsulate all behaviour
 * during data transfer state: flavour of congestion control, fast
 * retransmit/recovery, selective acknowledgement etc. Subclasses
 * may implement various sets and flavours of the above algorithms.
 */
class INET_API TcpCongestionControl : public TcpBaseAlg
{
  public:
    TcpCongestionControl() {}

    ~TcpCongestionControl() override {}

  /**
   * \brief Get the slow start threshold after a loss event
   *
   * Is guaranteed that the congestion control state (\p TcpAckState_t) is
   * changed BEFORE the invocation of this method.
   * The implementer should return the slow start threshold (and not change
   * it directly) because, in the future, the TCP implementation may require to
   * instantly recover from a loss event (e.g. when there is a network with an high
   * reordering factor).
   *
   * \param bytesInFlight total bytes in flight
   * \return Slow start threshold
   */
  virtual uint32_t getSsThresh(uint32_t bytesInFlight) = 0;

  /**
   * \brief Congestion avoidance algorithm implementation
   *
   * Mimic the function \pname{cong_avoid} in Linux. New segments have been ACKed,
   * and the congestion control duty is to update the window.
   *
   * The function is allowed to change directly cWnd and/or ssThresh.
   *
   * \param tcb internal congestion state
   * \param segmentsAcked count of segments acked
   */
  virtual void increaseWindow(uint32_t segmentsAcked) {}

  /**
   * \brief Timing information on received ACK
   *
   * The function is called every time an ACK is received (only one time
   * also for cumulative ACKs) and contains timing information. It is
   * optional (congestion controls need not implement it) and the default
   * implementation does nothing.
   *
   * \param tcb internal congestion state
   * \param segmentsAcked count of segments acked
   * \param rtt last rtt
   */
  virtual void pktsAcked(uint32_t segmentsAcked, const SimTime& rtt) {}

  /**
   * \brief Trigger events/calculations specific to a congestion state
   *
   * This function mimics the notification function \pname{set_state} in Linux.
   * The function does not change the congestion state in the tcb; it notifies
   * the congestion control algorithm that this state is about to be changed.
   * The tcb->m_congState variable must be separately set; for example:
   *
   * \code
   *   m_congestionControl->CongestionStateSet (m_tcb, TcpSocketState::CA_RECOVERY);
   *   m_tcb->m_congState = TcpSocketState::CA_RECOVERY;
   * \endcode
   *
   * \param tcb internal congestion state
   * \param newState new congestion state to which the TCP is going to switch
   */
  virtual void congestionStateSet(const TcpCongState_t newState) {}

  /**
   * \brief Trigger events/calculations on occurrence of congestion window event
   *
   * This function mimics the function \pname{cwnd_event} in Linux.
   * The function is called in case of congestion window events.
   *
   * \param tcb internal congestion state
   * \param event the event which triggered this function
   */
  virtual void cwndEvent(const TcpCAEvent_t event) {}

  /**
   * \brief Returns true when Congestion Control Algorithm implements CongControl
   *
   * \return true if CC implements CongControl function
   *
   * This function is the equivalent in C++ of the C checks that are used
   * in the Linux kernel to see if an optional function has been defined.
   * Since CongControl is optional, not all congestion controls have it. But,
   * from the perspective of TcpSocketBase, the behavior is different if
   * CongControl is present. Therefore, this check should return true for any
   * congestion controls that implements the CongControl optional function.
   */
  virtual bool hasCongControl() const { return false; }

  /**
   * \brief Called when packets are delivered to update cwnd and pacing rate
   *
   * This function mimics the function cong_control in Linux. It is allowed to
   * change directly cWnd and pacing rate.
   *
   * \param tcb internal congestion state
   * \param rc Rate information for the connection
   * \param rs Rate sample (over a period of time) information
   */
// TODO
//  virtual void congControl(const TcpRateOps::TcpRateConnection& rc,
//                           const TcpRateOps::TcpRateSample& rs);

  // Present in Linux but not in ns-3 yet:
  /* call when ack arrives (optional) */
  //     void (*in_ack_event)(struct sock *sk, u32 flags);
  /* new value of cwnd after loss (optional) */
  //     u32  (*undo_cwnd)(struct sock *sk);
  /* hook for packet ack accounting (optional) */
  //     void (*pkts_acked)(struct sock *sk, u32 ext, int *attr, union tcp_cc_info *info);
};

} // namespace tcp
} // namespace inet

#endif

