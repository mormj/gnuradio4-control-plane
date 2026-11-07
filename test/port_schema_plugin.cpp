#include <gnuradio-4.0/BlockModel.hpp>
#include <gnuradio-4.0/Plugin.hpp>

GR_PLUGIN("Port schema fixture", "GNU Radio", "MIT", "1")

struct PluginSchemaProbe : gr::Block<PluginSchemaProbe> {
    std::vector<gr::PortIn<double, gr::Optional, gr::Async>> inputs;
    std::array<gr::PortOut<float>, 2> outputs;
    gr::Annotated<gr::Size_t, "channels", gr::PortCountFor<"inputs">, gr::Limits<0U, 32U>> channels = 0;
    GR_MAKE_REFLECTABLE(PluginSchemaProbe, inputs, outputs, channels);
    gr::work::Status processBulk(auto&&...) { return gr::work::Status::OK; }
};

auto registerSchemaProbe = static_cast<gr::BlockRegistry&>(grPluginInstance()).insert<PluginSchemaProbe>("=PluginSchemaProbe");
