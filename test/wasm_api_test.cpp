#include "gr4cp/api/http_server.hpp"
#include "gr4cp/api/wasm_api.hpp"

#include <regex>
#include <string_view>
#include <thread>

#include <gtest/gtest.h>
#include <httplib.h>
#include <nlohmann/json.hpp>

#include "gr4cp/runtime/stub_runtime_manager.hpp"
#include "gr4cp/storage/in_memory_session_repository.hpp"

namespace {

using Json = nlohmann::json;

class EmptyBlockCatalogProvider final : public gr4cp::catalog::BlockCatalogProvider {
public:
    std::vector<gr4cp::domain::BlockDescriptor> entries;
    std::vector<gr4cp::domain::BlockDescriptor> list() const override { return entries; }
};

class EmptySchedulerCatalogProvider final : public gr4cp::catalog::SchedulerCatalogProvider {
public:
    std::vector<gr4cp::domain::SchedulerDescriptor> list() const override { return {}; }
};

class WasmApiTest : public ::testing::Test {
protected:
    void TearDown() override {
        http_server.stop();
        if (http_thread.joinable()) {
            http_thread.join();
        }
    }

    gr4cp::storage::InMemorySessionRepository repository;
    gr4cp::runtime::StubRuntimeManager runtime;
    EmptyBlockCatalogProvider blocks;
    EmptySchedulerCatalogProvider schedulers;
    gr4cp::app::SessionService sessions{repository, runtime};
    gr4cp::app::SessionStreamService streams;
    gr4cp::app::BlockCatalogService block_catalog{blocks};
    gr4cp::app::SchedulerCatalogService scheduler_catalog{schedulers};
    gr4cp::app::BlockSettingsService settings{sessions, runtime};
    gr4cp::api::WasmApi api{sessions, streams, block_catalog, scheduler_catalog, settings};
    gr4cp::api::HttpServer http_server{sessions, streams, block_catalog, scheduler_catalog, settings};
    std::jthread http_thread;
};

TEST_F(WasmApiTest, CatalogRetainsStructuralPortSchema) {
    gr4cp::domain::BlockDescriptor block;
    block.id = "schema";
    block.name = "Schema";
    gr4cp::domain::BlockPortDescriptor port("bank", "float64");
    port.collection_kind = "fixed";
    port.fixed_extent = 0;
    port.schema_version = 1;
    port.port_kind = "stream";
    port.is_optional = true;
    port.is_synchronous = false;
    block.inputs.push_back(port);
    gr4cp::domain::BlockParameterDescriptor parameter("gain", "float", false, 1.0, "Gain");
    parameter.unit = "V";
    parameter.minimum = 0.0;
    parameter.maximum = 10.0;
    parameter.visible = false;
    block.parameters.push_back(parameter);
    blocks.entries.push_back(block);
    const auto result = api.dispatch("GET", "/blocks/schema", "");
    ASSERT_EQ(result.status, 200);
    const auto input = Json::parse(result.body).at("inputs").at(0);
    const auto parameter_json = Json::parse(result.body).at("parameters").at(0);
    EXPECT_EQ(parameter_json.at("unit"), "V");
    EXPECT_EQ(parameter_json.at("minimum"), 0.0);
    EXPECT_EQ(parameter_json.at("maximum"), 10.0);
    EXPECT_EQ(parameter_json.at("visible"), false);
    EXPECT_EQ(input.at("fixed_extent"), 0);
    EXPECT_EQ(input.at("collection_kind"), "fixed");
    EXPECT_EQ(input.at("schema_version"), 1);
    EXPECT_EQ(input.at("is_optional"), true);
    EXPECT_EQ(input.at("is_synchronous"), false);
}

TEST_F(WasmApiTest, CatalogMetadataMatchesHttpAndPreservesWorkerEnvelope) {
    gr4cp::domain::BlockDescriptor block;
    block.id = "wire";
    block.name = "Wire";
    block.canonical_type = "internal.canonical";
    gr4cp::domain::BlockPortDescriptor fixed("empty", "float32");
    fixed.collection_kind = "fixed";
    fixed.fixed_extent = 0;
    fixed.is_optional = false;
    fixed.is_synchronous = false;
    block.inputs.push_back(fixed);
    gr4cp::domain::BlockPortDescriptor dynamic("unknown", "float32");
    dynamic.cardinality_kind = gr4cp::domain::BlockPortCardinalityKind::Dynamic;
    dynamic.current_port_count = 0;
    dynamic.max_port_count = -1;
    block.outputs.push_back(dynamic);
    gr4cp::domain::BlockParameterDescriptor flag("flag", "bool", false, false, "Flag");
    flag.allow_custom_value = false;
    block.parameters.push_back(flag);
    gr4cp::domain::BlockParameterDescriptor count("count", "int", false, 0, "Count");
    count.enum_choices = std::vector<std::string>{"0", "1"};
    count.enum_labels = {{"0", "Off"}, {"1", "On"}};
    block.parameters.push_back(count);
    blocks.entries.push_back(block);

    const auto response = api.dispatch("GET", "/blocks/wire", "");
    ASSERT_EQ(response.status, 200);
    const auto public_body = Json::parse(response.body);
    const auto port = http_server.bind_to_any_port("127.0.0.1");
    ASSERT_GT(port, 0);
    http_thread = std::jthread([this]() { http_server.listen_after_bind(); });
    httplib::Client client("127.0.0.1", port);
    const auto http = client.Get("/blocks/wire");
    ASSERT_TRUE(http);
    ASSERT_EQ(http->status, 200);
    EXPECT_EQ(Json::parse(http->body), public_body);
    EXPECT_EQ(public_body["inputs"][0]["fixed_extent"], 0);
    EXPECT_EQ(public_body["inputs"][0]["is_optional"], false);
    EXPECT_EQ(public_body["inputs"][0]["is_synchronous"], false);
    EXPECT_FALSE(public_body["inputs"][0].contains("size_parameter"));
    EXPECT_EQ(public_body["outputs"][0]["current_port_count"], 0);
    EXPECT_EQ(public_body["outputs"][0]["max_port_count"], -1);
    EXPECT_FALSE(public_body["outputs"][0].contains("min_port_count"));
    EXPECT_EQ(public_body["parameters"][0]["default"], false);
    EXPECT_EQ(public_body["parameters"][0]["allow_custom_value"], false);
    EXPECT_EQ(public_body["parameters"][1]["default"], 0);
    EXPECT_EQ(public_body["parameters"][1]["enum_labels"]["0"], "Off");
    EXPECT_EQ(public_body["parameters"][1]["enum_choices"], Json::array({"0", "1"}));
    EXPECT_FALSE(public_body.contains("canonical_type"));
    EXPECT_FALSE(public_body["parameters"][0].contains("default_value"));

    const auto worker = gr4cp::domain::block_catalog_worker_json(block);
    EXPECT_EQ(worker["canonical_type"], "internal.canonical");
    EXPECT_EQ(worker["parameters"][0]["default_value"], false);
    EXPECT_EQ(worker["parameters"][1]["default_value"], 0);
    EXPECT_FALSE(worker["parameters"][0].contains("default"));
    EXPECT_EQ(worker["inputs"], public_body["inputs"]);
    EXPECT_EQ(worker["outputs"], public_body["outputs"]);
}

TEST_F(WasmApiTest, HealthIdentityIsAStableUuidAcrossDispatchers) {
    const auto health = api.dispatch("GET", "/healthz", "");
    ASSERT_EQ(health.status, 200);
    const auto body = Json::parse(health.body);
    EXPECT_EQ(body.at("ok"), true);
    const auto identity = body.at("server_instance_id").get<std::string>();
    EXPECT_TRUE(std::regex_match(identity, std::regex{
        "[0-9a-f]{8}-[0-9a-f]{4}-4[0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}"}));
    EXPECT_EQ(api.dispatch("GET", "/healthz", "").body, health.body);

    gr4cp::api::WasmApi another{sessions, streams, block_catalog, scheduler_catalog, settings};
    EXPECT_EQ(another.dispatch("GET", "/healthz", "").body, health.body);
}

TEST_F(WasmApiTest, HealthAndEverySessionResponseShareHttpIdentityAndSerialization) {
    const auto port = http_server.bind_to_any_port("127.0.0.1");
    ASSERT_GT(port, 0);
    http_thread = std::jthread([this]() { http_server.listen_after_bind(); });
    httplib::Client client("127.0.0.1", port);
    const auto http_health = client.Get("/healthz");
    ASSERT_TRUE(http_health);
    ASSERT_EQ(http_health->status, 200);
    const auto health = Json::parse(api.dispatch("GET", "/healthz", "").body);
    EXPECT_EQ(Json::parse(http_health->body), health);

    const auto created = api.dispatch("POST", "/sessions", R"({"name":"identity-check","grc":"graph"})");
    ASSERT_EQ(created.status, 201);
    auto session = Json::parse(created.body);
    EXPECT_FALSE(session.contains("completion_reason"));
    const auto id = session.at("id").get<std::string>();
    const auto path = "/sessions/" + id;
    EXPECT_EQ(session.at("server_instance_id"), health.at("server_instance_id"));

    const auto expect_matching_session = [&]() {
        const auto http_session = client.Get(path);
        ASSERT_TRUE(http_session);
        ASSERT_EQ(http_session->status, 200);
        const auto detail = api.dispatch("GET", path, "");
        ASSERT_EQ(detail.status, 200);
        EXPECT_EQ(Json::parse(detail.body), session);
        EXPECT_EQ(Json::parse(http_session->body), session);
        const auto listed = api.dispatch("GET", "/sessions", "");
        ASSERT_EQ(listed.status, 200);
        EXPECT_EQ(Json::parse(listed.body), Json::array({session}));
    };
    expect_matching_session();

    for (const auto* action : {"start", "stop", "restart"}) {
        const auto response = api.dispatch("POST", path + "/" + action, "");
        ASSERT_EQ(response.status, 200) << response.body;
        session = Json::parse(response.body);
        EXPECT_EQ(session.at("server_instance_id"), health.at("server_instance_id"));
        const bool stopped = std::string_view(action) == "stop";
        EXPECT_EQ(session.contains("completion_reason"), stopped);
        if (stopped) {
            EXPECT_EQ(session.at("completion_reason"), "stopped");
        }
        expect_matching_session();
    }
}

}  // namespace
