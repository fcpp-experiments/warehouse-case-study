#ifndef FCPP_WAREHOUSE_SIMULATION_H_
#define FCPP_WAREHOUSE_SIMULATION_H_

#include "lib/warehouse.hpp"

#define WEARABLE_IDLE 0
#define WEARABLE_INSERT 1
#define WEARABLE_RETRIEVE 2
#define WEARABLE_INSERTING 3
#define WEARABLE_RETRIEVING 4
#define WEARABLE_INSERTED 5

//! @brief Number of pallet devices.
constexpr size_t pallet_node_num = 500;
//! @brief Number of wearable devices.
constexpr size_t wearable_node_num = 6;
//! @brief The final simulation time.
constexpr size_t end_time = 300;
constexpr size_t empty_pallet_node_num = 10;
//! @brief Dimensionality of the space.
constexpr size_t dim = 3;
//! @brief Side of the area.
constexpr size_t side = 8550;
constexpr size_t side_2 = 9450;
//! @brief Height of the area.
constexpr size_t height = 1000;
//! @brief Communication radius (25m w-w, 15m w-p, 9m p-p).
constexpr size_t comm = 2500;

constexpr size_t grid_cell_size = 150;

constexpr fcpp::real_t forklift_max_speed = 280;

constexpr size_t distance_to_consider_same_space = 100; // must be less than grid_cell_size

constexpr size_t loading_zone_bound_x_0 = grid_cell_size * 2;
constexpr size_t loading_zone_bound_x_1 = grid_cell_size * 34;
constexpr size_t loading_zone_bound_y_0 = grid_cell_size * 2;
constexpr size_t loading_zone_bound_y_1 = grid_cell_size * 8;

namespace fcpp {

using wearable_sim_state_type = fcpp::tuple<uint8_t, uint8_t, device_t>;

namespace coordination {

