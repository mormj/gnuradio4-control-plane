#include <gnuradio-4.0/BlockModel.hpp>
#include <gnuradio-4.0/Plugin.hpp>

GR_PLUGIN("Port schema fixture", "GNU Radio", "MIT", "1")

struct ProbeVisualization {
    static void exportMetadata(auto&& emit) {
        emit("studio:visualization", gr::property_map{{"version", std::uint32_t(1)},
            {"family", "series"}, {"payload_format", "series-window-json-v1"}, {"routing", "session"},
            {"supported_transports", std::vector<std::string>{"http_poll"}},
            {"parameters", gr::property_map{{"transport", "transport"}, {"endpoint", "endpoint"}, {"seriesCount", "channels"}}}});
    }
};

struct PluginSchemaProbe : gr::Block<PluginSchemaProbe> {
    using BlockMetadata = gr::BlockMetadata<ProbeVisualization>;
    std::string transport = "http_poll";
    std::string endpoint = "http://127.0.0.1:18080/snapshot";
    std::vector<gr::PortIn<double, gr::Optional, gr::Async>> inputs;
    std::array<gr::PortOut<float>, 2> outputs;
    gr::Annotated<gr::Size_t, "Series count", gr::Doc<"Number of input series">, gr::Unit<"channels">, gr::Visible, gr::PortCountFor<"inputs">, gr::Limits<0U, 32U>> channels = 0;
    gr::Annotated<std::uint64_t, "Wide limit", gr::Limits<std::uint64_t{0}, std::numeric_limits<std::uint64_t>::max()>> wide = 0;
    GR_MAKE_REFLECTABLE(PluginSchemaProbe, inputs, outputs, channels, wide, transport, endpoint);
    gr::work::Status processBulk(auto&&...) { return gr::work::Status::OK; }
};

auto registerSchemaProbe = static_cast<gr::BlockRegistry&>(grPluginInstance()).insert<PluginSchemaProbe>("=PluginSchemaProbe");
