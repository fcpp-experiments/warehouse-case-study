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

std::string to_string(warehouse_device_type t) {
    switch (t) {
        case warehouse_device_type::Pallet:
            return "Pallet";
            break;
        case warehouse_device_type::Wearable:
            return "Wearable";
    }
}

#define NO_GOODS 255
#define UNLOAD_GOODS 254
#define LOG_TYPE_PALLET_CONTENT_CHANGE 1
#define LOG_TYPE_HANDLE_PALLET 2
#define LOG_TYPE_COLLISION_RISK 3


// [SYSTEM SETUP]

//! @brief The final simulation time.
constexpr size_t end_time = 300;
//! @brief Number of pallet devices.
constexpr size_t pallet_node_num = 250;
//! @brief Number of wearable devices.
constexpr size_t wearable_node_num = 6;
//! @brief Communication radius.
constexpr size_t comm = 240;
//! @brief Dimensionality of the space.
constexpr size_t dim = 3;
constexpr size_t grid_cell_size = 20;
//! @brief Side of the area.
constexpr size_t side = 1140;
constexpr size_t side_2 = 1260;
//! @brief Height of the area.
constexpr size_t height = 200;


/**
 * @brief Namespace containing all the objects in the FCPP library.
 */
namespace fcpp {

//! @brief Tags used in the node storage.
namespace coordination {
    namespace tags {
        struct goods_type {};
        struct log_content_type {};
        struct logger_id {};
        struct log_time {};
        struct log_content {};
        struct loaded_good {};
        struct loading_goods{};
        struct query_goods {};
        struct logs {};
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

using log_type = common::tagged_tuple_t<coordination::tags::log_content_type, uint8_t, coordination::tags::logger_id, device_t, coordination::tags::log_time, times_t, coordination::tags::log_content, real_t>;

using query_type = common::tagged_tuple_t<coordination::tags::query_goods, uint8_t>;

//! @brief Namespace containing the libraries of coordination routines.
namespace coordination {

// [AGGREGATE PROGRAM]

FUN device_t nearest_pallet_device(ARGS) { CODE
    auto tuple_field = make_tuple(node.nbr_dist(), node.nbr_uid(), nbr(CALL, node.storage(tags::node_type{})));
    auto pallet_tuple_field = map_hood([](tuple<real_t, device_t, warehouse_device_type> const& t) {
        tuple<real_t, device_t, warehouse_device_type> o(t);
        if (get<2>(o) == warehouse_device_type::Wearable) {
            get<0>(o) = INF;
        }
        return o;
    }, tuple_field);
    return get<1>(min_hood(CALL, pallet_tuple_field, make_tuple(INF, node.uid, node.storage(tags::node_type{}))));
}

FUN_EXPORT nearest_pallet_device_t = common::export_list<warehouse_device_type>;

FUN std::vector<log_type> load_goods_on_pallet(ARGS) { CODE 
    using state_type = tuple<device_t,pallet_content_type>;
    std::vector<log_type> loading_logs;
    nbr(CALL, state_type(node.uid, node.storage(tags::loaded_good{})), [&](field<state_type> fs) {
        state_type last_state = self(CALL, fs);
        pallet_content_type pallet_value = fold_hood(CALL, [&](state_type const& s, pallet_content_type acc) {
            if (get<0>(s) == node.uid) {
                return get<1>(s);
            } else {
                return acc;
            }
        }, fs, get<1>(last_state));
        if (node.storage(tags::node_type{}) == warehouse_device_type::Pallet && get<1>(last_state) != pallet_value) {
            loading_logs.push_back(common::make_tagged_tuple<tags::log_content_type, tags::logger_id, tags::log_time, tags::log_content>(LOG_TYPE_PALLET_CONTENT_CHANGE, node.uid, node.net.real_time(), get<tags::goods_type>(pallet_value)));
        }
        pallet_content_type goods_currenting_loading = node.storage(tags::loading_goods{});
        device_t wearable_device_id = get<0>(last_state);
        pallet_content_type wearable_value = get<1>(last_state);
        device_t nearest = nearest_pallet_device(CALL);
        if (get<0>(last_state) == node.uid && get<tags::goods_type>(goods_currenting_loading) != NO_GOODS) {
            wearable_device_id = nearest;
            if (get<tags::goods_type>(goods_currenting_loading) == UNLOAD_GOODS) {
                wearable_value = NO_GOODS;
            } else {
                wearable_value = goods_currenting_loading;
            }
            loading_logs.push_back(common::make_tagged_tuple<tags::log_content_type, tags::logger_id, tags::log_time, tags::log_content>(LOG_TYPE_HANDLE_PALLET, node.uid, node.net.real_time(), nearest));
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
    return loading_logs;
}

FUN_EXPORT load_goods_on_pallet_t = common::export_list<tuple<device_t,pallet_content_type>>;

FUN void maybe_change_loading_goods_for_simulation(ARGS) { CODE
    if (any_hood(CALL, nbr(CALL, node.storage(tags::node_type{}) == warehouse_device_type::Pallet)) &&
        node.storage(tags::node_type{}) == warehouse_device_type::Wearable                          &&
        get<tags::goods_type>(node.storage(tags::loading_goods{})) == NO_GOODS                      &&
        (rand() % 100) < 5) {
        int new_type = rand() % 3;
        if (new_type == 2) {
            new_type = UNLOAD_GOODS;
        }
        node.storage(tags::loading_goods{}) = common::make_tagged_tuple<coordination::tags::goods_type>(new_type);
    }
}

FUN_EXPORT maybe_change_loading_goods_for_simulation_t = common::export_list<bool>;

//! @brief Collects distributed data with a Lossless-information-speed-threshold strategy (blist) and a idempotent accumulate function [TO CHECK].
GEN(T, G, BOUND( G, T(T,T) ))
T blist_idem_collection(ARGS, real_t const& distance, T const& value, real_t radius, real_t speed, T const& null, G&& accumulate) { CODE
    field<real_t> nbrdist = nbr(CALL, distance);
    real_t t = node.current_time();
    field<real_t> Tu = nbr(CALL, node.next_time());
    field<real_t> Pu = nbr(CALL,distance + speed * (Tu - t));
    field<real_t> dist = node.nbr_dist();
    field<real_t> maxDistanceNow = dist + speed * node.nbr_lag();
    field<real_t> Vwst = mux(isfinite(distance) && distance > nbrdist, (distance - Pu) / (Tu - t) , field<real_t>{0} ) ;
    field<real_t> threshold = mux( (isfinite(distance) && maxDistanceNow < radius), max_hood(CALL, Vwst) , -INF);
    return nbr(CALL, value, [&](field<T> old){
        return fold_hood(CALL, accumulate, mux(nbrdist >= distance + node.nbr_lag() * nbr(CALL,threshold) && nbrdist > distance, old, null), value);
    });
}
template <typename T> using blist_idem_collection_t = common::export_list<T,field<real_t>,real_t >;

FUN std::vector<log_type> collision_detection(ARGS, real_t radius, real_t threshold) { CODE
    bool wearable = node.storage(tags::node_type{}) == warehouse_device_type::Wearable;
    std::unordered_map<device_t, real_t> logmap = spawn(CALL, [&](device_t source){
        real_t dist = bis_distance(CALL, node.uid == source, 1, 0.5*comm);
        real_t closest_wearable = mp_collection(CALL, dist, wearable and node.uid != source ? dist : INF, INF, [](real_t x, real_t y){
            return min(x,y);
        }, [](real_t x, size_t) {
            return x;
        });
        real_t v = 0;
        if (isfinite(closest_wearable))
            v = (old(CALL, closest_wearable) - closest_wearable) / (node.current_time() - node.previous_time());
        return make_tuple(v, dist < radius);
    }, wearable ? common::option<device_t>{node.uid} : common::option<device_t>{});
    std::vector<log_type> logvec;
    if (logmap[node.uid] > threshold)
        logvec.emplace_back(LOG_TYPE_COLLISION_RISK, node.uid, node.current_time(), logmap[node.uid]);
    return logvec;
}
FUN_EXPORT collision_detection_t = common::export_list<spawn_t<device_t, bool>, bis_distance_t, mp_collection_t<real_t, real_t>, real_t>;

FUN device_t find_goods(ARGS, query_type query) { CODE
    // TODO
    return node.uid;
}
FUN_EXPORT find_goods_t = common::export_list<query_type>;

FUN std::vector<log_type> single_log_collection(ARGS, std::vector<log_type> const& new_logs, int parity) { CODE
    bool source = node.uid % 2 == parity and node.storage(tags::node_type{}) == warehouse_device_type::Wearable;
    real_t dist = bis_distance(CALL, source, 1, 0.5*comm);
    std::vector<log_type> r = mp_collection(CALL, dist, new_logs, std::vector<log_type>{}, [](std::vector<log_type> const& x, std::vector<log_type> const& y){
        std::vector<log_type> z;
        size_t i = 0, j = 0;
        while (i < x.size() and j < y.size()) {
            if (x[i] <= y[j]) {
                if (x[i] == y[j]) ++j;
                z.push_back(x[i++]);
            } else z.push_back(y[j++]);
        }
        while (i < x.size()) z.push_back(x[i++]);
        while (j < y.size()) z.push_back(y[j++]);
        return z;
    }, [](std::vector<log_type> x, size_t){
        return x;
    });
    return source ? r : std::vector<log_type>{};
}
FUN_EXPORT single_log_collection_t = common::export_list<mp_collection_t<real_t, std::vector<log_type>>, bis_distance_t>;

FUN std::vector<log_type> log_collection(ARGS, std::vector<log_type> new_logs) { CODE
    std::sort(new_logs.begin(), new_logs.end());
    std::vector<log_type> r0 = single_log_collection(CALL, new_logs, 0);
    std::vector<log_type> r1 = single_log_collection(CALL, new_logs, 1);
    return r0.empty() ? r1 : r0;
}
FUN_EXPORT log_collection_t = common::export_list<single_log_collection_t>;

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
    } else if (current_loaded_good == UNLOAD_GOODS) {
        node.storage(node_color{}) = color(DARK_GREEN);
    } else {
        node.storage(node_color{}) = color(RED);
    }
    if (node.storage(led_on{})) {
        node.storage(node_size{}) = 30;
    } else {
        node.storage(node_size{}) = 15;
    }
    if (node.storage(node_type{}) == warehouse_device_type::Wearable && current_loaded_good == NO_GOODS) {
        auto closest_obstacle = node.net.closest_obstacle(node.position());
        real_t dist_to_obstacle = distance(closest_obstacle, node.position());
        if (dist_to_obstacle <= 10) {
            node.velocity() = make_vec(0,0,0);
            node.propulsion() = make_vec(0,0,0);
            if (dist_to_obstacle > 0) {
                node.propulsion() += -coordination::point_elastic_force(CALL,closest_obstacle,1,5);
            } else {
                node.propulsion() += coordination::point_elastic_force(CALL,node.net.closest_space(node.position()),1,5);
            }
        } else {
            node.propulsion() = make_vec(0,0,0);
            rectangle_walk(CALL, make_vec(grid_cell_size,grid_cell_size,0), make_vec(side - grid_cell_size,side_2 - grid_cell_size,0), comm/10, 1);
        }
    } else {
        node.propulsion() = make_vec(0,0,0);
        rectangle_walk(CALL, make_vec(0,0,0), make_vec(0,0,0), 0, 1);
    }
}

FUN_EXPORT update_node_in_simulation_t = common::export_list<vec<dim>, int>;

std::set<tuple<int,int,int>> used_slots;

FUN void setup_nodes_if_first_round_of_simulation(ARGS) { CODE
    if (coordination::counter(CALL) == 1 && node.storage(tags::node_type{}) == warehouse_device_type::Pallet) {
        int row, col, height;
        real_t x, y, z;
        do {
            row = node.next_int(0,21);
            col = node.next_int(0,44);
            height = node.next_int(0,2);
            if (row == 0) {
                row = 22;
            }
            x = ((((row / 2) * 3) + row) * grid_cell_size) + (grid_cell_size / 2);
            y = ((((col / 15) * 3) + col + 9) * grid_cell_size) + (grid_cell_size / 2);
            z = height * grid_cell_size * 1.5;
        } while (used_slots.find(make_tuple(row,col,height)) != used_slots.end());
        used_slots.insert(make_tuple(row,col,height));
        node.position() = make_vec(x,y,z);
    }
}

FUN_EXPORT setup_nodes_if_first_round_of_simulation_t = common::export_list<uint32_t>;

//! @brief Main function.
MAIN() {
    std::vector<log_type> new_logs;
    setup_nodes_if_first_round_of_simulation(CALL);
    maybe_change_loading_goods_for_simulation(CALL);
    std::vector<log_type> loading_logs = load_goods_on_pallet(CALL);
    new_logs.insert(new_logs.end(), loading_logs.begin(), loading_logs.end());
    std::vector<log_type> collision_logs = collision_detection(CALL, 0.1, 0.1);
    new_logs.insert(new_logs.end(), collision_logs.begin(), collision_logs.end());
    find_goods(CALL, common::make_tagged_tuple<tags::query_goods>(0));
    std::vector<log_type> collected_logs = log_collection(CALL, new_logs);
    if (node.storage(tags::node_type{}) == warehouse_device_type::Wearable) {
        std::vector<log_type>& previously_collected_logs = node.storage(tags::logs{});
        previously_collected_logs.insert(previously_collected_logs.end(), collected_logs.begin(), collected_logs.end());
    }
    update_node_in_simulation(CALL);
}
//! @brief Export types used by the main function.
FUN_EXPORT main_t = common::export_list<
    nearest_pallet_device_t,
    load_goods_on_pallet_t, 
    collision_detection_t, 
    find_goods_t, 
    log_collection_t, 
    update_node_in_simulation_t, 
    setup_nodes_if_first_round_of_simulation_t,
    maybe_change_loading_goods_for_simulation_t
>;


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
using rectangle_d = distribution::rect_n<1, 0, 0, 0, side, side_2, 0>;
//! @brief The distribution of initial node positions (random in a given rectangle).
using wearable_rectangle_d = distribution::rect_n<1, grid_cell_size, grid_cell_size, 0, grid_cell_size * 35, grid_cell_size * 9, 0>;
//! @brief The contents of the node storage as tags and associated types.
using store_t = tuple_store<
    loaded_good,        pallet_content_type,
    loading_goods,      pallet_content_type,
    logs,               std::vector<log_type>,
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
        x,      wearable_rectangle_d,
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
    color_tag<node_color>,  // colors of a node are read from these
    area<0,0,side,side_2>
);


} // namespace option


} // namespace fcpp


#endif // FCPP_WAREHOUSE_H_