    namespace tags {
        //! @brief Color of the current node.
        struct node_color {};
        //! @brief Size of the current node.
        struct node_size {};
        //! @brief Shape of the current node.
        struct node_shape {};
        struct node_uid {};
        struct logs {};
        struct log_received__perc {};
        struct wearable_sim_op {};
        struct wearable_sim_target_pos {};
        struct pallet_sim_follow {};
        struct pallet_sim_follow_pos {};
    }

FUN uint8_t random_good(ARGS) { CODE
    constexpr real_t f = 5.187377517639621;
    real_t r = node.next_real(f);
    for (uint8_t i=1; ; ++i) {
        r -= real_t(1)/i;
        if (r < 0) return i-1;
    }
    return 99;
}

//! @brief Computes the next waypoint towards target q while avoiding obstacles.
FUN vec<3> waypoint_target(ARGS, vec<3> q) { CODE
    // rescale for convenience
    vec<3> p = node.position() / grid_cell_size;
    q /= grid_cell_size;
    // discretized target x
    real_t qx = int((q[0]-1)/5) * 5 + 3.5;
    // if close to target
    if (abs(qx-p[0]) <= 2.5 and abs(q[1]-p[1]) <= 1)
        return make_vec(q[0], q[1], 0) * grid_cell_size;
    // if same vertical corridor as target
    if (abs(qx-p[0]) <= 1)
        return make_vec(p[0], q[1], 0) * grid_cell_size;
    // if in horizontal corridor
    if (int(p[1])%18 == 7)
        return make_vec(qx, p[1], 0) * grid_cell_size;
    // if in vertical corridor
    if (int(p[0])%5 == 3) {
        real_t qy = (p[1] + q[1])/2;
        qy = int((qy+1.5)/18) * 18 + 7.5;
        return make_vec(p[0], qy, 0) * grid_cell_size;
    }
    // otherwise
    qx = int((p[0]-1)/5) * 5 + 3.5;
    return make_vec(qx, p[1], 0) * grid_cell_size;
}

FUN void update_node_visually_in_simulation(ARGS) { CODE
    using namespace tags;
    node.storage(node_uid{}) = node.uid;
    uint8_t current_loaded_good = NO_GOODS;
    if (node.storage(node_type{}) == warehouse_device_type::Pallet) {
        node.storage(node_shape{}) = shape::cube;
        current_loaded_good = get<tags::goods_type>(node.storage(loaded_goods{}));
    } else {
        if (node.storage(loading_goods{}) == null_content) {
            node.storage(node_shape{}) = shape::sphere;
        } else {
            node.storage(node_shape{}) = shape::star;
        }
        current_loaded_good = get<tags::goods_type>(node.storage(loading_goods{}));
    }
    if (current_loaded_good == UNDEFINED_GOODS) {
        node.storage(node_color{}) = color(GREEN);
    } else if (current_loaded_good >= 0 && current_loaded_good < 100) {
        node.storage(node_color{}) = color(GOLD);
    } else if (current_loaded_good == NO_GOODS) {
        node.storage(node_color{}) = color(DARK_GREEN);
    } else {
        node.storage(node_color{}) = color(RED);
    }
    if (node.storage(pallet_handled{})) {
        node.storage(node_color{}) = color(DARK_BLUE);
    }
    if (node.storage(led_on{})) {
        node.storage(node_size{}) = (grid_cell_size * 3) / 2;
    } else {
        node.storage(node_size{}) = (grid_cell_size * 2) / 3;
    }
    if (node.uid >= pallet_node_num && node.uid < pallet_node_num + wearable_node_num)
        node.storage(tags::node_color{}) = color::hsva((node.uid-pallet_node_num)*360/wearable_node_num,1,1,1);
}

FUN_EXPORT update_node_visually_in_simulation_t = common::export_list<vec<dim>, int>;

std::set<tuple<int,int,int>> used_slots;

std::vector<int> goods_counter(100, 0);

FUN void setup_nodes_if_first_round_of_simulation(ARGS) { CODE
    if (coordination::counter(CALL) == 1 && 
            node.storage(tags::node_type{}) == warehouse_device_type::Pallet) {
        if (node.position()[0] > loading_zone_bound_x_1 && 
                node.position()[1] > loading_zone_bound_y_1) {
            int row, col, height;
            real_t x, y, z;
            do {
                row = node.next_int(1,22);
                col = node.next_int(0,44);
                height = node.next_int(0,2);
                x = ((((row / 2) * 3) + row) * grid_cell_size) + (grid_cell_size / 2);
                y = ((((col / 15) * 3) + col + 9) * grid_cell_size) + (grid_cell_size / 2);
                z = height * grid_cell_size;
            } while (used_slots.find(make_tuple(row,col,height)) != used_slots.end());
            used_slots.insert(make_tuple(row,col,height));
            node.position() = make_vec(x,y,z);
            uint8_t init_good = random_good(CALL);
            node.storage(tags::loaded_goods{}) = init_good;
            goods_counter[init_good] = goods_counter[init_good] + 1;
        } else {
            node.position() = make_vec(loading_zone_bound_x_0 + (node.next_int(1, 33) * grid_cell_size), loading_zone_bound_y_0 + (node.next_int(0, 3) * grid_cell_size), 0);
        }
    }
}

FUN_EXPORT setup_nodes_if_first_round_of_simulation_t = common::export_list<uint32_t>;

unsigned int total_created_logs = 0;

std::set<log_type> received_logs;

unsigned int non_unique_received_logs = 0;

FUN void simulation_statistics(ARGS) { CODE
    std::vector<log_type>& new_logs = node.storage(tags::new_logs{});
    std::vector<log_type>& coll_logs = node.storage(tags::coll_logs{});
    total_created_logs += new_logs.size();
    non_unique_received_logs += coll_logs.size();
    std::copy(coll_logs.begin(), coll_logs.end(), std::inserter(received_logs, received_logs.end()));
    node.storage(tags::log_received__perc{}) = received_logs.size() / (double)total_created_logs;
}

FUN real_t distance_from(ARGS, vec<dim> const& other) { CODE
    vec<dim> other_copy(other); // consider it only horizontally
    other_copy[2] = 0;
    vec<dim> position_copy(node.position());
    position_copy[2] = 0;
    return norm(other_copy - position_copy);
}

FUN void stop_mov(ARGS) { CODE
    node.propulsion() = make_vec(0,0,0);
    rectangle_walk(CALL, make_vec(0,0,0), make_vec(0,0,0), 0, 1);
}

FUN bool pallet_in_near_location(ARGS, vec<dim> loc) { CODE
    for (auto pallets : details::get_ids(node.nbr_uid())) {
        if (node.net.node_count(pallets) &&
                norm(loc - node.net.node_at(pallets).position()) < distance_to_consider_same_space) {
            return true;
        }
    }
    return false;
}

FUN vec<dim> find_actual_space(ARGS, device_t near) { CODE
    vec<dim> const& near_position = node.net.node_at(near).position();
    vec<dim> test_position(near_position);
    vec<dim> limit_test(near_position);
    bool found = false;
    for (uint8_t i = 0; i < 3 && !found; i++) {
        limit_test[1] = (((i * 4) + (14 * (i + 1)) + 9) * grid_cell_size) + (grid_cell_size / 2);
        found = norm(near_position - limit_test) < distance_to_consider_same_space;
    }
    if (!found) {
        test_position[1] = test_position[1] + grid_cell_size;
        if (!pallet_in_near_location(CALL, test_position)) {
            return test_position;
        }
    }
    test_position = near_position;
    found = false;
    for (uint8_t i = 0; i < 3 && !found; i++) {
        limit_test[1] = (((i * 18) + 9) * grid_cell_size) + (grid_cell_size / 2);
        found = norm(near_position - limit_test) < distance_to_consider_same_space;
    }
    if (!found) {
        test_position[1] = test_position[1] - grid_cell_size;
        if (!pallet_in_near_location(CALL, test_position)) {
            return test_position;
        }
    }
    test_position = near_position;
    if (near_position[2] / grid_cell_size < 2) {
        test_position[2] = test_position[2] + grid_cell_size;
        if (!pallet_in_near_location(CALL, test_position)) {
            return test_position;
        }
    }
    if (near_position[2] / grid_cell_size > 0) {
        test_position[2] = test_position[2] - grid_cell_size;
        if (!pallet_in_near_location(CALL, test_position)) {
            return test_position;
        }
    }
    return make_vec(near_position[0],near_position[1],grid_cell_size * 10);
}

FUN void update_simulation_pre_program(ARGS) { CODE
    common::unique_lock<false> lock;
    device_t nearest_pallet = nearest_pallet_device(CALL);
    if (node.storage(tags::node_type{}) == warehouse_device_type::Wearable) {
        wearable_sim_state_type current_state = node.storage(tags::wearable_sim_op{});
        if (get<0>(current_state) == WEARABLE_IDLE) {
            if (node.next_int(1,20) == 1) { // 20% change to start acting
                uint8_t new_action = node.next_int(1,2);
                uint8_t new_good = node.next_int(0,99);
                if (new_action == WEARABLE_RETRIEVE) { // use a good that is somewhere
                    bool found = false;
                    while (!found) {
                        new_good = node.next_int(0,99);
                        if (goods_counter[new_good] > 0) {
                            found = true;
                        }
                    }
                }
                node.storage(tags::wearable_sim_op{}) = make_tuple(new_action, new_good, 0);
            }
        } else if (get<0>(current_state) == WEARABLE_INSERT) {
            if (get<2>(current_state) == 0) {
                for (auto search_candidate : details::get_ids(node.nbr_uid())) {
                    if (node.net.node_count(search_candidate) &&
                            node.net.node_at(search_candidate).storage(tags::node_type{}) == warehouse_device_type::Pallet &&
                            get<tags::goods_type>(node.net.node_at(search_candidate).storage(tags::loaded_goods{})) == NO_GOODS &&
                            node.net.node_at(search_candidate).storage(tags::pallet_handled{}) == false) {
                        node.storage(tags::wearable_sim_op{}) = make_tuple(WEARABLE_INSERT, get<1>(current_state), search_candidate);
                        node.net.node_at(search_candidate, lock).storage(tags::pallet_handled{}) = true;
                        break;
                    }
                }
            } else if (node.net.node_count(get<2>(current_state)) &&
                        distance_from(CALL, node.net.node_at(get<2>(current_state)).position()) < distance_to_consider_same_space &&
                        nearest_pallet == get<2>(current_state)) {
                if (get<tags::goods_type>(node.net.node_at(get<2>(current_state)).storage(tags::loaded_goods{})) == get<1>(current_state)) {
                    node.net.node_at(get<2>(current_state), lock).storage(tags::pallet_sim_follow{}) = node.uid;
                    node.storage(tags::wearable_sim_op{}) = make_tuple(WEARABLE_INSERTING, get<1>(current_state), get<2>(current_state));
                    node.storage(tags::loading_goods{}) = null_content;
                } else {
                    node.storage(tags::loading_goods{}) = common::make_tagged_tuple<tags::goods_type>(get<1>(current_state));
                }
            }
        } else if (get<0>(current_state) == WEARABLE_RETRIEVE) {
            node.storage(tags::querying{}) = common::make_tagged_tuple<coordination::tags::goods_type>(get<1>(current_state));
        } else if (get<0>(current_state) == WEARABLE_RETRIEVING &&
                    node.net.node_count(get<2>(current_state))) {
            if (make_vec(0,0,0) == node.storage(tags::wearable_sim_target_pos{})) {
                node.net.node_at(get<2>(current_state), lock).storage(tags::pallet_sim_follow{}) = node.uid;
                node.storage(tags::querying{}) = common::make_tagged_tuple<tags::goods_type>(NO_GOODS);
                int random_x = node.next_int(loading_zone_bound_x_0, loading_zone_bound_x_1);
                int random_y = loading_zone_bound_y_0 + (node.next_int(0, 3) * grid_cell_size);
                node.storage(tags::wearable_sim_target_pos{}) = make_vec(random_x, random_y, 0);
            } else if (distance_from(CALL, node.storage(tags::wearable_sim_target_pos{})) < distance_to_consider_same_space &&
                    distance_from(CALL, node.net.node_at(get<2>(current_state)).position()) < distance_to_consider_same_space &&
                    nearest_pallet == get<2>(current_state)) {
                node.net.node_at(get<2>(current_state), lock).storage(tags::pallet_sim_follow{}) = 0;
                node.net.node_at(get<2>(current_state), lock).storage(tags::pallet_handled{}) = false;
                goods_counter[get<1>(current_state)] = goods_counter[get<1>(current_state)] - 1;
                node.storage(tags::wearable_sim_op{}) = make_tuple(WEARABLE_IDLE, NO_GOODS, 0);
                node.storage(tags::wearable_sim_target_pos{}) = make_vec(0,0,0);
                node.storage(tags::loading_goods{}) = no_content;
            }
        } else if (get<0>(current_state) == WEARABLE_INSERTED) {
            if (make_vec(0,0,0) == node.storage(tags::wearable_sim_target_pos{})) {
                int random_x = node.next_int(loading_zone_bound_x_0, loading_zone_bound_x_1);
                int random_y = node.next_int(loading_zone_bound_y_0, loading_zone_bound_y_1);
                node.storage(tags::wearable_sim_target_pos{}) = make_vec(random_x, random_y, 0);
            } else if (distance_from(CALL, node.storage(tags::wearable_sim_target_pos{})) < distance_to_consider_same_space) {
                goods_counter[get<1>(current_state)] = goods_counter[get<1>(current_state)] + 1;
                node.storage(tags::wearable_sim_op{}) = make_tuple(WEARABLE_IDLE, NO_GOODS, 0);
                node.storage(tags::wearable_sim_target_pos{}) = make_vec(0,0,0);
            }
        }
    }
}

FUN_EXPORT update_simulation_pre_program_t = common::export_list<wearable_sim_state_type, vec<dim>>;

FUN void update_simulation_post_program(ARGS, device_t waypoint) { CODE
    common::unique_lock<false> lock;
    if (node.storage(tags::node_type{}) == warehouse_device_type::Wearable) {
        wearable_sim_state_type current_state = node.storage(tags::wearable_sim_op{});
        if (get<0>(current_state) == WEARABLE_IDLE) {
            stop_mov(CALL);
        } else if (get<0>(current_state) == WEARABLE_INSERT) {
            if (get<2>(current_state) != 0 && node.net.node_count(get<2>(current_state))) {
                follow_target(CALL, node.net.node_at(get<2>(current_state)).position(), forklift_max_speed, real_t(1.0));
            }
        } else if (get<0>(current_state) == WEARABLE_INSERTING && node.net.node_count(waypoint)) {
            node.storage(tags::pallet_sim_follow{}) = waypoint;
            vec<dim> const& target_position = waypoint_target(CALL, node.net.node_at(waypoint).position());
            if (distance_from(CALL, target_position) < (distance_to_consider_same_space * 2.5) &&
                    node.net.node_count(get<2>(current_state))) {
                stop_mov(CALL);
                auto& pallet_node = node.net.node_at(get<2>(current_state), lock);
                if (norm(pallet_node.position() - pallet_node.storage(tags::pallet_sim_follow_pos{})) < distance_to_consider_same_space) {
                    node.storage(tags::wearable_sim_op{}) = make_tuple(WEARABLE_INSERTED, get<1>(current_state), get<2>(current_state));
                    node.net.node_at(get<2>(current_state), lock).storage(tags::pallet_sim_follow{}) = 0;
                    node.net.node_at(get<2>(current_state), lock).storage(tags::pallet_handled{}) = false;
                    pallet_node.storage(tags::pallet_sim_follow_pos{}) = make_vec(0,0,0);
                } else {
                    pallet_node.storage(tags::pallet_sim_follow{}) = 0;
                    pallet_node.storage(tags::pallet_sim_follow_pos{}) = find_actual_space(CALL, waypoint);
                }
            } else {
                follow_target(CALL, target_position, forklift_max_speed, real_t(1.0));
            }
        } else if (get<0>(current_state) == WEARABLE_RETRIEVE && node.net.node_count(waypoint)) {
            vec<dim> const& target_position = node.net.node_at(waypoint).position();
            if (get<tags::goods_type>(node.net.node_at(waypoint).storage(tags::loaded_goods{})) == get<1>(current_state) &&
                    node.net.node_at(waypoint).storage(tags::pallet_handled{}) == false &&
                    distance_from(CALL, target_position) < distance_to_consider_same_space) {
                stop_mov(CALL);
                node.net.node_at(waypoint, lock).storage(tags::pallet_handled{}) = true;
                node.storage(tags::wearable_sim_op{}) = make_tuple(WEARABLE_RETRIEVING, get<1>(current_state), waypoint);
            } else {
                follow_target(CALL, waypoint_target(CALL, target_position), forklift_max_speed, real_t(1.0));
            }
        } else if (get<0>(current_state) == WEARABLE_RETRIEVING || get<0>(current_state) == WEARABLE_INSERTED) {
            follow_target(CALL, waypoint_target(CALL, node.storage(tags::wearable_sim_target_pos{})), forklift_max_speed, real_t(1.0));
        }
    } else {
        if (node.storage(tags::pallet_sim_follow{}) != 0 && node.net.node_count(node.storage(tags::pallet_sim_follow{}))) {
            follow_target(CALL, node.net.node_at(node.storage(tags::pallet_sim_follow{})).position(), forklift_max_speed * 2, real_t(1.0));
        } else if (node.storage(tags::pallet_sim_follow_pos{}) != make_vec(0,0,0)) {
            follow_target(CALL, node.storage(tags::pallet_sim_follow_pos{}), forklift_max_speed, real_t(1.0));
        } else {
            stop_mov(CALL);
        }
    }
}

FUN_EXPORT update_simulation_post_program_t = common::export_list<real_t>;

//! @brief Main function.
MAIN() {
    setup_nodes_if_first_round_of_simulation(CALL);
    update_simulation_pre_program(CALL);
    device_t waypoint = warehouse_app(CALL, grid_cell_size, comm, 2000, 300);
    simulation_statistics(CALL);
    update_simulation_post_program(CALL, waypoint);
    update_node_visually_in_simulation(CALL);
}
//! @brief Export types used by the main function.
FUN_EXPORT main_t = common::export_list<
    nearest_pallet_device_t,
    load_goods_on_pallet_t, 
    collision_detection_t,
    find_space_t,
    find_goods_t,
    log_collection_t, 
    update_node_visually_in_simulation_t, 
    setup_nodes_if_first_round_of_simulation_t,
    update_simulation_pre_program_t,
    update_simulation_post_program_t,
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

using empty_pallet_spawn_s = sequence::multiple_n<empty_pallet_node_num, 0>;

using wearable_spawn_s = sequence::multiple_n<wearable_node_num, 0>;
//! @brief The distribution of initial node positions (random in a given rectangle).
using pallet_rectangle_d = distribution::rect_n<1, loading_zone_bound_x_1, loading_zone_bound_y_1, 0, side, side_2, 0>;
//! @brief The distribution of initial node positions (random in a given rectangle).
using wearable_rectangle_d = distribution::rect_n<1, loading_zone_bound_x_0, loading_zone_bound_y_0, 0, loading_zone_bound_x_1, loading_zone_bound_y_1, 0>;
//! @brief The contents of the node storage as tags and associated types.
using store_t = tuple_store<
    loaded_goods,           pallet_content_type,
    loading_goods,          pallet_content_type,
    querying,               query_type,
    new_logs,               std::vector<log_type>,
    coll_logs,              std::vector<log_type>,
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
    logging_delay,          std::vector<times_t>,
    wearable_sim_op,        wearable_sim_state_type,
    wearable_sim_target_pos,vec<dim>,
    pallet_sim_follow,      device_t,
    pallet_sim_follow_pos,  vec<dim>,
    pallet_handled,         bool
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
CONSTANT_DISTRIBUTION(no_goods_distribution, pallet_content_type, coordination::no_content);
CONSTANT_DISTRIBUTION(undefined_goods_distribution, pallet_content_type, coordination::null_content);
CONSTANT_DISTRIBUTION(no_query_distribution, query_type, fcpp::common::make_tagged_tuple<coordination::tags::goods_type>(NO_GOODS));
CONSTANT_DISTRIBUTION(wearables_sim_idle_distribution, wearable_sim_state_type, make_tuple(WEARABLE_IDLE, NO_GOODS, 0));

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
        x,      pallet_rectangle_d,
        led_on, false_distribution,
        node_type, pallet_distribution,
        loaded_goods, no_goods_distribution,
        loading_goods, undefined_goods_distribution,
        querying, no_query_distribution,
        connection_data, distribution::constant_n<real_t, 6, 10>,
        pallet_handled, false_distribution
    >,
    spawn_schedule<wearable_spawn_s>,
    init<
        x,      wearable_rectangle_d,
        led_on, false_distribution,
        node_type, wearable_distribution,
        loaded_goods, no_goods_distribution,
        loading_goods, undefined_goods_distribution,
        querying, no_query_distribution,
        connection_data, distribution::constant_n<real_t, 1>,
        pallet_handled, false_distribution
    >,
    spawn_schedule<empty_pallet_spawn_s>,
    init<
        x, wearable_rectangle_d,
        node_type, pallet_distribution,
        loaded_goods, no_goods_distribution,
        loading_goods, undefined_goods_distribution,
        querying, no_query_distribution,
        connection_data, distribution::constant_n<real_t, 6, 10>,
        pallet_handled, false_distribution
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

#endif // FCPP_WAREHOUSE_SIMULATION_H_
