#! /usr/bin/env python3

# Copyright © 2012-2023 Forschungszentrum Jülich GmbH
# SPDX-License-Identifier: LGPL-3.0-or-later
import logging
import pathlib
import sys

from shapely import GeometryCollection, Polygon, to_wkt

import jupedsim as jps


def log_debug(msg):
    logging.debug(msg)


def log_info(msg):
    logging.info(msg)


def log_warn(msg):
    logging.warning(msg)


def log_error(msg):
    logging.error(msg)


def main():
    logging.basicConfig(
        level=logging.DEBUG, format="%(levelname)s : %(message)s"
    )
    jps.set_debug_callback(log_debug)
    jps.set_info_callback(log_info)
    jps.set_warning_callback(log_warn)
    jps.set_error_callback(log_error)

    p1 = Polygon([(0, 0), (10, 0), (10, 10), (0, 10)])
    p2 = Polygon([(10, 4), (20, 4), (20, 6), (10, 6)])
    area = GeometryCollection(p1.union(p2))
    geometry = jps.build_jps_geometry(area)

    model_builder = jps.VelocityModelBuilder(
        a_ped=8, d_ped=0.1, a_wall=5, d_wall=0.02
    )
    profile_id = 3
    model_builder.add_parameter_profile(
        id=profile_id, time_gap=1, tau=0.5, v0=1.2, radius=0.3
    )

    model = model_builder.build()

    simulation = jps.Simulation(model=model, geometry=geometry, dt=0.01)

    stage_id = simulation.add_waiting_set_stage([(16, 5), (15, 5), (14, 5)])
    waiting_stage = simulation.get_stage_proxy(stage_id)
    assert isinstance(waiting_stage, jps.WaitingSetProxy)
    exit_id = simulation.add_exit_stage([(18, 4), (20, 4), (20, 6), (18, 6)])

    journey = jps.JourneyDescription()
    journey.append(stage_id)
    journey.append(exit_id)

    journey_id = simulation.add_journey(journey)

    agent_parameters = jps.VelocityModelAgentParameters()
    agent_parameters.journey_id = journey_id
    agent_parameters.orientation = (1.0, 0.0)
    agent_parameters.position = (0.0, 0.0)
    agent_parameters.profile_id = profile_id

    for new_pos in [(7, 7), (1, 3), (1, 5), (1, 7), (2, 7)]:
        agent_parameters.position = new_pos
        simulation.add_agent(agent_parameters)

    print("Running simulation")

    writer = jps.SqliteTrajectoryWriter(pathlib.Path("example1_out.sqlite"))
    writer.begin_writing(10, to_wkt(area, rounding_precision=-1))

    while simulation.agent_count() > 0:
        try:
            simulation.iterate()
            if simulation.iteration_count() % 4 == 0:
                writer.write_iteration_state(simulation)
                for a in simulation.agents():
                    print(f"{a.model.e0}")
                    break
            if simulation.iteration_count() == 1300:
                if waiting_stage.state == jps.WaitingSetState.ACTIVE:
                    waiting_stage.state = jps.WaitingSetState.INACTIVE
        except KeyboardInterrupt:
            writer.end_writing()
            print("CTRL-C Recieved! Shuting down")
            sys.exit(1)
    writer.end_writing()
    print(
        f"Simulation completed after {simulation.iteration_count()} iterations"
    )


if __name__ == "__main__":
    main()
