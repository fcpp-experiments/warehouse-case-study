// Copyright © 2021 Giorgio Audrito. All Rights Reserved.

/**
 * @file warehouse.hpp
 * @brief Case study on smart warehouse management.
 *
 * This header file is designed to work under multiple execution paradigms.
 */

#ifndef FCPP_WAREHOUSE_H_
#define FCPP_WAREHOUSE_H_

#include "lib/fcpp.hpp"


/**
 * @brief Namespace containing all the objects in the FCPP library.
 */
namespace fcpp {


// [INTRODUCTION]

//! @brief Dummy ordering between positions (allows positions to be used as secondary keys in ordered tuples).
template <size_t n>
bool operator<(vec<n> const&, vec<n> const&) {
    return false;
}

//! @brief Namespace containing the libraries of coordination routines.
namespace coordination {


//! @brief Tags used in the node storage.
namespace tags {
    //! @brief Color of the current node.
    struct node_color {};
    //! @brief Size of the current node.
    struct node_size {};
    //! @brief Shape of the current node.
    struct node_shape {};
}


// [AGGREGATE PROGRAM]

//! @brief Function doing stuff.
FUN void stuff(ARGS) { CODE
}
//! @brief Export types used by the stuff function (none).
FUN_EXPORT stuff_t = common::export_list<>;


//! @brief Main function.
MAIN() {
}
//! @brief Export types used by the main function.
FUN_EXPORT main_t = common::export_list<>;


} // namespace coordination


//! @brief Namespace for component options.
namespace option {

//! @brief Import tags to be used for component options.
using namespace component::tags;
//! @brief Import tags used by aggregate functions.
using namespace coordination::tags;


// [SYSTEM SETUP]

//! @brief The final simulation time.
constexpr size_t end_time = 300;
//! @brief Number of devices.
constexpr size_t node_num = 1000;
//! @brief Communication radius.
constexpr size_t comm = 100;
//! @brief Dimensionality of the space.
constexpr size_t dim = 3;
//! @brief Side of the area.
constexpr size_t side = 500;

//! @brief The randomised sequence of rounds for every node (about one every second, with 10% variance).
using round_s = sequence::periodic<
    distribution::interval_n<times_t, 0, 1>,       // uniform time in the [0,1] interval for start
    distribution::weibull_n<times_t, 10, 1, 10>,   // weibull-distributed time for interval (10/10=1 mean, 1/10=0.1 deviation)
    distribution::constant_n<times_t, end_time+2>  // the constant end_time+2 number for end
>;
//! @brief The sequence of network snapshots (one every simulated second).
using log_s = sequence::periodic_n<1, 0, 1, end_time>;
//! @brief The sequence of node generation events (multiple devices all generated at time 0).
using spawn_s = sequence::multiple_n<node_num, 0>;
//! @brief The distribution of initial node positions (random in a given rectangle).
using rectangle_d = distribution::rect_n<1, 0, 0, 0, side, side, 0>;
//! @brief The contents of the node storage as tags and associated types.
using store_t = tuple_store<
    node_color,         color,
    node_shape,         shape,
    node_size,          double
>;
//! @brief The tags and corresponding aggregators to be logged.
using aggregator_t = aggregators<
>;
//! @brief The description of plots.
using plot_t = plot::none;


//! @brief The general simulation options.
DECLARE_OPTIONS(list,
    parallel<false>,     // no multithreading on node rounds
    synchronised<false>, // optimise for asynchronous networks
    program<coordination::main>,   // program to be run (refers to MAIN above)
    exports<coordination::main_t>, // export type list (types used in messages)
    round_schedule<round_s>, // the sequence generator for round events on nodes
    log_schedule<log_s>,     // the sequence generator for log events on the network
    spawn_schedule<spawn_s>, // the sequence generator of node creation events on the network
    store_t,       // the contents of the node storage
    aggregator_t,  // the tags and corresponding aggregators to be logged
    init<
        x,      rectangle_d
    >,
    plot_type<plot_t>, // the plot description to be used
    dimension<dim>, // dimensionality of the space
    connector<connect::radial<80,connect::fixed<comm,1,dim>>>, // probabilistic connection within a comm range (50% loss at 80% radius)
    connector<connect::fixed<comm, 1, dim>>,
    shape_tag<node_shape>, // the shape of a node is read from this tag in the store
    size_tag<node_size>,   // the size of a node is read from this tag in the store
    color_tag<node_color>  // colors of a node are read from these
);


} // namespace option


} // namespace fcpp


#endif // FCPP_WAREHOUSE_H_
