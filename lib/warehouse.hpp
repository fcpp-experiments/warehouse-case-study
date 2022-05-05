// Copyright © 2022 Giorgio Audrito and Lorenzo Testa. All Rights Reserved.

/**
 * @file warehouse.hpp
 * @brief Case study on smart warehouse management.
 *
 * This header file is designed to work under multiple execution paradigms.
 */

#ifndef FCPP_WAREHOUSE_H_
#define FCPP_WAREHOUSE_H_

#include "lib/fcpp.hpp"

#define NO_GOODS 255
#define UNDEFINED_GOODS 254

#define LOG_TYPE_PALLET_CONTENT_CHANGE 1
#define LOG_TYPE_HANDLE_PALLET 2
#define LOG_TYPE_COLLISION_RISK_START 3
#define LOG_TYPE_COLLISION_RISK_END 4

#if FCPP_ENVIRONMENT == FCPP_ENVIRONMENT_PHYSICAL
    #define MSG_SIZE_HARDWARE_LIMIT 222
#else
    #define MSG_SIZE_HARDWARE_LIMIT 222 + 20 // extra space needed for simulation
#endif

// [INTRODUCTION]

//! @brief Enumeration of device types.
enum class warehouse_device_type { Pallet, Wearable };

//! @brief Printing the device types.
std::string to_string(warehouse_device_type const& t) {
    switch (t) {
        case warehouse_device_type::Pallet:
            return "Pallet";
            break;
        case warehouse_device_type::Wearable:
            return "Wearable";
        default:
            return "Error";
    }
}

/**
 * @brief Namespace containing all the objects in the FCPP library.
 */
namespace fcpp {
//! @brief Tags used in the node storage and tagged tuples.
namespace coordination {
    namespace tags {
        //! @brief Goods in a pallet or query.
        struct goods_type {};
        //! @brief Type of a log.
        struct log_content_type {};
        //! @brief UID of the logging device.
        struct logger_id {};
        //! @brief Time of the log.
        struct log_time {};
        //! @brief Content of the log.
        struct log_content {};

