// Copyright © 2021 Giorgio Audrito. All Rights Reserved.

/**
 * @file batch.cpp
 * @brief Runs multiple executions of the warehouse case study non-interactively from the command line, producing overall plots.
 */

#include "lib/warehouse_simulation.hpp"

using namespace fcpp;

//! @brief The main function.
int main() {
    //! @brief Construct the plotter object.
    option::plot_t p;
    //! @brief The component type (batch simulator with given options).
    using comp_t = component::batch_simulator<option::list>;
    //! @brief The list of initialisation values to be used for simulations.
    auto init_list = batch::make_tagged_tuple_sequence(
        batch::arithmetic<option::seed>(0, 99, 1),                  // 100 different random seeds
        // generate output file name for the run
        batch::stringify<option::output>("output/batch", "txt"),
        batch::constant<option::plotter>(&p)                        // reference to the plotter object
    );
    //! @brief Runs the given simulations.
    batch::run(comp_t{}, init_list);
    //! @brief Builds the resulting plots.
    std::cout << plot::file("batch", p.build());
    return 0;
}
