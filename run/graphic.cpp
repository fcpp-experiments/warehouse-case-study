// Copyright © 2021 Giorgio Audrito. All Rights Reserved.

/**
 * @file graphic.cpp
 * @brief Runs a single execution of the warehouse case study with a graphical user interface.
 */

#include "lib/warehouse_simulation.hpp"

using namespace fcpp;

//! @brief The main function.
int main() {
    //! @brief Construct the plotter object.
    option::plot_t p;
    std::cout << "/*\n";
    {
        //! @brief The network object type (interactive simulator with given options).
        using net_t = component::interactive_simulator<option::list>::net;
        //! @brief The initialisation values (simulation name, texture of the reference plane, node movement speed).
        auto init_v = common::make_tagged_tuple<option::name, option::texture, option::obstacles, option::plotter>(
            "Warehouse Case Study",
            "warehouse.png",
            "warehouse-obstacles.png",
            &p
        );
        //! @brief Construct the network object.
        net_t network{init_v};
        //! @brief Run the simulation until exit.
        network.run();
    }
    //! @brief Builds the resulting plots.
    std::cout << "*/\n" << plot::file("graphic", p.build());
    return 0;
}