        //! @brief A shared global clock.
        struct global_clock {};
        //! @brief Whether the device is a Wearable or a Pallet.
        struct node_type {};
        //! @brief Whether a pallet is currently being handled by a wearable.
        struct pallet_handled {};
        //! @brief A query for a good, if any.
        struct querying {};
        //! @brief The goods currently contained in a pallet.
        struct loaded_goods {};
        //! @brief The goods that a wearable is trying to load on a pallet.
        struct loading_goods{};
        //! @brief The logs newly generated by the device.
        struct new_logs {};
        //! @brief The logs being collected by the device.
        struct coll_logs {};
        //! @brief Whether the led is currently on.
        struct led_on {};
        //! @brief Message size of the last message sent.
        struct msg_size {};
        //! @brief Whether the last message was sent.
        struct msg_received__perc {};
        //! @brief The number of log entries just created.
        struct log_created {};
        //! @brief The number of log entries just collected.
        struct log_collected {};
        //! @brief The delays of received logs.
        struct logging_delay {};
    }
}


//! @brief Dummy ordering between positions (allows positions to be used as secondary keys in ordered tuples).
template <size_t n>
bool operator<(vec<n> const&, vec<n> const&) {
    return false;
}

//! @brief Type for the content description of pallets.
using pallet_content_type = common::tagged_tuple_t<coordination::tags::goods_type, uint8_t>;

//! @brief Type for logs.
using log_type = common::tagged_tuple_t<coordination::tags::log_content_type, uint8_t, coordination::tags::logger_id, device_t, coordination::tags::log_time, uint8_t, coordination::tags::log_content, uint16_t>;

//! @brief Type for queries.
using query_type = common::tagged_tuple_t<coordination::tags::goods_type, uint8_t>;

//! @brief Converts a floating-point time to a byte value (tenth of secs precision).
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

FUN device_t nearest_pallet_device(ARGS) { CODE
    field<uint8_t> nbr_pallet = nbr(CALL, uint8_t{node.storage(tags::node_type{}) == warehouse_device_type::Pallet});
    return get<1>(min_hood(CALL, make_tuple(mux(nbr_pallet, node.nbr_dist(), INF), node.nbr_uid())));
}
//! @brief Export list for nearest_pallet_device.
FUN_EXPORT nearest_pallet_device_t = export_list<uint8_t>;


//! @brief Computes the distance of every neighbour from a source, and the best waypoint towards it (distorting the nbr_dist metric).
FUN tuple<field<real_t>, device_t> distance_waypoint(ARGS, bool source, real_t distortion) { CODE
    return nbr(CALL, INF, [&] (field<real_t> d) {
        real_t dist;
        dist = min_hood(CALL, d + node.nbr_dist(), source ? -distortion : INF);
        dist += distortion;
        mod_self(CALL, d) = dist;
        device_t waypoint = get<1>(min_hood(CALL, make_tuple(d, node.nbr_uid())));
        return make_tuple(make_tuple(d, waypoint), dist);
    });
}
//! @brief Export list for distance_waypoint.
FUN_EXPORT distance_waypoint_t = export_list<real_t>;


//! @brief Null content.
constexpr pallet_content_type null_content{UNDEFINED_GOODS};

//! @brief No content.
constexpr pallet_content_type no_content{NO_GOODS};

//! @brief Extract content for logging.
inline uint16_t log_content(pallet_content_type const& c) {
    return get<tags::goods_type>(c);
}

//! @brief Loads a content.
inline void load_content(pallet_content_type& c, pallet_content_type const& l) {
    c = l;
}

//! @brief Turns loading_goods on wearables into loaded_goods for the closest pallet.
FUN std::vector<log_type> load_goods_on_pallet(ARGS, times_t current_clock) { CODE
    // currently loaded good (pallet) and good to be loaded (wearable)
    pallet_content_type& loading = node.storage(tags::loading_goods{});
    pallet_content_type& loaded  = node.storage(tags::loaded_goods{});
    // whether I am a wearable that is about to load
    bool is_loading = loading != null_content;
    // the loading or loaded good of a neighbor
    field<uint8_t> nbr_good = nbr(CALL, get<0>(is_loading ? loading : loaded));
    // the nearest pallet device for loading neighbors
    device_t nearest = nearest_pallet_device(CALL);
    field<real_t> nbr_nearest = nbr(CALL, is_loading ? constant(CALL, (real_t)nearest) : (real_t)node.uid);
    // the loading logs vector
    std::vector<log_type> loading_logs;
    // a loading wearable with a matching nearest good is reset
    if (is_loading and details::self(nbr_good, nearest) == get<0>(loading)) {
        loading = null_content;
        loading_logs.emplace_back(LOG_TYPE_HANDLE_PALLET, node.uid, discretizer(current_clock), nearest);
    }
    // loading good if nearest for a neighbor (breaking ties by highest good type)
    auto t = max_hood(CALL, fcpp::make_tuple(nbr_nearest == node.uid, nbr_good), fcpp::make_tuple(false, get<0>(no_content)));
    if (get<0>(t) and get<0>(loaded) != get<1>(t)) {
        node.storage(tags::pallet_handled{}) = true;
        load_content(loaded, get<1>(t));
        loading_logs.emplace_back(LOG_TYPE_PALLET_CONTENT_CHANGE, node.uid, discretizer(current_clock), log_content(get<1>(t)));
    }
    // return loading logs
    return loading_logs;
}
//! @brief Export list for load_goods_on_pallet.
FUN_EXPORT load_goods_on_pallet_t = export_list<nearest_pallet_device_t, constant_t<real_t>, real_t, uint8_t>;


//! @brief Detects potential collision risks.
FUN std::vector<log_type> collision_detection(ARGS, real_t radius, real_t threshold, times_t current_clock, real_t comm) { CODE
    bool wearable = node.storage(tags::node_type{}) == warehouse_device_type::Wearable;
    std::unordered_map<device_t, real_t> logmap = spawn(CALL, [&](device_t source){
        auto t = distance_waypoint(CALL, node.uid == source, 0.1*comm);
        real_t dist = self(CALL, get<0>(t));
        real_t closest_wearable = nbr(CALL, INF, [&](field<real_t> x){
            return min_hood(CALL, mux(get<0>(t) > dist, x, INF), wearable and node.uid != source ? dist : INF);
        });
        real_t v = 0;
        if (isfinite(closest_wearable))
            v = (old(CALL, closest_wearable) - closest_wearable) / (node.current_time() - node.previous_time());
        return make_tuple(dist < radius ? v : -INF, dist < radius);
    }, wearable ? common::option<device_t>{node.uid} : common::option<device_t>{});
    std::vector<log_type> logvec;
    real_t vn = max(logmap[node.uid], real_t(0));
    real_t vo = old(CALL, vn);
    if (vn > threshold and vo <= threshold)
        logvec.emplace_back(LOG_TYPE_COLLISION_RISK_START, node.uid, discretizer(current_clock), vn);
    if (vo > threshold and vn <= threshold)
        logvec.emplace_back(LOG_TYPE_COLLISION_RISK_END, node.uid, discretizer(current_clock), vn);
    return logvec;
}
//! @brief Export list for collision_detection.
FUN_EXPORT collision_detection_t = export_list<spawn_t<device_t, bool>, distance_waypoint_t, real_t>;


//! @brief Combinatorics over neighbor distances to find whether there is a nearby space (unused).
FUN bool smart_nearby_space(ARGS, bool is_pallet, real_t grid_step) { CODE
    field<bool> nbr_pallet = nbr(CALL, is_pallet);
    if (not is_pallet) return false;
    field<int> ndi = round(node.nbr_dist() * node.nbr_dist() / (grid_step * grid_step)) * nbr_pallet;
    int dc[6] = {0,0,0,0,0,0};
    for (int i=1; i<6; ++i)
        dc[i] = sum_hood(CALL, field<int>(ndi == i));
    dc[0] = dc[1] + dc[5];
    dc[3] = dc[2] + dc[4];
    dc[4] += dc[1];
    dc[5] += dc[2];
    constexpr int lc[6] = {3, 2, 1, 2, 3, 2};
    for (int i=0; i<6; ++i) if (dc[i] < lc[i])
        return true;
    return false;
}
//! @brief Export list for smart_nearby_space.
FUN_EXPORT smart_nearby_space_t = export_list<bool>;

//! @brief Searches the direction towards the closest space.
FUN device_t find_space(ARGS, real_t grid_step, real_t comm) { CODE
    bool is_pallet = node.storage(tags::node_type{}) == warehouse_device_type::Pallet and
        node.storage(tags::loaded_goods{}) != no_content and
        node.storage(tags::pallet_handled{}) == false;
    int pallet_count = fold_hood(CALL, [&](tuple<real_t, uint8_t> t, int c){
        return c + (get<0>(t) < 1.2 * grid_step and get<1>(t));
    }, make_tuple(node.nbr_dist(), nbr(CALL, uint8_t{is_pallet})), 0);
    bool source = is_pallet and pallet_count < 2;
    auto t = distance_waypoint(CALL, source, 0.1*comm);
    return get<1>(t);
}
//! @brief Export list for find_space.
FUN_EXPORT find_space_t = export_list<distance_waypoint_t, uint8_t>;

//! @brief No query.
constexpr query_type no_query{NO_GOODS};

//! @brief Whether a pallet content matches a query.
inline bool match(query_type const& q, pallet_content_type const& c) {
    return get<tags::goods_type>(q) == get<tags::goods_type>(c);
}

//! @brief Searches the direction towards the closest pallet with a good matching the query.
FUN device_t find_goods(ARGS, query_type query, real_t comm) { CODE
    using key_type = tuple<device_t,query_type>;
    std::unordered_map<key_type, device_t> resmap = spawn(CALL, [&](key_type const& key){
        bool found = match(get<1>(key), node.storage(tags::loaded_goods{})) and node.storage(tags::pallet_handled{}) == false;
        auto t = distance_waypoint(CALL, found, 0.1*comm);
        device_t waypoint = get<1>(t);
        return make_tuple(waypoint, get<0>(key) != node.uid ? status::internal : query == no_query ? status::terminated : status::internal_output);
    }, query == no_query ? common::option<key_type>{} : common::option<key_type>{node.uid,query});
    return resmap.empty() ? node.uid : resmap.begin()->second;
}
//! @brief Export list for find_goods.
FUN_EXPORT find_goods_t = export_list<spawn_t<tuple<device_t,query_type>, status>, distance_waypoint_t>;


//! @brief Checks whether a vector of logs is sorted.
bool is_sorted(std::vector<log_type> const& v) {
    for (size_t i=1; i<v.size(); ++i)
        if (v[i] <= v[i-1]) return false;
    return true;
}

//! @brief Collects logs towards wearables of given UID parity.
FUN std::vector<log_type> single_log_collection(ARGS, std::vector<log_type> const& new_logs, int parity) { CODE
    bool source = node.uid % 2 == parity and node.storage(tags::node_type{}) == warehouse_device_type::Wearable;
    field<uint8_t> nbrdist = nbr(CALL, std::numeric_limits<uint8_t>::max(), [&](field<uint8_t> d){
        uint8_t nd = min_hood(CALL, d, std::numeric_limits<uint8_t>::max());
        nd = source ? 0 : nd + 1;
        mod_self(CALL, d) = nd;
        return make_tuple(std::move(d), nd);
    });
    uint8_t dist = self(CALL, nbrdist);
    std::vector<log_type> r = nbr(CALL, std::vector<log_type>{}, [&](field<std::vector<log_type>> nl){
        std::vector<log_type> uplogs   = sum_hood(CALL, mux(nbrdist > dist, nl, std::vector<log_type>{}));
        std::vector<log_type> downlogs = sum_hood(CALL, mux(nbrdist < dist, nl, std::vector<log_type>{}));
        return (uplogs - downlogs) + new_logs;
    });
    assert(is_sorted(r));
    return source ? r : std::vector<log_type>{};
}
//! @brief Export list for single_log_collection.
FUN_EXPORT single_log_collection_t = export_list<uint8_t, std::vector<log_type>>;

//! @brief Collects logs towards wearables with redundancy.
FUN std::vector<log_type> log_collection(ARGS, std::vector<log_type> const& new_logs) { CODE
    assert(is_sorted(new_logs));
    std::vector<log_type> r0 = single_log_collection(CALL, new_logs, 0);
    std::vector<log_type> r1 = single_log_collection(CALL, new_logs, 1);
    return r0.empty() ? r1 : r0;
}
//! @brief Export list for log_collection.
FUN_EXPORT log_collection_t = export_list<single_log_collection_t>;


//! @brief Computes some statistics for network analysis.
FUN void statistics(ARGS, times_t current_clock) { CODE
    // message size stats
    node.storage(tags::msg_size{}) = node.msg_size();
    node.storage(tags::msg_received__perc{}) = node.msg_size() <= MSG_SIZE_HARDWARE_LIMIT;
    // log size and delay stats
    node.storage(tags::log_created{}) = node.storage(tags::new_logs{}).size();
    node.storage(tags::log_collected{}) = node.storage(tags::coll_logs{}).size();
    std::vector<times_t> delays;
    for (auto const& log : node.storage(tags::coll_logs{})) {
        uint8_t d = discretizer(current_clock) - get<tags::log_time>(log);
        delays.push_back(d * 0.1);
    }
    node.storage(tags::logging_delay{}) = delays;
}
//! @brief Export list for statistics.
FUN_EXPORT statistics_t = export_list<>;


//! @brief Application for warehouse assistance.
FUN device_t warehouse_app(ARGS, real_t grid_step, real_t comm_rad, real_t safety_radius, real_t safe_speed) { CODE
    bool is_pallet = node.storage(tags::node_type{}) == warehouse_device_type::Pallet;
    times_t current_clock = shared_clock(CALL);
    node.storage(tags::global_clock{}) = current_clock;
    std::vector<log_type>& logs = node.storage(tags::new_logs{});
    logs = {};
    logs = logs + load_goods_on_pallet(CALL, current_clock);
    logs = logs + collision_detection(CALL, safety_radius, safe_speed, current_clock, comm_rad);
    node.storage(tags::coll_logs{}) = log_collection(CALL, logs);
    device_t space_waypoint = find_space(CALL, grid_step, comm_rad);
    device_t goods_waypoint = find_goods(CALL, node.storage(tags::querying{}), comm_rad);
    device_t waypoint = is_pallet ? node.uid : node.storage(tags::querying{}) == no_query ? space_waypoint : goods_waypoint;
    node.storage(tags::led_on{}) = any_hood(CALL, nbr(CALL, (real_t)waypoint) == node.uid, false);
    statistics(CALL, current_clock);
    return waypoint;
}
//! @brief Export list for warehouse_app.
FUN_EXPORT warehouse_app_t = export_list<shared_clock_t, load_goods_on_pallet_t, collision_detection_t, find_space_t, find_goods_t, real_t, log_collection_t, statistics_t>;

} // namespace coordination

//! @brief Namespace for component options.
namespace option {

//! @brief Import tags to be used for component options.
using namespace component::tags;
//! @brief Import tags used by aggregate functions.
using namespace coordination::tags;

//! @brief Data in the node storage.
using store_t = tuple_store<
    loaded_goods,           pallet_content_type,
    loading_goods,          pallet_content_type,
    querying,               query_type,
    new_logs,               std::vector<log_type>,
    coll_logs,              std::vector<log_type>,
    led_on,                 bool,
    global_clock,           times_t,
    node_type,              warehouse_device_type,
    msg_size,               size_t,
    msg_received__perc,     bool,
    log_collected,          size_t,
    log_created,            unsigned int,
    logging_delay,          std::vector<times_t>,
    pallet_handled,         bool
>;

//! @brief Dictates that messages are thrown away after 5/1 seconds.
using retain_t = retain<metric::retain<5, 1>>;

//! @brief The general options.
DECLARE_OPTIONS(general,
    export_split<true>,
    retain_t,
    store_t
);

} // namespace option

} // namespace fcpp


#endif // FCPP_WAREHOUSE_H_
