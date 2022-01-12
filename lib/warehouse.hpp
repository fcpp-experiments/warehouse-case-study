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
constexpr size_t pallet_node_num = 500;
constexpr size_t empty_pallet_node_num = 10;
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
        struct wearable_sim_op {};
        struct wearable_sim_target_pos {};
        struct pallet_sim_follow {};
        struct pallet_sim_follow_pos {};
        struct pallet_sim_handling {};
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

using wearable_sim_state_type = fcpp::tuple<uint8_t, uint8_t, device_t>;

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

// TODO: use nbr_pallet instead (in common with find_space)
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
    bool is_pallet = node.storage(tags::node_type{}) == warehouse_device_type::Pallet && 
        get<tags::goods_type>(node.storage(tags::loaded_good{})) != NO_GOODS &&
        node.storage(tags::pallet_sim_handling{}) == false;
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
        bool found = match(get<1>(key), node.storage(tags::loaded_good{})) && node.storage(tags::pallet_sim_handling{}) == false;
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

} // namespace coordination

} // namespace fcpp


#endif // FCPP_WAREHOUSE_H_
