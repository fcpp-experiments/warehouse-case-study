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

//! @brief Tags used in the node storage.
namespace coordination {
    namespace tags {
        struct goods_type {};
        struct log_type {};
        struct logger_id {};
        struct log_time {};
        struct log_content {};
        struct loaded_good {};
        struct loading_goods{};
        struct query_goods {};
        struct led_on {};
        struct node_type {};
        //! @brief Color of the current node.
        struct node_color {};
        //! @brief Size of the current node.
        struct node_size {};
        //! @brief Shape of the current node.
        struct node_shape {};
        struct node_uid {};
    }
}


// [INTRODUCTION]

//! @brief Dummy ordering between positions (allows positions to be used as secondary keys in ordered tuples).
template <size_t n>
bool operator<(vec<n> const&, vec<n> const&) {
    return false;
}

using pallet_content_type = common::tagged_tuple_t<coordination::tags::goods_type, uint8_t>;

using log_type = common::tagged_tuple_t<coordination::tags::log_type, uint8_t, coordination::tags::logger_id, device_t, coordination::tags::log_time, times_t, coordination::tags::log_content, real_t>;

using query_type = common::tagged_tuple_t<coordination::tags::query_goods, uint8_t>;

//! @brief Namespace containing the libraries of coordination routines.
namespace coordination {

// [AGGREGATE PROGRAM]

FUN device_t nearest_device(ARGS) { CODE
    return get<1>(min_hood(CALL, make_tuple(node.nbr_dist(), node.nbr_uid()), make_tuple(INF, node.uid)));
}

FUN void load_goods_on_pallet(ARGS) { CODE 
    using state_type = tuple<device_t,pallet_content_type>;
    nbr(CALL, state_type(node.uid, node.storage(tags::loaded_good{})), [&](field<state_type> fs) {
        state_type last_state = self(CALL, fs);
        pallet_content_type pallet_value = fold_hood(CALL, [&](state_type const& s, pallet_content_type acc) {
            if (get<0>(s) == node.uid) {
                return get<1>(s);
            } else {
                return acc;
            }
        }, fs, get<1>(last_state));
        pallet_content_type goods_currenting_loading = node.storage(tags::loading_goods{});
        device_t wearable_device_id = get<0>(last_state);
        pallet_content_type wearable_value = get<1>(last_state);
        device_t nearest = nearest_device(CALL);
        if (get<0>(last_state) == node.uid && get<tags::goods_type>(goods_currenting_loading) != NO_GOODS) {
            wearable_device_id = nearest;
            wearable_value = goods_currenting_loading;
        }
        if (any_hood(CALL, map_hood([&](state_type const& x) {
                return get<0>(x) == get<0>(last_state) && get<1>(x) == get<1>(last_state);
            }, fs)) && get<0>(last_state) != node.uid) {
            node.storage(tags::loading_goods{}) = common::make_tagged_tuple<coordination::tags::goods_type>(NO_GOODS);
            wearable_device_id = node.uid;
            wearable_value = common::make_tagged_tuple<coordination::tags::goods_type>(NO_GOODS);
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

FUN_EXPORT load_goods_on_pallet_t = common::export_list<tuple<device_t,pallet_content_type>>;

//! @brief Function doing stuff.
FUN void stuff(ARGS) { CODE
}
//! @brief Export types used by the stuff function (none).
FUN_EXPORT stuff_t = common::export_list<>;

FUN void maybe_change_loading_goods_for_simulation(ARGS) { CODE
    if (node.storage(tags::node_type{}) == warehouse_device_type::Wearable &&
    get<tags::goods_type>(node.storage(tags::loading_goods{})) == NO_GOODS && (rand() % 100) < 5) {
        node.storage(tags::loading_goods{}) = common::make_tagged_tuple<coordination::tags::goods_type>(rand() % 2);
    }
}

FUN std::vector<log_type> collision_detection(ARGS, real_t radius, real_t threshold) { CODE
    // TODO
    return std::vector<log_type>();
}

FUN_EXPORT collision_detection_t = common::export_list<>;

FUN device_t find_goods(ARGS, query_type query) { CODE
    // TODO
    return node.uid;
}

FUN_EXPORT find_goods_t = common::export_list<query_type>;

FUN std::vector<log_type> log_collection(ARGS, std::vector<log_type> new_logs) { CODE
    // TODO
    return std::vector<log_type>();
}

FUN void update_node_in_simulation(ARGS) { CODE
    using namespace tags;
    node.storage(node_uid{}) = node.uid;
    uint8_t current_loaded_good = NO_GOODS;
    if (node.storage(node_type{}) == warehouse_device_type::Pallet) {
        node.storage(node_shape{}) = shape::cube;
        current_loaded_good = get<tags::goods_type>(node.storage(loaded_good{}));
    } else {
        if (get<tags::goods_type>(node.storage(loading_goods{})) == NO_GOODS) {
            node.storage(node_shape{}) = shape::sphere;
        } else {
            node.storage(node_shape{}) = shape::star;
        }
        current_loaded_good = get<tags::goods_type>(node.storage(loading_goods{}));
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
    if (node.storage(led_on{})) {
        node.storage(node_size{}) = 10;
    } else {
        node.storage(node_size{}) = 5;
    }
    if (node.storage(node_type{}) == warehouse_device_type::Wearable && current_loaded_good == NO_GOODS) {
        rectangle_walk(CALL, make_vec(0,0,0), make_vec(side,side,height), comm/15, 1);
    } else {
        rectangle_walk(CALL, make_vec(0,0,0), make_vec(side,side,height), 0, 1);
    }
}

FUN_EXPORT update_node_in_simulation_t = common::export_list<vec<dim>>;

//! @brief Main function.
MAIN() {
    maybe_change_loading_goods_for_simulation(CALL);
    load_goods_on_pallet(CALL);
    collision_detection(CALL, 0.1, 0.1);
    find_goods(CALL, common::make_tagged_tuple<tags::query_goods>(0));
    log_collection(CALL, std::vector<log_type>());
    update_node_in_simulation(CALL);
}
//! @brief Export types used by the main function.
FUN_EXPORT main_t = common::export_list<load_goods_on_pallet_t, collision_detection_t, find_goods_t, update_node_in_simulation_t>;


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
    loaded_good,        pallet_content_type,
    loading_goods,      pallet_content_type,
    led_on,             bool,
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

CONSTANT_DISTRIBUTION(false_distribution, bool, false);
CONSTANT_DISTRIBUTION(pallet_distribution, warehouse_device_type, warehouse_device_type::Pallet);
CONSTANT_DISTRIBUTION(wearable_distribution, warehouse_device_type, warehouse_device_type::Wearable);
CONSTANT_DISTRIBUTION(no_goods_distribution, pallet_content_type, fcpp::common::make_tagged_tuple<coordination::tags::goods_type>(NO_GOODS));

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
        led_on, false_distribution,
        node_type, pallet_distribution,
        loaded_good, no_goods_distribution,
        loading_goods, no_goods_distribution
    >,
    spawn_schedule<wearable_spawn_s>,
    init<
        x,      rectangle_d,
        led_on, false_distribution,
        node_type, wearable_distribution,
        loaded_good, no_goods_distribution,
        loading_goods, no_goods_distribution
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
