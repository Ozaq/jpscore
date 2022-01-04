/**
 * \file        VelocityModel.cpp
 * \date        Aug. 07, 2015
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
 * Implementation of first-order model
 * 3. Velocity Model: Tordeux2015
 *
 *
 **/
#include "VelocityModel.h"

#include "Simulation.h"
#include "direction/walking/DirectionStrategy.h"
#include "geometry/SubRoom.h"
#include "geometry/Wall.h"
#include "neighborhood/NeighborhoodSearch.h"
#include "pedestrian/Pedestrian.h"

#include <Logger.h>
#include <memory>

double xRight = 26.0;
double xLeft  = 0.0;
double cutoff = 2.0;

VelocityModel::VelocityModel(
    std::shared_ptr<DirectionManager> dir,
    double aped,
    double Dped,
    double awall,
    double Dwall)
{
    _direction = dir;
    // Force_rep_PED Parameter
    _aPed = aped;
    _DPed = Dped;
    // Force_rep_WALL Parameter
    _aWall = awall;
    _DWall = Dwall;
}

void VelocityModel::ComputeNextTimeStep(double current, double deltaT, Building * building)
{
    // collect all pedestrians in the simulation.
    const auto & allPeds          = _simulation->Agents();
    std::vector<Point> result_acc = std::vector<Point>();
    result_acc.reserve(allPeds.size());
    std::vector<my_pair> spacings = std::vector<my_pair>();
    spacings.reserve(allPeds.size()); // larger than needed

    for(const auto & ped : allPeds) {
        auto [room, subroom] = building->GetRoomAndSubRoom(ped->GetPos());
        Point repPed         = Point(0, 0);
        std::vector<Pedestrian *> neighbours =
            building->GetNeighborhoodSearch().GetNeighbourhood(ped.get());

        int size = static_cast<int>(neighbours.size());
        for(int i = 0; i < size; i++) {
            Pedestrian * ped1 = neighbours[i];
            //if they are in the same subroom
            Point p1 = ped->GetPos();

            Point p2 = ped1->GetPos();

            auto [room1, subroom1] = building->GetRoomAndSubRoom(ped1->GetPos());
            //subrooms to consider when looking for neighbour for the 3d visibility
            std::vector<SubRoom *> emptyVector;
            emptyVector.push_back(subroom);
            emptyVector.push_back(subroom1);
            bool isVisible = building->IsVisible(p1, p2, emptyVector, false);
            if(!isVisible) {
                continue;
            }
            if(room == room1 && subroom == subroom1) {
                repPed += ForceRepPed(ped.get(), ped1);
            } else {
                // or in neighbour subrooms
                if(subroom->IsDirectlyConnectedWith(subroom1)) {
                    repPed += ForceRepPed(ped.get(), ped1);
                }
            }
        } // for i
        //repulsive forces to walls and closed transitions that are not my target
        Point repWall = ForceRepRoom(ped.get(), subroom);

        // calculate new direction ei according to (6)
        Point direction = e0(ped.get(), room) + repPed + repWall;
        for(int i = 0; i < size; i++) {
            Pedestrian * ped1      = neighbours[i];
            auto [room1, subroom1] = building->GetRoomAndSubRoom(ped1->GetPos());
            // calculate spacing
            // my_pair spacing_winkel = GetSpacing(ped, ped1);
            if(room == room1 && subroom == subroom1) {
                spacings.push_back(GetSpacing(ped.get(), ped1, direction));
            } else {
                // or in neighbour subrooms
                if(subroom->IsDirectlyConnectedWith(subroom1)) {
                    spacings.push_back(GetSpacing(ped.get(), ped1, direction));
                }
            }
        }
        //TODO get spacing to walls
        //TODO update direction every DT?

        // calculate min spacing
        std::sort(spacings.begin(), spacings.end(), sort_pred());
        double spacing = spacings.empty() ? 100.0 : spacings.front().first;
        //============================================================
        // TODO: Hack for Head on situations: ped1 x ------> | <------- x ped2
        if(0 && direction.NormSquare() < 0.5) {
            double pi_half = 1.57079663;
            double alpha   = pi_half * exp(-spacing);
            direction      = e0(ped.get(), room).Rotate(cos(alpha), sin(alpha));
            printf(
                "\nRotate %f, %f, norm = %f alpha = %f, spacing = %f\n",
                direction._x,
                direction._y,
                direction.NormSquare(),
                alpha,
                spacing);
            getc(stdin);
        }
        //============================================================
        Point speed = direction.Normalized() * OptimalSpeed(ped.get(), spacing);
        result_acc.push_back(speed);
        spacings.clear();
    }

    // update
    size_t counter = 0;
    for(auto & ped : allPeds) {
        Point v_neu   = result_acc[counter];
        Point pos_neu = ped->GetPos() + v_neu * deltaT;
        //only update the position if the velocity is above a threshold
        if(v_neu.Norm() >= J_EPS_V) {
            ped->SetPhiPed();
        }
        if(!ped->InPremovement(current)) {
            ped->SetPos(pos_neu);
            ped->SetV(v_neu);
        }
        ++counter;
    }
}

