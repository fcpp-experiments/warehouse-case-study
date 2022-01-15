// Copyright © 2022 Giorgio Audrito and Lorenzo Testa. All Rights Reserved.

/**
 * @file warehouse_hardware.hpp
 * @brief Case study on smart warehouse management (deployment-specific code).
 */
#ifndef FCPP_WAREHOUSE_HARDWARE_H_
#define FCPP_WAREHOUSE_HARDWARE_H_

#include "warehouse.hpp"
#include "fcpp-contiki-api.hpp"
#include "dwm1001_hardware_api.h"
#include "SEGGER_RTT.h"

//! @brief Time in seconds between transmission rounds.
constexpr size_t round_period = 1;

//! @brief Reference size for the grid disposition in aisles (cm).
constexpr size_t grid_cell_size = 150;

//! @brief Communication radius (cm).
constexpr size_t comm = 2500;

//! @brief Print operator for warehouse device type.
template<typename O>
O& operator<<(O& o, warehouse_device_type const& t) {
    return o << to_string(t);
}

namespace fcpp {

namespace coordination {

/**
 * DEPLOYMENT PLAN:
 *
 * - long button press terminates, short button press interacts
 * - desk 1 representing "loading zone", with empty pallets
 * - desk 2 representing an "aisle", with full pallets
 * - one device is a wearable, the rest are pallets
 * - every device is set up as empty (use buttons on pallets for initial setup)
 * - use button on idle wearable to make it do something random
 * - handled pallet has flashing light (use button to reset it)
 * - querying wearable has flashing lights (turns off on load)
 */
MAIN() {
    // primitive check of button status
    bool button = button_is_pressed();
    // detect a short press
    bool button_pressed = old(CALL, not button) and button;
    // terminate on 5s long press
    if (time_since(CALL, not button) >= 5) node.terminate();
    // set up node type
    bool is_pallet = node.uid != WEARABLE_ID;
    node.storage(tags::node_type{}) = is_pallet ? warehouse_device_type::Pallet : warehouse_device_type::Wearable;
    // effect of button on pallets
    if (is_pallet and button_pressed) {
        if (node.storage(tags::pallet_handled{})) {
            // resetting handling status
            node.storage(tags::pallet_handled{}) = false;
        } else {
            // increasing content (for initial setup): NO_GOODS/0/1/2...
            uint8_t& loaded  = get<tags::goods_type>(node.storage(tags::loaded_goods{}));
            loaded = (loaded+1) % 256;
        }
    }
    // on wearables, button triggers an action on pallets
    if (not is_pallet and button_pressed) {
        // might be equally loading or unloading
        bool is_loading = node.next_int(1);
        if (is_loading) {
            // loading random good between 0 and 2
            node.storage(tags::loading_goods{}) = node.next_int(2);
            node.storage(tags::querying{}) = no_query;
            // experimenter should move it close to an empty pallet,
            // then with it to a space following led lights, and back
        } else {
            // querying for a random good between 0 and 2
            node.storage(tags::loading_goods{}) = null_content;
            node.storage(tags::querying{}) = node.next_int(2);
            // experimenter should follow led lights to find pallet to unload,
            // then back with it to loading zone
        }
    }
    // calls main warehouse app
    device_t waypoint = warehouse_app(CALL, grid_cell_size, 2000, 0, 0); // TODO: tweak numbers
    // checking if querying wearables has found its pallet
    if (not is_pallet and node.storage(tags::querying{}) != no_query) {
        if (waypoint != node.uid and details::self(node.nbr_dist(), waypoint) < 0.5*grid_cell_size) {
            // resetting query and unloading good
            node.storage(tags::loading_goods{}) = no_content;
            node.storage(tags::querying{}) = no_query;
        }
    }
    // add flashing lights to handled pallets and querying wearables
    if (int(node.current_time()) % 2 == 0)
        if (node.storage(tags::pallet_handled{}) or node.storage(tags::querying{}) != no_query)
            node.storage(tags::led_on{}) = true;
    // physically turn on led if necessary
    set_led(node.storage(tags::led_on{}));
}
FUN_EXPORT main_t = export_list<bool, time_since_t, warehouse_app_t>;

} // namespace coordination

//! @brief Namespace for component options.
namespace option {

//! @brief Dictates that rounds are happening every 1 seconds (den, start, period).
using schedule_t = round_schedule<sequence::periodic_n<1, round_period, round_period>>;

//! @brief Data to be stored in pallets for later printing.
using rows_t = plot::rows<
    tuple_store<
        loaded_goods,       pallet_content_type,
        loading_goods,      pallet_content_type,
        querying,           query_type,
        led_on,             bool,
        pallet_handled,     bool,
        new_logs,           std::vector<log_type>,
        msg_size,           uint8_t
    >,
    tuple_store<
        global_clock,       times_t
    >,
    tuple_store<
        node_type,          warehouse_device_type
    >,
    1024*10
>;

//! @brief The general hardware options.
DECLARE_OPTIONS(list,
    general,
    base_fcpp_contiki_opt,
    program<coordination::main>,
    exports<coordination::main_t>,
    schedule_t,
    plot_type<rows_t>
);

} // namespace option


} // namespace fcpp

#endif // FCPP_WAREHOUSE_HARDWARE_H_
