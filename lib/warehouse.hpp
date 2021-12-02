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

enum class warehouse_device_type { Pallet, Wearable };

#define NO_GOODS -1

// [SYSTEM SETUP]

//! @brief The final simulation time.
constexpr size_t end_time = 300;
//! @brief Number of pallet devices.
constexpr size_t pallet_node_num = 1000;
//! @brief Number of wearable devices.
constexpr size_t wearable_node_num = 5;
//! @brief Communication radius.
constexpr size_t comm = 100;
//! @brief Dimensionality of the space.
constexpr size_t dim = 3;
//! @brief Side of the area.
constexpr size_t side = 500;
//! @brief Height of the area.
constexpr size_t height = 100;


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
    struct loaded_good {};
    struct loading_goods{};
    struct node_type {};
    //! @brief Color of the current node.
    struct node_color {};
    //! @brief Size of the current node.
    struct node_size {};
    //! @brief Shape of the current node.
    struct node_shape {};
}

// [AGGREGATE PROGRAM]

FUN device_t nearest_device(ARGS) { CODE
    return get<1>(min_hood(CALL, make_tuple(node.nbr_dist(), node.nbr_uid())));
}

FUN void load_goods_on_pallet(ARGS) { CODE 
    using state_type = tuple<device_t,char>;
    nbr(CALL, state_type(node.uid, node.storage(tags::loaded_good{})), [&](field<state_type> fs) {
        if (node.storage(tags::node_type{}) == warehouse_device_type::Pallet) {
            common::option<char> new_value = fold_hood(CALL, [&](state_type const& s, common::option<char> acc) {
                if (get<0>(s) == node.uid) {
                    return common::option<char>(get<1>(s));
                } else {
                    return acc;
                }
            }, fs, common::option<char>());
            if (!new_value.empty()) {
                node.storage(tags::loaded_good{}) = new_value.front();
            }
            return state_type(node.uid, node.storage(tags::loaded_good{}));
        } else {
            char goods_currenting_loading = node.storage(tags::loading_goods{});
            if (goods_currenting_loading == NO_GOODS) {
                return state_type(node.uid, NO_GOODS);
            } else {
                state_type last_state = self(CALL, fs);
                if (get<0>(last_state) == node.uid) {
                    return state_type(nearest_device(CALL), goods_currenting_loading);
                } else {
                    if (any_hood(CALL, map_hood([&](state_type const& x) {
                        return get<0>(x) == get<0>(last_state) && get<1>(x) == get<1>(last_state);
                    }, fs))) {
                        node.storage(tags::loading_goods{}) = NO_GOODS;
                        return state_type(node.uid, NO_GOODS);
                    } else {
                        return last_state;
                    }
                }
            }
        }
    });
}

FUN_EXPORT load_goods_on_pallet_t = common::export_list<tuple<device_t,char>>;

//! @brief Function doing stuff.
FUN void stuff(ARGS) { CODE
}
//! @brief Export types used by the stuff function (none).
FUN_EXPORT stuff_t = common::export_list<>;

//! @brief Main function.
MAIN() {
    using namespace tags;
    load_goods_on_pallet(CALL);
    char current_loaded_good = node.storage(loaded_good{});
    if (current_loaded_good == -1) {
        node.storage(node_color{}) = color(GREEN);
    } else if (current_loaded_good == 0) {
        node.storage(node_color{}) = color(GOLD);
    } else if (current_loaded_good == 1) {
        node.storage(node_color{}) = color(CYAN);
    } else {
        node.storage(node_color{}) = color(RED);
    }
    if (node.storage(node_type{}) == warehouse_device_type::Pallet) {
        node.storage(node_shape{}) = shape::cube;
    } else {
        node.storage(node_shape{}) = shape::star;
    }
    node.storage(node_size{}) = 5;
    if (node.storage(node_type{}) == warehouse_device_type::Wearable) {
        rectangle_walk(CALL, make_vec(0,0,0), make_vec(side,side,height), comm/3, 1);
    }
}
//! @brief Export types used by the main function.
FUN_EXPORT main_t = common::export_list<load_goods_on_pallet_t, vec<dim>>;


} // namespace coordination


//! @brief Namespace for component options.
namespace option {

//! @brief Import tags to be used for component options.
using namespace component::tags;
//! @brief Import tags used by aggregate functions.
using namespace coordination::tags;

//! @brief The randomised sequence of rounds for every node (about one every second, with 10% variance).
using round_s = sequence::periodic<
    distribution::interval_n<times_t, 0, 1>,       // uniform time in the [0,1] interval for start
    distribution::weibull_n<times_t, 10, 1, 10>,   // weibull-distributed time for interval (10/10=1 mean, 1/10=0.1 deviation)
    distribution::constant_n<times_t, end_time+2>  // the constant end_time+2 number for end
>;
//! @brief The sequence of network snapshots (one every simulated second).
using log_s = sequence::periodic_n<1, 0, 1, end_time>;
//! @brief The sequence of node generation events (multiple devices all generated at time 0).
using pallet_spawn_s = sequence::multiple_n<pallet_node_num, 0>;

using wearable_spawn_s = sequence::multiple_n<wearable_node_num, 0>;
//! @brief The distribution of initial node positions (random in a given rectangle).
using rectangle_d = distribution::rect_n<1, 0, 0, 0, side, side, height>;
//! @brief The contents of the node storage as tags and associated types.
using store_t = tuple_store<
    loaded_good,        char,
    loading_goods,      char,
    node_type,          warehouse_device_type,
    node_color,         color,
    node_shape,         shape,
    node_size,          double
>;
//! @brief The tags and corresponding aggregators to be logged.
using aggregator_t = aggregators<
>;
//! @brief The description of plots.
using plot_t = plot::none;

CONSTANT_DISTRIBUTION(pallet_distribution, warehouse_device_type, warehouse_device_type::Pallet);
CONSTANT_DISTRIBUTION(wearable_distribution, warehouse_device_type, warehouse_device_type::Wearable);

//! @brief The general simulation options.
DECLARE_OPTIONS(list,
    parallel<false>,     // no multithreading on node rounds
    synchronised<false>, // optimise for asynchronous networks
    program<coordination::main>,   // program to be run (refers to MAIN above)
    exports<coordination::main_t>, // export type list (types used in messages)
    round_schedule<round_s>, // the sequence generator for round events on nodes
    log_schedule<log_s>,     // the sequence generator for log events on the network
    store_t,       // the contents of the node storage
    aggregator_t,  // the tags and corresponding aggregators to be logged
    spawn_schedule<pallet_spawn_s>, // the sequence generator of node creation events on the network
    init<
        x,      rectangle_d,
        node_type, pallet_distribution,
        loaded_good, distribution::constant_n<char, -1>,
        loading_goods, distribution::constant_n<char, -1>
    >,
    spawn_schedule<wearable_spawn_s>,
    init<
        x,      rectangle_d,
        node_type, wearable_distribution,
        loaded_good, distribution::constant_n<char, -1>,
        loading_goods, distribution::constant_n<char, -1>
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
