/**
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
 **/
//
// Created by laemmel on 23.03.16.
//
#pragma once

#include "Macros.h"
#include "direction/DirectionManager.h"
#include "general/Filesystem.h"
#include "math/OperationalModel.h"
#include "pedestrian/AgentsParameters.h"
#include "routing/RoutingEngine.h"

#include <cstdlib>
#include <filesystem>
#include <memory>
#include <set>
#include <string>


// This class provides a data container for all configuration parameters.
struct Configuration {
    std::shared_ptr<RoutingEngine> routingEngine{std::make_shared<RoutingEngine>()};
    unsigned int seed{0};
    double fps{8};
    unsigned int precision{2};
    double linkedCellSize{2.2};
    std::shared_ptr<OperationalModel> model{};
    double tMax{500};
    double dT{0.01};
    double nuPed{0.4};
    double nuWall{0.2};
    double aPed{1.0};
    double aWall{1.0};
    double dWall{0.1};
    double dPed{0.1};
    double intPWidthPed{0.1};
    double intPWidthWall{0.1};
    double maxFPed{3.0};
    double maxFWall{3.0};
    double distEffMaxPed{2};
    double distEffMaxWall{2};
    double deltaH{0.0625};
    double wallAvoidDistance{0.4};
    bool useWallAvoidance{true};
    bool hasDirectionalEscalators{false};
    int exitStrat{9};
    std::shared_ptr<DirectionManager> directionManager{};
    fs::path trajectoriesFile{"trajectories.txt"};
    fs::path originalTrajectoriesFile{"trajectories.txt"};
    fs::path logFile{"log.txt"};
    fs::path iniFile{};
    fs::path geometryFile{};
    fs::path transitionFile{};
    fs::path goalFile{};
    fs::path sourceFile{};
    fs::path trafficContraintFile{};
    fs::path eventFile{};
    fs::path scheduleFile{};
    fs::path trainTypeFile{};
    fs::path trainTimeTableFile{};
    fs::path projectRootDir{"."};
    fs::path outputPath{fs::absolute("results")};
    bool showStatistics{false};
    std::map<int, std::shared_ptr<AgentsParameters>> agentsParameters{};
    std::set<OptionalOutput> optionalOutput{};
    AgentColorMode agentColorMode{AgentColorMode::BY_VELOCITY};
};
