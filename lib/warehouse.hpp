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

#define MSG_SIZE_HARDWARE_LIMIT 200


// [SYSTEM SETUP]

//! @brief The final simulation time.
constexpr size_t end_time = 300;
//! @brief Number of pallet devices.
constexpr size_t pallet_node_num = 500;
//! @brief Number of wearable devices.
constexpr size_t wearable_node_num = 6;
//! @brief Communication radius (25m w-w, 15m w-p, 9m p-p).
constexpr size_t comm = 2500;
//! @brief Dimensionality of the space.
constexpr size_t dim = 3;
constexpr size_t grid_cell_size = 150;
//! @brief Side of the area.
constexpr size_t side = 8550;
constexpr size_t side_2 = 9450;
//! @brief Height of the area.
constexpr size_t height = 1000;


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
        struct querying {};
        struct logs {};
        struct led_on {};
        struct node_type {};
        //! @brief Color of the current node.
        struct node_color {};
        struct side_color {};
        //! @brief Size of the current node.
        struct node_size {};
        //! @brief Shape of the current node.
        struct node_shape {};
        struct node_uid {};
        //! @brief Maximum message size ever experienced.
        struct msg_size {};
        struct msg_received__perc {};
        struct log_collected {};
        struct log_received__perc {};
        struct log_created {};
        struct logging_delay {};
    }
}


// [INTRODUCTION]

//! @brief Dummy ordering between positions (allows positions to be used as secondary keys in ordered tuples).
template <size_t n>
bool operator<(vec<n> const&, vec<n> const&) {
    return false;
}

using pallet_content_type = common::tagged_tuple_t<coordination::tags::goods_type, uint8_t>;

using log_type = common::tagged_tuple_t<coordination::tags::log_content_type, uint8_t, coordination::tags::logger_id, device_t, coordination::tags::log_time, uint8_t, coordination::tags::log_content, uint16_t>;

using query_type = common::tagged_tuple_t<coordination::tags::goods_type, uint8_t>;

uint8_t discretizer(times_t t) {
    return int(10*t) % 256;
}

}

namespace std {

//! @brief Sorted vector merging.
std::vector<fcpp::log_type> operator+(std::vector<fcpp::log_type> const& x, std::vector<fcpp::log_type> const& y) {
    if (y.size() == 0) return x;
    if (x.size() == 0) return y;
    std::vector<fcpp::log_type> z;
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
}

//! @brief Sorted vector subtraction.
std::vector<fcpp::log_type> operator-(std::vector<fcpp::log_type> x, std::vector<fcpp::log_type> const& y) {
    if (y.size() == 0) return x;
    size_t i = 0;
    for (size_t j=0, k=0; j<x.size(); ++j) {
        while (k < y.size() and y[k] < x[j]) ++k;
        if (k >= y.size() or y[k] > x[j]) x[i++] = x[j];
    }
    x.resize(i);
    return x;
}

//! @brief Tuple hasher.
template <>
struct hash<fcpp::tuple<fcpp::device_t,fcpp::query_type>> {
    size_t operator()(fcpp::tuple<fcpp::device_t,fcpp::query_type> const& k) const {
        return get<fcpp::coordination::tags::goods_type>(get<1>(k)) | (get<0>(k) << 8);
    }
};

}

