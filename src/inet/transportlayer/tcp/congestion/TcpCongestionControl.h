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