Point VelocityModel::e0(Pedestrian * ped, Room * room) const
{
    Point target;

    if(_direction) {
        // target is where the ped wants to be after the next timestep
        target = _direction->GetTarget(room, ped);
    } else { //@todo: we need a model for waiting pedestrians
        LOG_WARNING("VelocityModel::e0 Ped {} has no navline.", ped->GetUID());
        // set random destination
        std::mt19937 mt(ped->GetBuilding()->GetConfig()->seed);
        std::uniform_real_distribution<double> dist(0, 1.0);
        double random_x = dist(mt);
        double random_y = dist(mt);
        Point P1        = Point(ped->GetPos()._x - random_x, ped->GetPos()._y - random_y);
        Point P2        = Point(ped->GetPos()._x + random_x, ped->GetPos()._y + random_y);
        const Line L(P1, P2);
        ped->SetExitLine(&L);
        target = P1;
    }
    Point desired_direction;
    const Point pos = ped->GetPos();
    const auto dist = ped->GetExitLine().DistTo(pos);
    // check if the molified version works
    Point lastE0 = ped->GetLastE0();
    ped->SetLastE0(target - pos);

    if(std::dynamic_pointer_cast<DirectionLocalFloorfield>(_direction->GetDirectionStrategy())) {
        desired_direction = target - pos;
        if(desired_direction.NormSquare() < 0.25 && !ped->IsWaiting()) {
            desired_direction = lastE0;
            ped->SetLastE0(lastE0);
        }
    } else if(dist > J_EPS_GOAL) {
        desired_direction = ped->GetV0(target);
    } else {
        ped->SetSmoothTurning();
        desired_direction = ped->GetV0();
    }
    return desired_direction;
}


double VelocityModel::OptimalSpeed(Pedestrian * ped, double spacing) const
{
    double v0    = ped->GetV0Norm();
    double T     = ped->GetT();
    double l     = 2 * ped->GetEllipse().GetBmax(); //assume peds are circles with const radius
    double speed = (spacing - l) / T;
    speed        = (speed > 0) ? speed : 0;
    speed        = (speed < v0) ? speed : v0;
    //      (1-winkel)*speed;
    //todo use winkel
    return speed;
}

// return spacing and id of the nearest pedestrian
my_pair VelocityModel::GetSpacing(Pedestrian * ped1, Pedestrian * ped2, Point ei) const
{
    Point distp12   = ped2->GetPos() - ped1->GetPos(); // inversed sign
    double Distance = distp12.Norm();
    double l        = 2 * ped1->GetEllipse().GetBmax();
    Point ep12;
    if(Distance >= J_EPS) {
        ep12 = distp12.Normalized();
    } else {
        LOG_WARNING(
            "VelocityModel::GetSPacing() ep12 can not be calculated! Pedestrians are to close "
            "to "
            "each other ({:f})",
            Distance);
        exit(EXIT_FAILURE); //TODO
    }

    double condition1 = ei.ScalarProduct(ep12); // < e_i , e_ij > should be positive
    double condition2 =
        ei.Rotate(0, 1).ScalarProduct(ep12); // theta = pi/2. condition2 should <= than l/Distance
    condition2 = (condition2 > 0) ? condition2 : -condition2; // abs

    if((condition1 >= 0) && (condition2 <= l / Distance)) {
        // return a pair <dist, condition1>. Then take the smallest dist. In case of equality the biggest condition1
        return my_pair(distp12.Norm(), ped2->GetUID());
    }
    return my_pair(FLT_MAX, ped2->GetUID());
}
Point VelocityModel::ForceRepPed(Pedestrian * ped1, Pedestrian * ped2) const
{
    Point F_rep(0.0, 0.0);
    // x- and y-coordinate of the distance between p1 and p2
    Point distp12   = ped2->GetPos() - ped1->GetPos();
    double Distance = distp12.Norm();
    Point ep12; // x- and y-coordinate of the normalized vector between p1 and p2
    double R_ij;
    double l = 2 * ped1->GetEllipse().GetBmax();

    if(Distance >= J_EPS) {
        ep12 = distp12.Normalized();
    } else {
        LOG_ERROR(
            "VelocityModel::forcePedPed() ep12 can not be calculated! Pedestrians are too near "
            "to "
            "each other (dist={:f}). Adjust <a> value in force_ped to counter this. Affected "
            "pedestrians ped1 {} at ({:f},{:f}) and ped2 {} at ({:f}, {:f})",
            Distance,
            ped1->GetUID(),
            ped1->GetPos()._x,
            ped1->GetPos()._y,
            ped2->GetUID(),
            ped2->GetPos()._x,
            ped2->GetPos()._y);
        exit(EXIT_FAILURE); //TODO: quick and dirty fix for issue #158
                            // (sometimes sources create peds on the same location)
    }
    Point ei = ped1->GetV().Normalized();
    if(ped1->GetV().NormSquare() < 0.01) {
        ei = ped1->GetV0().Normalized();
    }
    double condition1 = ei.ScalarProduct(ep12);            // < e_i , e_ij > should be positive
    condition1        = (condition1 > 0) ? condition1 : 0; // abs

    R_ij  = -_aPed * exp((l - Distance) / _DPed);
    F_rep = ep12 * R_ij;

    return F_rep;
} //END Velocity:ForceRepPed()

