//
// Copyright (C) 2005 OpenSim Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//


#include "inet/mobility/base/LineSegmentsMobilityBase.h"

#include "inet/common/INETMath.h"

namespace inet {

LineSegmentsMobilityBase::LineSegmentsMobilityBase()
{
    targetPosition = Coord::ZERO;
}

void LineSegmentsMobilityBase::initializePosition()
{
    MobilityBase::initializePosition();
    doSetTargetPosition();
    lastUpdate = simTime();
    scheduleUpdate();
}

void LineSegmentsMobilityBase::doSetTargetPosition()
{
    segmentStartPosition = lastPosition;
    segmentStartTime = simTime();
    setTargetPosition();
    EV_INFO << "new target position = " << targetPosition << ", next change = " << nextChange << endl;
    lastVelocity = segmentStartVelocity = stationary ? Coord::ZERO : (targetPosition - segmentStartPosition) / (nextChange - segmentStartTime).dbl();
}

void LineSegmentsMobilityBase::processBorderPolicy()
{
    Coord dummyCoord;
    handleIfOutside(borderPolicy, dummyCoord, lastVelocity);
}

void LineSegmentsMobilityBase::move()
{
    simtime_t now = simTime();
    if (now == nextChange) {
        lastPosition = targetPosition;
        lastVelocity = segmentStartVelocity;
        processBorderPolicy();
        targetPosition = lastPosition;
        EV_INFO << "reached current target position = " << lastPosition << endl;
        doSetTargetPosition();
    }
    else if (now > lastUpdate) {
        ASSERT(nextChange == -1 || now < nextChange);
        lastPosition = segmentStartPosition + segmentStartVelocity * (now - segmentStartTime).dbl();
        lastVelocity = segmentStartVelocity;
        processBorderPolicy();
    }
}

} // namespace inet

