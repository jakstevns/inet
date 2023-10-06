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
#ifndef TCPNEWRENO2_H
#define TCPNEWRENO2_H

#include "inet/transportlayer/tcp/congestion/TcpCongestionControl.h"

namespace inet {
namespace tcp {

struct TcpNewReno2StateVariables : TcpBaseAlgStateVariables
{
    uint32_t ssthresh = 0;
};

/**
 * \brief The NewReno implementation
 *
 * New Reno introduces partial ACKs inside the well-established Reno algorithm.
 * This and other modifications are described in RFC 6582.
 *
 * \see IncreaseWindow
 */
class TcpNewReno2 : public TcpCongestionControl
{
  protected:
    TcpNewReno2StateVariables *& state; // alias to TcpAlgorithm's 'state'

    /** Create and return a TcpNewRenoStateVariables object. */
    virtual TcpStateVariables *createStateVariables() override {
        return new TcpNewReno2StateVariables();
    }

  public:
    TcpNewReno2();

    virtual ~TcpNewReno2() override;

    void increaseWindow(uint32_t segmentsAcked) override;
    uint32_t getSsThresh(uint32_t bytesInFlight) override;

  protected:
    virtual uint32_t slowStart(uint32_t segmentsAcked);
    virtual void congestionAvoidance(uint32_t segmentsAcked);
};

} // namespace
} // namespace

#endif // TCPCONGESTIONOPS_H
