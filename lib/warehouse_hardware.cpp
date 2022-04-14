#include "warehouse_hardware.hpp"

using namespace fcpp;
using namespace fcpp::option;
using namespace component::tags;

static rows_t row_store;

namespace fcpp::component {

template <class... Ts>
struct log_dumper {

    //! @brief Type of the output stream.
    using ostream_type = common::option_type<tags::ostream_type, fcpp::common::print_stream, Ts ...>;

    template <typename F, typename P>
    struct component : public P {
        DECLARE_COMPONENT(log_dumper);
        REQUIRE_COMPONENT(log_dumper, timer);

        class node : public P::node {
            public:
            template <typename S, typename T>
            node(typename F::net& n, common::tagged_tuple<S,T> const& t) : P::node(n,t), m_stream(details::make_stream(common::get_or<tags::output>(t, &std::cout), t)) {}

            void terminate() {
                P::node::terminate();
                // force full log
                SEGGER_RTT_SetFlagsUpBuffer(0, SEGGER_RTT_MODE_BLOCK_IF_FIFO_FULL);
                *m_stream << "----" << std::endl << "log size " << row_store.byte_size() << std::endl;
                row_store.print(*m_stream);
            }

            //! @brief The stream where data is exported.
            std::shared_ptr<ostream_type> m_stream;
        };
    };
};

DECLARE_COMBINE(dwm1001_deployment, log_dumper, hardware_logger, storage, hardware_connector, timer, scheduler, hardware_identifier, randomizer, calculus);

} // namespace fcpp::component

using component_type = component::dwm1001_deployment<fcpp::option::list>;

static os::dwm1001_network::data_type driver_settings("DWM", -12, 3, 2, 0.1 * CLOCK_SECOND);

static auto input_tuple = common::make_tagged_tuple<plotter, loaded_goods, loading_goods, querying, connection_data>(
    &row_store, coordination::no_content, coordination::null_content, 
    fcpp::common::make_tagged_tuple<coordination::tags::goods_type>(NO_GOODS), driver_settings);

FCPP_CONTIKI(component_type, input_tuple)