namespace fcpp {

//! @brief Namespace containing the libraries of coordination routines.
namespace coordination {

// [AGGREGATE PROGRAM]

FUN uint8_t random_good(ARGS) { CODE
    constexpr real_t f = 5.187377517639621;
    real_t r = node.next_real(f);
    for (uint8_t i=1; ; ++i) {
        r -= real_t(1)/i;
        if (r < 0) return i-1;
    }
    return 99;
}

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

FUN std::vector<log_type> load_goods_on_pallet(ARGS, times_t current_clock) { CODE 
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
            loading_logs.emplace_back(LOG_TYPE_PALLET_CONTENT_CHANGE, node.uid, discretizer(current_clock), get<tags::goods_type>(pallet_value));
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
            loading_logs.emplace_back(LOG_TYPE_HANDLE_PALLET, node.uid, discretizer(current_clock), nearest);
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

// TODO: [LATER] tweak distortion
//! @brief Computes the distance of every neighbour from a source, and the best waypoint towards it (distorting the nbr_dist metric).
FUN tuple<field<real_t>, device_t> distance_waypoint(ARGS, bool source, real_t distortion) { CODE
    return nbr(CALL, INF, [&] (field<real_t> d) {
        real_t dist;
        device_t waypoint;
        tie(dist, waypoint) = min_hood(CALL, make_tuple(d + node.nbr_dist(), node.nbr_uid()), make_tuple(source ? -distortion : INF, node.uid));
        dist += distortion;
        mod_self(CALL, d) = dist;
        return make_tuple(make_tuple(d, waypoint), dist);
    });
}
FUN_EXPORT distance_waypoint_t = common::export_list<real_t>;

// TODO: [LATER] tweak threshold
// TODO: [MAYBE] log only risk start and end
FUN std::vector<log_type> collision_detection(ARGS, real_t radius, real_t threshold, times_t current_clock) { CODE
    bool wearable = node.storage(tags::node_type{}) == warehouse_device_type::Wearable;
    std::unordered_map<device_t, real_t> logmap = spawn(CALL, [&](device_t source){
        auto t = distance_waypoint(CALL, node.uid == source, 0.1*comm);
        real_t dist = self(CALL, get<0>(t));
        real_t closest_wearable = nbr(CALL, INF, [&](field<real_t> x){
            return min_hood(CALL, mux(get<0>(t) > dist, x, INF), dist);
        });
        real_t v = 0;
        if (isfinite(closest_wearable))
            v = (old(CALL, closest_wearable) - closest_wearable) / (node.current_time() - node.previous_time());
        return make_tuple(dist < radius ? v : -INF, dist < radius);
    }, wearable ? common::option<device_t>{node.uid} : common::option<device_t>{});
    node.storage(tags::side_color{}) = color(BLACK);
    for (size_t i=0; i<wearable_node_num; ++i)
        if (logmap.count(i + pallet_node_num) and logmap.at(i + pallet_node_num) > -INF) {
            node.storage(tags::side_color{}) = color::hsva(i*360/wearable_node_num,1,1,1);
            break;
        }
    std::vector<log_type> logvec;
    if (logmap[node.uid] > threshold)
        logvec.emplace_back(LOG_TYPE_COLLISION_RISK, node.uid, discretizer(current_clock), logmap[node.uid]);
    return logvec;
}
FUN_EXPORT collision_detection_t = common::export_list<spawn_t<device_t, bool>, distance_waypoint_t, real_t>;

inline bool match(query_type const& q, pallet_content_type const& c) {
    return get<tags::goods_type>(q) == get<tags::goods_type>(c);
}

inline bool empty(query_type const& q) {
    return get<tags::goods_type>(q) == NO_GOODS;
}

// TODO: [LATER] set led on outside, to save further messages
FUN device_t find_space(ARGS, real_t grid_step) { CODE
    bool is_pallet = node.storage(tags::node_type{}) == warehouse_device_type::Pallet;
    int pallet_count = sum_hood(CALL, field<int>{node.nbr_dist() < 1.2 * grid_step and nbr(CALL, is_pallet)}, 0);
    bool source = is_pallet and pallet_count < 2;
    auto t = distance_waypoint(CALL, source, 0.1*comm);
    real_t dist = self(CALL, get<0>(t));
    device_t waypoint = get<1>(t);
    node.storage(tags::led_on{}) = any_hood(CALL, nbr(CALL, waypoint) == node.uid, false);
    return waypoint;
}
FUN_EXPORT find_space_t = common::export_list<distance_waypoint_t, bool, device_t>;

// TODO: [LATER] broadcast dist to cut propagation radius
// TODO: [LATER] set led on outside, to save further messages
// TODO: [MAYBE] bloom filter to guide process expansion
FUN device_t find_goods(ARGS, query_type query) { CODE
    using key_type = tuple<device_t,query_type>;
    std::unordered_map<key_type, device_t> resmap = spawn(CALL, [&](key_type const& key){
        bool found = match(get<1>(key), node.storage(tags::loaded_good{}));
        auto t = distance_waypoint(CALL, found, 0.1*comm);
        real_t dist = self(CALL, get<0>(t));
        device_t waypoint = get<1>(t);
        return make_tuple(waypoint, get<0>(key) != node.uid ? status::internal : empty(query) ? status::terminated : status::internal_output);
    }, empty(query) ? common::option<key_type>{} : common::option<key_type>{node.uid,query});
    device_t waypoint = resmap.empty() ? node.uid : resmap.begin()->second;
    node.storage(tags::led_on{}) = any_hood(CALL, nbr(CALL, waypoint) == node.uid, false);
    return waypoint;
}
FUN_EXPORT find_goods_t = common::export_list<spawn_t<tuple<device_t,query_type>, status>, distance_waypoint_t, device_t>;

bool is_sorted(std::vector<log_type> const& v) {
    for (size_t i=1; i<v.size(); ++i)
        if (v[i] <= v[i-1]) return false;
    return true;
}

FUN std::vector<log_type> single_log_collection(ARGS, std::vector<log_type> const& new_logs, int parity) { CODE
    bool source = node.uid % 2 == parity and node.storage(tags::node_type{}) == warehouse_device_type::Wearable;
    field<hops_t> nbrdist = nbr(CALL, std::numeric_limits<hops_t>::max(), [&](field<hops_t> d){
        hops_t nd = min_hood(CALL, d, source ? hops_t(-1) : std::numeric_limits<hops_t>::max()-1) + 1;
        mod_self(CALL, d) = nd;
        return make_tuple(std::move(d), nd);
    });
    hops_t dist = self(CALL, nbrdist);
    std::vector<log_type> r = nbr(CALL, std::vector<log_type>{}, [&](field<std::vector<log_type>> nl){
        std::vector<log_type> uplogs   = sum_hood(CALL, mux(nbrdist > dist, nl, std::vector<log_type>{}));
        std::vector<log_type> downlogs = sum_hood(CALL, mux(nbrdist < dist, nl, std::vector<log_type>{}));
        return (uplogs - downlogs) + new_logs;
    });
    assert(is_sorted(r));
    return source ? r : std::vector<log_type>{};
}
FUN_EXPORT single_log_collection_t = common::export_list<hops_t, std::vector<log_type>>;

FUN std::vector<log_type> log_collection(ARGS, std::vector<log_type> new_logs) { CODE
    std::sort(new_logs.begin(), new_logs.end());
    assert(is_sorted(new_logs));
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
    } else if (current_loaded_good >= 0 && current_loaded_good < 100) {
        node.storage(node_color{}) = color(GOLD);
    } else if (current_loaded_good == UNLOAD_GOODS) {
        node.storage(node_color{}) = color(DARK_GREEN);
    } else {
        node.storage(node_color{}) = color(RED);
    }
    if (node.storage(led_on{})) {
        node.storage(node_size{}) = (grid_cell_size * 3) / 2;
    } else {
        node.storage(node_size{}) = (grid_cell_size * 2) / 3;
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
        node.storage(tags::loaded_good{}) = random_good(CALL);
    }
}

FUN_EXPORT setup_nodes_if_first_round_of_simulation_t = common::export_list<uint32_t>;

unsigned int total_created_logs = 0;

std::set<log_type> received_logs;

unsigned int non_unique_received_logs = 0;

times_t total_delay_logs = 0.0;

FUN void collect_data_for_plot(ARGS, std::vector<log_type>& new_logs, std::vector<log_type>& collected_logs, times_t current_clock) { CODE
    node.storage(tags::log_received__perc{}) = received_logs.size() / (double)total_created_logs;
    node.storage(tags::log_created{}) = new_logs.size();
    node.storage(tags::msg_size{}) = node.msg_size();
    node.storage(tags::msg_received__perc{}) = node.msg_size() > MSG_SIZE_HARDWARE_LIMIT;
    node.storage(tags::log_collected{}) = collected_logs.size();
    std::vector<times_t> delays;
    transform(collected_logs.begin(), collected_logs.end(), back_inserter(delays), [current_clock](log_type log) -> times_t {
        uint8_t d = discretizer(current_clock) - get<tags::log_time>(log);
        return d * 0.1;
    });
    node.storage(tags::logging_delay{}) = delays;
}

//! @brief Main function.
MAIN() {
    std::vector<log_type> new_logs;
    setup_nodes_if_first_round_of_simulation(CALL);
    times_t shared_clock_value = coordination::shared_clock(CALL);
    maybe_change_loading_goods_for_simulation(CALL);
    std::vector<log_type> loading_logs = load_goods_on_pallet(CALL, shared_clock_value);
    new_logs.insert(new_logs.end(), loading_logs.begin(), loading_logs.end());
    std::vector<log_type> collision_logs = collision_detection(CALL, 2000, 300, shared_clock_value);
    new_logs.insert(new_logs.end(), collision_logs.begin(), collision_logs.end());
    find_goods(CALL, node.storage(tags::querying{}));
    total_created_logs += new_logs.size();
    std::vector<log_type> collected_logs = log_collection(CALL, new_logs);
    if (node.storage(tags::node_type{}) == warehouse_device_type::Wearable) {
        std::vector<log_type>& previously_collected_logs = node.storage(tags::logs{});
        previously_collected_logs.insert(previously_collected_logs.end(), collected_logs.begin(), collected_logs.end());
        std::copy(collected_logs.begin(), collected_logs.end(), std::inserter(received_logs, received_logs.end()));
        non_unique_received_logs += collected_logs.size();
        for (log_type log: collected_logs) {
            uint8_t d = discretizer(node.current_time()) - get<tags::log_time>(log);
            total_delay_logs += d * 0.1;
        }
    }
    collect_data_for_plot(CALL, new_logs, collected_logs, shared_clock_value);
    update_node_in_simulation(CALL);
    if (node.uid >= pallet_node_num)
        node.storage(tags::node_color{}) = color::hsva((node.uid-pallet_node_num)*360/wearable_node_num,1,1,1);
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
    maybe_change_loading_goods_for_simulation_t,
    size_t
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
    distribution::weibull_n<times_t, 100, 1, 100>,   // weibull-distributed time for interval (100/100=1 mean, 1/100=0.01 deviation)
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
    loaded_good,            pallet_content_type,
    loading_goods,          pallet_content_type,
    querying,               query_type,
    logs,                   std::vector<log_type>,
    led_on,                 bool,
    node_type,              warehouse_device_type,
    node_color,             color,
    side_color,             color,
    node_shape,             shape,
    node_size,              double,
    node_uid,               device_t,
    msg_size,               size_t,
    log_collected,          size_t,
    msg_received__perc,     bool,
    log_received__perc,     double,
    log_created,            unsigned int,
    logging_delay,          std::vector<times_t>
>;
//! @brief The tags and corresponding aggregators to be logged.
using aggregator_t = aggregators<
    msg_size,               aggregator::combine<aggregator::max<size_t>, aggregator::min<size_t>, aggregator::mean<double>>,
    msg_received__perc,     aggregator::mean<double>,
    log_collected,          aggregator::combine<aggregator::max<size_t>, aggregator::sum<size_t>>,
    log_created,            aggregator::combine<aggregator::max<size_t>, aggregator::sum<size_t>>,
    logging_delay,          aggregator::container<std::vector<times_t>, aggregator::combine<aggregator::max<times_t>, aggregator::mean<times_t>>>,
    log_received__perc,     aggregator::mean<double>
>;
//! @brief Message size plot.
using msg_plot_t = plot::split<plot::time, plot::values<aggregator_t, common::type_sequence<>, msg_size>>;
//! @brief Log plot.
using log_plot_t = plot::split<plot::time, plot::values<aggregator_t, common::type_sequence<>, log_created, log_collected>>;
//! @brief Loss percentage plot.
using loss_plot_t = plot::split<plot::time, plot::values<aggregator_t, common::type_sequence<>, msg_received__perc, log_received__perc>>;
//! @brief Log delay plot.
using delay_plot_t = plot::split<plot::time, plot::values<aggregator_t, common::type_sequence<>, logging_delay>>;
//! @brief The overall description of plots.
using plot_t = plot::join<msg_plot_t, log_plot_t, loss_plot_t, delay_plot_t>;

CONSTANT_DISTRIBUTION(false_distribution, bool, false);
CONSTANT_DISTRIBUTION(pallet_distribution, warehouse_device_type, warehouse_device_type::Pallet);
CONSTANT_DISTRIBUTION(wearable_distribution, warehouse_device_type, warehouse_device_type::Wearable);
CONSTANT_DISTRIBUTION(no_goods_distribution, pallet_content_type, fcpp::common::make_tagged_tuple<coordination::tags::goods_type>(NO_GOODS));
CONSTANT_DISTRIBUTION(no_query_distribution, query_type, fcpp::common::make_tagged_tuple<coordination::tags::goods_type>(NO_GOODS));

//! @brief The general simulation options.
DECLARE_OPTIONS(list,
    parallel<false>,     // no multithreading on node rounds
    synchronised<false>, // optimise for asynchronous networks
    message_size<true>,
    export_split<true>,
    program<coordination::main>,   // program to be run (refers to MAIN above)
    exports<coordination::main_t>, // export type list (types used in messages)
    retain<metric::retain<3>>, // 3s retain time of messages
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
        loading_goods, no_goods_distribution,
        querying, no_query_distribution,
        connection_data, distribution::constant_n<real_t, 6, 10>
    >,
    spawn_schedule<wearable_spawn_s>,
    init<
        x,      wearable_rectangle_d,
        led_on, false_distribution,
        node_type, wearable_distribution,
        loaded_good, no_goods_distribution,
        loading_goods, no_goods_distribution,
        querying, no_query_distribution,
        connection_data, distribution::constant_n<real_t, 1>
    >,
    plot_type<plot_t>, // the plot description to be used
    dimension<dim>, // dimensionality of the space
    connector<connect::radial<80,connect::powered<comm,1,dim>>>, // probabilistic connection within a comm range (50% loss at 80% radius)
    connector<connect::fixed<comm, 1, dim>>,
    shape_tag<node_shape>, // the shape of a node is read from this tag in the store
    size_tag<node_size>,   // the size of a node is read from this tag in the store
    color_tag<node_color, side_color>,  // colors of a node are read from these
    area<0,0,side,side_2>
);


} // namespace option


} // namespace fcpp


#endif // FCPP_WAREHOUSE_H_
