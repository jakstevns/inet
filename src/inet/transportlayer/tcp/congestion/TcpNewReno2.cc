/*
 * Copyright (c) 2015 Natale Patriciello <natale.patriciello@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#include "inet/transportlayer/tcp/congestion/TcpNewReno2.h"

namespace inet {
namespace tcp {

// RENO

Register_Class(TcpNewReno2);


TcpNewReno2::TcpNewReno2()
    : TcpCongestionControl(),
    state((TcpNewReno2StateVariables *&)TcpAlgorithm::state)
{
}

TcpNewReno2::~TcpNewReno2()
{
}

/**
 * \brief Tcp NewReno slow start algorithm
 *
 * Defined in RFC 5681 as
 *
 * > During slow start, a TCP increments cwnd by at most SMSS bytes for
 * > each ACK received that cumulatively acknowledges new data.  Slow
 * > start ends when cwnd exceeds ssthresh (or, optionally, when it
 * > reaches it, as noted above) or when congestion is observed.  While
 * > traditionally TCP implementations have increased cwnd by precisely
 * > SMSS bytes upon receipt of an ACK covering new data, we RECOMMEND
 * > that TCP implementations increase cwnd, per:
 * >
 * >    cwnd += min (N, SMSS)                      (2)
 * >
 * > where N is the number of previously unacknowledged bytes acknowledged
 * > in the incoming ACK.
 *
 * The ns-3 implementation respect the RFC definition. Linux does something
 * different:
 * \verbatim
u32 tcp_slow_start(struct tcp_sock *tp, u32 acked)
  {
    u32 cwnd = tp->snd_cwnd + acked;

    if (cwnd > tp->snd_ssthresh)
      cwnd = tp->snd_ssthresh + 1;
    acked -= cwnd - tp->snd_cwnd;
    tp->snd_cwnd = min(cwnd, tp->snd_cwnd_clamp);

    return acked;
  }
  \endverbatim
 *
 * As stated, we want to avoid the case when a cumulative ACK increases cWnd more
 * than a segment size, but we keep count of how many segments we have ignored,
 * and return them.
 *
 * \param state internal congestion state
 * \param segmentsAcked count of segments acked
 * \return the number of segments not considered for increasing the cWnd
 */
uint32_t
TcpNewReno2::slowStart(uint32_t segmentsAcked)
{
    EV << state << segmentsAcked << endl;

    if (segmentsAcked >= 1)
    {
        state->snd_cwnd += state->snd_mss;
        EV << "In SlowStart, updated to cwnd " << state->snd_cwnd << " ssthresh "
                                                     << state->ssthresh << endl;
        return segmentsAcked - 1;
    }

    return 0;
}

/**
 * \brief NewReno congestion avoidance
 *
 * During congestion avoidance, cwnd is incremented by roughly 1 full-sized
 * segment per round-trip time (RTT).
 *
 * \param state internal congestion state
 * \param segmentsAcked count of segments acked
 */
void
TcpNewReno2::congestionAvoidance(uint32_t segmentsAcked)
{
    EV << state << segmentsAcked << endl;

    if (segmentsAcked > 0)
    {
        double adder =
            static_cast<double>(state->snd_mss * state->snd_mss) / state->snd_cwnd;
        adder = std::max(1.0, adder);
        state->snd_cwnd += static_cast<uint32_t>(adder);
        EV << "In CongAvoid, updated to cwnd " << state->snd_cwnd << " ssthresh "
                                                     << state->ssthresh << endl;
    }
}

/**
 * \brief Try to increase the cWnd following the NewReno specification
 *
 * \see SlowStart
 * \see CongestionAvoidance
 *
 * \param state internal congestion state
 * \param segmentsAcked count of segments acked
 */
void
TcpNewReno2::increaseWindow(uint32_t segmentsAcked)
{
    EV << state << segmentsAcked << endl;

    if (state->snd_cwnd < state->ssthresh)
    {
        segmentsAcked = slowStart(segmentsAcked);
    }

    if (state->snd_cwnd >= state->ssthresh)
    {
        congestionAvoidance(segmentsAcked);
    }

    /* At this point, we could have segmentsAcked != 0. This because RFC says
     * that in slow start, we should increase cWnd by min (N, SMSS); if in
     * slow start we receive a cumulative ACK, it counts only for 1 SMSS of
     * increase, wasting the others.
     *
     * // Incorrect assert, I am sorry
     * NS_ASSERT (segmentsAcked == 0);
     */
}

uint32_t
TcpNewReno2::getSsThresh(uint32_t bytesInFlight)
{
    EV << state << bytesInFlight << endl;

    return std::max(2 * state->snd_mss, bytesInFlight / 2);
}

} // namespace tcp
} // namespace inet
