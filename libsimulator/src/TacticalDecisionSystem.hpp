#pragma once

#include "Agent.hpp"
#include "Area.hpp"
#include "RoutingEngine.hpp"

#include <vector>

class TacticalDecisionSystem
{
public:
    TacticalDecisionSystem() = default;
    ~TacticalDecisionSystem() = default;
    TacticalDecisionSystem(const TacticalDecisionSystem& other) = delete;
    TacticalDecisionSystem& operator=(const TacticalDecisionSystem& other) = delete;
    TacticalDecisionSystem(TacticalDecisionSystem&& other) = delete;
    TacticalDecisionSystem& operator=(TacticalDecisionSystem&& other) = delete;

    void
    Run(const std::map<Area::Id, Area> areas,
        RoutingEngine& routingEngine,
        std::vector<std::unique_ptr<Agent>>& agents) const;
};
