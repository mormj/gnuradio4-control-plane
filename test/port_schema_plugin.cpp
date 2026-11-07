#include <gnuradio-4.0/BlockModel.hpp>
#include <gnuradio-4.0/Plugin.hpp>

GR_PLUGIN("Port schema fixture", "GNU Radio", "MIT", "1")

struct PluginSchemaProbe : gr::Block<PluginSchemaProbe> {
    std::vector<gr::PortIn<double, gr::Optional, gr::Async>> inputs;
    std::array<gr::PortOut<float>, 2> outputs;
    gr::Annotated<gr::Size_t, "Series count", gr::Doc<"Number of input series">, gr::Unit<"channels">, gr::Visible, gr::PortCountFor<"inputs">, gr::Limits<0U, 32U>> channels = 0;
    gr::Annotated<std::uint64_t, "Wide limit", gr::UpdatePolicy<gr::SettingUpdatePolicy::Stopped>, gr::Limits<std::uint64_t{0}, std::numeric_limits<std::uint64_t>::max()>> wide = 0;
    gr::Annotated<float, "Gain", gr::UpdatePolicy<gr::SettingUpdatePolicy::Live>> gain = 1.0F;
    GR_MAKE_REFLECTABLE(PluginSchemaProbe, inputs, outputs, channels, wide, gain);
    gr::work::Status processBulk(auto&&...) { return gr::work::Status::OK; }
};

auto registerSchemaProbe = static_cast<gr::BlockRegistry&>(grPluginInstance()).insert<PluginSchemaProbe>("=PluginSchemaProbe");
