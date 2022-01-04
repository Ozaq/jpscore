/**
 * \file        AccessPoint.cpp
 * \date        Aug 24, 2010
 * \version     v0.7
 * \copyright   <2009-2015> Forschungszentrum Jülich GmbH. All rights reserved.
 *
 * \section License
 * This file is part of JuPedSim.
 *
 * JuPedSim is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * JuPedSim is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with JuPedSim. If not, see <http://www.gnu.org/licenses/>.
 *
 * \section Description
 *
 *
 **/
#include "AccessPoint.h"

#include <Logger.h>

AccessPoint::AccessPoint(int id, double center[2], double radius)
{
    _id                = id;
    _center[0]         = center[0];
    _center[1]         = center[1];
    _radius            = radius;
    _finaExitToOutside = false;
    _finalGoalOutside  = false;
    _room1ID           = -1;
    _room2ID           = -1;
    _connectingAPs.clear();
    _mapDestToDist.clear();
    pCentre             = Point(center[0], center[1]);
    _transitPedestrians = std::vector<Pedestrian *>();
    _connectingAPs      = std::vector<AccessPoint *>();
    //     _isClosed=0
    _navLine = nullptr;
    _state   = DoorState::OPEN;
}

AccessPoint::~AccessPoint()
{
    if(_navLine)
        delete _navLine;
}

int AccessPoint::GetID()
{
    return _id;
}

bool AccessPoint::IsClosed()
{
    return _state == DoorState::CLOSE;
}

//void AccessPoint::SetClosed(int isClosed)
//{
//     _isClosed=isClosed;
//}

void AccessPoint::SetFinalExitToOutside(bool isFinal)
{
    _finaExitToOutside = isFinal;
}

bool AccessPoint::GetFinalExitToOutside()
{
    return _finaExitToOutside;
}

const Point & AccessPoint::GetCentre() const
{
    return pCentre;
}

void AccessPoint::SetFinalGoalOutside(bool isFinal)
{
    _finalGoalOutside = isFinal;
}

bool AccessPoint::GetFinalGoalOutside()
{
    return _finalGoalOutside;
}

//TODO: possibly remove
void AccessPoint::AddIntermediateDest(int final, int inter)
{
    _mapDestToAp[final] = inter;
}

void AccessPoint::AddFinalDestination(int UID, double distance)
{
    _mapDestToDist[UID] = distance;
}

double AccessPoint::GetDistanceTo(int UID)
{
    //this is probably a final destination
    if(_mapDestToDist.count(UID) == 0) {
        LOG_ERROR("No route to destination  [{:d}]", UID);
        //return 0;
        exit(EXIT_FAILURE);
    }
    return _mapDestToDist[UID];
}

double AccessPoint::GetDistanceTo(AccessPoint * ap)
{
    return (pCentre - ap->GetCentre()).Norm();
}

void AccessPoint::AddConnectingAP(AccessPoint * ap)
{
    //only add of not already inside
    for(unsigned int p = 0; p < _connectingAPs.size(); p++) {
        if(_connectingAPs[p]->GetID() == ap->GetID())
            return;
    }
    _connectingAPs.push_back(ap);
}

//TODO: remove this one
int AccessPoint::GetNextApTo(int UID)
{
    //this is probably a final destination
    if(_mapDestToAp.count(UID) == 0) {
        LOG_ERROR("No route to destination  [{:d}]", UID);
        exit(EXIT_FAILURE);
    }
    return _mapDestToAp[UID];
}

int AccessPoint::GetNearestTransitAPTO(int UID)
{
    const std::vector<AccessPoint *> & possibleDest = _navigationGraphTo[UID];

    if(possibleDest.size() == 0) {
        return -1;
    } else if(possibleDest.size() == 1) {
        return possibleDest[0]->GetID();
    } else {
        AccessPoint * best_ap = possibleDest[0];
        double min_dist =
            GetDistanceTo(best_ap) +
            best_ap->GetDistanceTo(UID); // FIXME: add the shortest distance to outside

        for(unsigned int i = 0; i < possibleDest.size(); i++) {
            double tmp = GetDistanceTo(possibleDest[i]);
            if(tmp < min_dist) {
                min_dist = tmp;
                best_ap  = possibleDest[i];
            }
        }
        return best_ap->GetID();
    }
}

void AccessPoint::setConnectingRooms(int r1, int r2)
{
    _room1ID = r1;
    _room2ID = r2;
}

double AccessPoint::DistanceTo(double x, double y)
{
    return sqrt((x - _center[0]) * (x - _center[0]) + (y - _center[1]) * (y - _center[1]));
}


bool AccessPoint::isInRange(int roomID)
{
    if((roomID != _room1ID) && (roomID != _room2ID)) {
        return false;
    }
    return true;
}

bool AccessPoint::IsInRange(double xPed, double yPed, int roomID)
{
    if((roomID != _room1ID) && (roomID != _room2ID)) {
        return false;
    }
    if(((xPed - _center[0]) * (xPed - _center[0]) + (yPed - _center[1]) * (yPed - _center[1])) <=
       _radius * _radius)
        return true;

    return false;
}

void AccessPoint::SetNavLine(Line * line)
{
    _navLine = new Line(*line);
}

Line * AccessPoint::GetNavLine() const
{
    return _navLine;
}

const std::vector<AccessPoint *> & AccessPoint::GetConnectingAPs()
{
    return _connectingAPs;
}

void AccessPoint::RemoveConnectingAP(AccessPoint * ap)
{
    std::vector<AccessPoint *>::iterator it;
    it = find(_connectingAPs.begin(), _connectingAPs.end(), ap);
    if(it == _connectingAPs.end()) {
        LOG_WARNING("AP {} RemoveConnection: There is no connection to AP {}", _id, ap->GetID());
    } else {
        _connectingAPs.erase(it);
    }
}

const std::vector<AccessPoint *> & AccessPoint::GetTransitAPsTo(int UID)
{
    return _navigationGraphTo[UID];
}

void AccessPoint::AddTransitAPsTo(int UID, AccessPoint * ap)
{
    _navigationGraphTo[UID].push_back(ap);
}

void AccessPoint::Reset(int UID)
{
    _navigationGraphTo[UID].clear();
}


void AccessPoint::SetFriendlyName(const std::string & name)
{
    _friendlyName = name;
}


const std::string AccessPoint::GetFriendlyName()
{
    return _friendlyName;
}

void AccessPoint::SetState(DoorState state)
{
    _state = state;
}

DoorState AccessPoint::GetState() const
{
    return _state;
}
