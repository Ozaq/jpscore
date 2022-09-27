/// Copyright © 2012-2022 Forschungszentrum Jülich GmbH
/// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "Ellipse.hpp"
#include "Journey.hpp"
#include "Line.hpp"
#include "Macros.hpp"
#include "OperationalModel.hpp"
#include "UniqueID.hpp"

#include <memory>

class Agent
{
public:
    using ID = jps::UniqueID<Agent>;
    ID id{};

    // This is evaluated by the "strategic level"
    std::unique_ptr<Behaviour> behaviour{};

    // This is evaluated by the "operational level"
    Point destination{};
    Point waypoint{};
    OperationalModel::ParametersID parameterProfileId;

    // Agent fields common for all models
    Point pos;
    Point orientation;

    // Guaranteed to be a unit vector
    double speed;

private:
    Point _e0 = Point(0, 0); // desired direction
    int _newOrientationDelay = 0;

public:
    void SetE0(const Point& p) { _e0 = p; }
    void SetSmoothTurning();
    void IncrementOrientationDelay();
    const Point& GetE0() const;
    Point GetE0(const Point& target, double deltaT) const;
};

std::ostream& operator<<(std::ostream& out, const Agent& pedestrian);
