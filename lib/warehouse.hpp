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

#define NO_GOODS 255

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
    struct node_uid {};
}

// [AGGREGATE PROGRAM]

FUN device_t nearest_device(ARGS) { CODE
    return get<1>(min_hood(CALL, make_tuple(node.nbr_dist(), node.nbr_uid()), make_tuple(INF, node.uid)));
}

FUN void load_goods_on_pallet(ARGS) { CODE 
    using state_type = tuple<device_t,uint8_t>;
    nbr(CALL, state_type(node.uid, node.storage(tags::loaded_good{})), [&](field<state_type> fs) {
        state_type last_state = self(CALL, fs);
        uint8_t pallet_value = fold_hood(CALL, [&](state_type const& s, uint8_t acc) {
            if (get<0>(s) == node.uid) {
                return get<1>(s);
            } else {
                return acc;
            }
        }, fs, get<1>(last_state));
        uint8_t goods_currenting_loading = node.storage(tags::loading_goods{});
        device_t wearable_device_id = get<0>(last_state);
        uint8_t wearable_value = get<1>(last_state);
        device_t nearest = nearest_device(CALL);
        if (get<0>(last_state) == node.uid && goods_currenting_loading != NO_GOODS) {
            wearable_device_id = nearest;
            wearable_value = goods_currenting_loading;
        }
        if (any_hood(CALL, map_hood([&](state_type const& x) {
                return get<0>(x) == get<0>(last_state) && get<1>(x) == get<1>(last_state);
            }, fs)) && get<0>(last_state) != node.uid) {
            node.storage(tags::loading_goods{}) = NO_GOODS;
            wearable_device_id = node.uid;
            wearable_value = NO_GOODS;
        }
        if (node.storage(tags::node_type{}) == warehouse_device_type::Pallet) {
            node.storage(tags::loaded_good{}) = pallet_value;
        }
        return mux(
            node.storage(tags::node_type{}) == warehouse_device_type::Pallet,
            state_type(node.uid, pallet_value),
            state_type(wearable_device_id, wearable_value)
        );
    });
}

FUN_EXPORT load_goods_on_pallet_t = common::export_list<tuple<device_t,uint8_t>>;

//! @brief Function doing stuff.
FUN void stuff(ARGS) { CODE
}
//! @brief Export types used by the stuff function (none).
FUN_EXPORT stuff_t = common::export_list<>;

FUN void maybe_change_loading_goods_for_simulation(ARGS) { CODE
    if (node.storage(tags::node_type{}) == warehouse_device_type::Wearable &&
    node.storage(tags::loading_goods{}) == NO_GOODS && (rand() % 100) < 5) {
        node.storage(tags::loading_goods{}) = rand() % 2;
    }
}

//! @brief Main function.
MAIN() {
    using namespace tags;
    maybe_change_loading_goods_for_simulation(CALL);
    load_goods_on_pallet(CALL);
    node.storage(node_uid{}) = node.uid;
    uint8_t current_loaded_good = NO_GOODS;
    if (node.storage(node_type{}) == warehouse_device_type::Pallet) {
        node.storage(node_shape{}) = shape::cube;
        current_loaded_good = node.storage(loaded_good{});
    } else {
        if (node.storage(loading_goods{}) == NO_GOODS) {
            node.storage(node_shape{}) = shape::sphere;
        } else {
            node.storage(node_shape{}) = shape::star;
        }
        current_loaded_good = node.storage(loading_goods{});
    }
    if (current_loaded_good == NO_GOODS) {
        node.storage(node_color{}) = color(GREEN);
    } else if (current_loaded_good == 0) {
        node.storage(node_color{}) = color(GOLD);
    } else if (current_loaded_good == 1) {
        node.storage(node_color{}) = color(CYAN);
    } else {
        node.storage(node_color{}) = color(RED);
    }
    node.storage(node_size{}) = 5;
    if (node.storage(node_type{}) == warehouse_device_type::Wearable && node.storage(loading_goods{}) == NO_GOODS) {
        rectangle_walk(CALL, make_vec(0,0,0), make_vec(side,side,height), comm/15, 1);
    } else {
        rectangle_walk(CALL, make_vec(0,0,0), make_vec(side,side,height), 0, 1);
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
    loaded_good,        uint8_t,
    loading_goods,      uint8_t,
    node_type,          warehouse_device_type,
    node_color,         color,
    node_shape,         shape,
    node_size,          double,
    node_uid,           device_t
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
        loaded_good, distribution::constant_n<uint8_t, NO_GOODS>,
        loading_goods, distribution::constant_n<uint8_t, NO_GOODS>
    >,
    spawn_schedule<wearable_spawn_s>,
    init<
        x,      rectangle_d,
        node_type, wearable_distribution,
        loaded_good, distribution::constant_n<uint8_t, NO_GOODS>,
        loading_goods, distribution::constant_n<uint8_t, NO_GOODS>
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