Point VelocityModel::ForceRepRoom(Pedestrian * ped, SubRoom * subroom) const
{
    Point f(0., 0.);
    const Point & centroid = subroom->GetCentroid();
    bool inside            = subroom->IsInSubRoom(centroid);
    //first the walls
    for(const auto & wall : subroom->GetAllWalls()) {
        f += ForceRepWall(ped, wall, centroid, inside);
    }

    //then the obstacles
    for(const auto & obst : subroom->GetAllObstacles()) {
        if(obst->Contains(ped->GetPos())) {
            LOG_ERROR(
                "Agent {} is trapped in obstacle in room/subroom {:d}/{:d}",
                ped->GetUID(),
                subroom->GetRoomID(),
                subroom->GetSubRoomID());
            exit(EXIT_FAILURE);
        } else
            for(const auto & wall : obst->GetAllWalls()) {
                f += ForceRepWall(ped, wall, centroid, inside);
            }
    }

    // and finally the closed doors
    for(const auto & trans : subroom->GetAllTransitions()) {
        if(!trans->IsOpen()) {
            f += ForceRepWall(ped, *(static_cast<Line *>(trans)), centroid, inside);
        }
    }

    return f;
}

Point VelocityModel::ForceRepWall(
    Pedestrian * ped,
    const Line & w,
    const Point & centroid,
    bool inside) const
{
    Point F_wrep = Point(0.0, 0.0);
    Point pt     = w.ShortestPoint(ped->GetPos());

    Point dist       = pt - ped->GetPos(); // x- and y-coordinate of the distance between ped and p
    const double EPS = 0.000;              // molified see Koester2013
    double Distance  = dist.Norm() + EPS;  // distance between the centre of ped and point p
    Point e_iw; // x- and y-coordinate of the normalized vector between ped and pt
    double l = ped->GetEllipse().GetBmax();
    double R_iw;
    double min_distance_to_wall = 0.001; // 10 cm

    if(Distance > min_distance_to_wall) {
        e_iw = dist / Distance;
    } else {
        LOG_WARNING(
            "Velocity: forceRepWall() ped {} [{:f}, {:f}] is too near to the wall [{:f}, "
            "{:f}]-[{:f}, {:f}] (dist={:f})",
            ped->GetUID(),
            ped->GetPos()._y,
            ped->GetPos()._y,
            w.GetPoint1()._x,
            w.GetPoint1()._y,
            w.GetPoint2()._x,
            w.GetPoint2()._y,
            Distance);
        Point new_dist = centroid - ped->GetPos();
        new_dist       = new_dist / new_dist.Norm();
        e_iw           = (inside ? new_dist : new_dist * -1);
    }
    //-------------------------

    const Point & pos   = ped->GetPos();
    const auto distGoal = ped->GetExitLine().DistToSquare(pos);

    if(distGoal < J_EPS_GOAL * J_EPS_GOAL)
        return F_wrep;
    //-------------------------
    R_iw   = -_aWall * exp((l - Distance) / _DWall);
    F_wrep = e_iw * R_iw;

    return F_wrep;
}

std::string VelocityModel::GetDescription() const
{
    std::string rueck;
    char tmp[1024];

    sprintf(tmp, "\t\ta: \t\tPed: %f \tWall: %f\n", _aPed, _aWall);
    rueck.append(tmp);
    sprintf(tmp, "\t\tD: \t\tPed: %f \tWall: %f\n", _DPed, _DWall);
    rueck.append(tmp);
    return rueck;
}
