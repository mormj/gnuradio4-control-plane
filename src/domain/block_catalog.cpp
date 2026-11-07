#include "gr4cp/domain/block_catalog.hpp"

#include <nlohmann/json.hpp>

namespace gr4cp::domain {

void to_json(nlohmann::json& json, const VisualizationCapability& capability) {
    json = {{"version", capability.version}, {"family", capability.family},
            {"payload_format", capability.payload_format}, {"routing", capability.routing},
            {"supported_transports", capability.supported_transports},
            {"parameters", capability.parameters}};
    if (!capability.semantic_kind.empty()) json["semantic_kind"] = capability.semantic_kind;
}

void from_json(const nlohmann::json& json, VisualizationCapability& capability) {
    capability.version = json.value("version", 0);
    capability.family = json.value("family", "");
    capability.payload_format = json.value("payload_format", "");
    capability.routing = json.value("routing", "");
    capability.semantic_kind = json.value("semantic_kind", "");
    capability.supported_transports = json.value("supported_transports", std::vector<std::string>{});
    capability.parameters = json.value("parameters", std::map<std::string, std::string>{});
}

using Json = nlohmann::json;

Json block_parameter_default_to_json(const BlockParameterDefault& value) {
    return std::visit([](const auto& item) -> Json { return item; }, value);
}

void to_json(Json& result, const BlockPortDescriptor& port) {
    auto body = Json{
        {"name", port.name},
        {"type", port.type},
        {"cardinality_kind", port.cardinality_kind == BlockPortCardinalityKind::Dynamic ? "dynamic" : "fixed"},
    };

    if (port.collection_kind.has_value()) {
        body["collection_kind"] = *port.collection_kind;
    }
    if (port.fixed_extent.has_value()) {
        body["fixed_extent"] = *port.fixed_extent;
    }
    if (port.port_kind.has_value()) {
        body["port_kind"] = *port.port_kind;
    }
    if (port.is_optional.has_value()) {
        body["is_optional"] = *port.is_optional;
    }
    if (port.is_synchronous.has_value()) {
        body["is_synchronous"] = *port.is_synchronous;
    }
    if (port.schema_version.has_value()) {
        body["schema_version"] = *port.schema_version;
    }
    if (port.current_port_count.has_value()) {
        body["current_port_count"] = *port.current_port_count;
    }
    if (port.render_port_count.has_value()) {
        body["render_port_count"] = *port.render_port_count;
    }
    if (port.min_port_count.has_value()) {
        body["min_port_count"] = *port.min_port_count;
    }
    if (port.max_port_count.has_value()) {
        body["max_port_count"] = *port.max_port_count;
    }
    if (port.size_parameter.has_value()) {
        body["size_parameter"] = *port.size_parameter;
    }
    if (port.handle_name_template.has_value()) {
        body["handle_name_template"] = *port.handle_name_template;
    }

    result = std::move(body);
}

void to_json(Json& result, const BlockParameterDescriptor& parameter) {
    auto body = Json{
        {"name", parameter.name},
        {"type", parameter.type},
        {"required", parameter.required},
        {"default", block_parameter_default_to_json(parameter.default_value)},
        {"summary", parameter.summary},
    };

    if (parameter.runtime_mutability.has_value()) {
        body["runtime_mutability"] = *parameter.runtime_mutability;
    }
    if (parameter.value_kind.has_value()) {
        body["value_kind"] = *parameter.value_kind;
    }
    if (parameter.enum_choices.has_value()) {
        body["enum_choices"] = *parameter.enum_choices;
    }
    if (parameter.enum_type.has_value()) {
        body["enum_type"] = *parameter.enum_type;
    }
    if (!parameter.enum_labels.empty()) {
        body["enum_labels"] = parameter.enum_labels;
    }
    if (parameter.enum_source.has_value()) {
        body["enum_source"] = *parameter.enum_source;
    }
    if (parameter.ui_hint.has_value()) {
        body["ui_hint"] = *parameter.ui_hint;
    }
    if (parameter.allow_custom_value.has_value()) {
        body["allow_custom_value"] = *parameter.allow_custom_value;
    }

    if (parameter.label.has_value()) body["label"] = *parameter.label;
    if (parameter.unit.has_value()) body["unit"] = *parameter.unit;
    if (parameter.visible.has_value()) body["visible"] = *parameter.visible;
    if (parameter.minimum.has_value()) body["minimum"] = *parameter.minimum;
    if (parameter.maximum.has_value()) body["maximum"] = *parameter.maximum;
    result = std::move(body);
}

void to_json(Json& result, const BlockDescriptor& block) {
    result = Json{{"id", block.id}, {"name", block.name}, {"category", block.category},
                  {"summary", block.summary}, {"inputs", block.inputs},
                  {"outputs", block.outputs}, {"parameters", block.parameters}};
    if (block.visualization) result["capabilities"]["studio:visualization"] = *block.visualization;
}

nlohmann::json block_catalog_worker_json(const BlockDescriptor& block) {
    Json result = block;
    for (auto& parameter : result["parameters"]) {
        parameter["default_value"] = std::move(parameter["default"]);
        parameter.erase("default");
    }
    if (block.canonical_type) result["canonical_type"] = *block.canonical_type;
    return result;
}



bool operator==(const BlockPortDescriptor& left, const BlockPortDescriptor& right) {
    return left.collection_kind == right.collection_kind && left.fixed_extent == right.fixed_extent &&
           left.port_kind == right.port_kind && left.is_optional == right.is_optional &&
           left.is_synchronous == right.is_synchronous && left.schema_version == right.schema_version &&
           left.name == right.name && left.type == right.type && left.cardinality_kind == right.cardinality_kind &&
           left.current_port_count == right.current_port_count && left.min_port_count == right.min_port_count &&
           left.render_port_count == right.render_port_count && left.max_port_count == right.max_port_count &&
           left.size_parameter == right.size_parameter &&
           left.handle_name_template == right.handle_name_template;
}

bool operator==(const BlockParameterDescriptor& left, const BlockParameterDescriptor& right) {
    return left.name == right.name && left.type == right.type && left.required == right.required &&
           left.default_value == right.default_value && left.summary == right.summary &&
           left.label == right.label &&
           left.unit == right.unit &&
           left.visible == right.visible &&
           left.minimum == right.minimum &&
           left.maximum == right.maximum &&
           left.runtime_mutability == right.runtime_mutability && left.value_kind == right.value_kind &&
           left.enum_choices == right.enum_choices && left.enum_type == right.enum_type &&
           left.enum_labels == right.enum_labels && left.enum_source == right.enum_source &&
           left.ui_hint == right.ui_hint && left.allow_custom_value == right.allow_custom_value;
}

bool operator==(const BlockDescriptor& left, const BlockDescriptor& right) {
    return left.id == right.id && left.canonical_type == right.canonical_type && left.name == right.name && left.category == right.category &&
           left.summary == right.summary && left.inputs == right.inputs && left.outputs == right.outputs &&
           left.parameters == right.parameters && left.visualization == right.visualization;
}

}  // namespace gr4cp::domain
