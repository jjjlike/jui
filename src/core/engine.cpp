#include "jui/core/engine.h"
#include <json.hpp>
#include <sstream>
#include <fstream>

namespace jui {

using json = nlohmann::json;

// ============================================================
// EngineImpl — 内部实现（隐藏 nlohmann/json 依赖）
// ============================================================
struct EngineImpl {
    void processMessage(const json& msg, JUIEngine& engine);

    void handleCreateSurface(const json& msg, JUIEngine& engine);
    void handleUpdateComponents(const json& msg, JUIEngine& engine);
    void handleUpdateDataModel(const json& msg, JUIEngine& engine);
    void handleBeginRendering(const json& msg, JUIEngine& engine);
    void handleDeleteSurface(const json& msg, JUIEngine& engine);

    WidgetPtr parseWidget(const json& componentJson);
    std::vector<WidgetPtr> parseComponents(const json& componentsArray);
    JValue parseValue(const json& jv);
    JValue parseBoundValue(const json& jv);
    void bindDataToWidget(WidgetPtr widget, const DataModel& model);
    void refreshLayout(SurfacePtr surface, D2DRenderer& renderer, LayoutEngine& layoutEngine);
};

// ============================================================
// JUIEngine 实现
// ============================================================

JUIEngine::JUIEngine() : impl_(std::make_unique<EngineImpl>()) {}
JUIEngine::~JUIEngine() = default;

bool JUIEngine::initialize(HWND hwnd) {
    return renderer_.initialize(hwnd);
}

void JUIEngine::processMessage(const std::string& jsonStr) {
    try {
        json msg = json::parse(jsonStr);
        impl_->processMessage(msg, *this);
    } catch (const std::exception& e) {
        // 解析失败时输出错误，便于排查
        OutputDebugStringA(("JUI processMessage parse error: " + std::string(e.what()) + "\n").c_str());
    }
}

void JUIEngine::processMessageStream(const std::string& jsonlStream) {
    std::istringstream stream(jsonlStream);
    std::string line;
    while (std::getline(stream, line)) {
        if (line.empty()) continue;
        if (line.find_first_not_of(" \t\r\n") == std::string::npos) continue;
        processMessage(line);
    }
}

bool JUIEngine::loadFromFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) return false;
    try {
        json doc = json::parse(file);
        impl_->processMessage(doc, *this);
        return true;
    } catch (...) {
        return false;
    }
}

// ============================================================
// EngineImpl::processMessage — A2UI 消息分发
// ============================================================

void EngineImpl::processMessage(const json& msg, JUIEngine& engine) {
    try {
        if (msg.contains("createSurface")) {
            handleCreateSurface(msg["createSurface"], engine);
            return;
        }
        if (msg.contains("updateComponents")) {
            handleUpdateComponents(msg["updateComponents"], engine);
            return;
        }
        if (msg.contains("surfaceUpdate")) {
            handleUpdateComponents(msg["surfaceUpdate"], engine);
            return;
        }
        if (msg.contains("updateDataModel")) {
            handleUpdateDataModel(msg["updateDataModel"], engine);
            return;
        }
        if (msg.contains("dataModelUpdate")) {
            handleUpdateDataModel(msg["dataModelUpdate"], engine);
            return;
        }
        if (msg.contains("beginRendering")) {
            handleBeginRendering(msg["beginRendering"], engine);
            return;
        }
        if (msg.contains("deleteSurface")) {
            handleDeleteSurface(msg["deleteSurface"], engine);
            return;
        }
    } catch (const std::exception&) {
        // 消息处理失败
    }
}

// ============================================================
// EngineImpl A2UI 消息处理实现
// ============================================================

void EngineImpl::handleCreateSurface(const json& msg, JUIEngine& engine) {
    std::string surfaceId = msg.value("surfaceId", "main");
    auto surface = std::make_shared<Surface>(surfaceId);
    engine.surfaces_[surfaceId] = surface;
    engine.currentSurfaceId_ = surfaceId;
    engine.renderer_.setSurface(surface);
}

void EngineImpl::handleUpdateComponents(const json& msg, JUIEngine& engine) {
    std::string surfaceId = msg.value("surfaceId", engine.currentSurfaceId_);
    auto surface = engine.getSurface(surfaceId);
    if (!surface) {
        surface = engine.createSurface(surfaceId);
    }
    if (msg.contains("components")) {
        auto components = parseComponents(msg["components"]);
        surface->updateComponents(components);
    }
}

void EngineImpl::handleUpdateDataModel(const json& msg, JUIEngine& engine) {
    std::string surfaceId = msg.value("surfaceId", engine.currentSurfaceId_);
    auto surface = engine.getSurface(surfaceId);
    if (!surface) return;

    std::string path = msg.value("path", "");
    if (msg.contains("contents") && msg["contents"].is_array()) {
        std::vector<std::pair<std::string, JValue>> contents;
        for (auto& item : msg["contents"]) {
            std::string key = item.value("key", "");
            JValue val;
            if (item.contains("valueString")) val = JValue(item["valueString"].get<std::string>());
            else if (item.contains("valueNumber")) val = JValue(item["valueNumber"].get<double>());
            else if (item.contains("valueBoolean")) val = JValue(item["valueBoolean"].get<bool>());
            else if (item.contains("valueMap")) {
                std::map<std::string, JValue> obj;
                for (auto& [k, v] : item["valueMap"].items()) {
                    if (v.is_string()) obj[k] = JValue(v.get<std::string>());
                    else if (v.is_number()) obj[k] = JValue(v.get<double>());
                    else if (v.is_boolean()) obj[k] = JValue(v.get<bool>());
                }
                val = JValue::fromObject(obj);
            }
            contents.push_back({key, val});
        }
        surface->dataModel().updateContents(path, contents);
    }
    surface->setNeedsLayout(true);
}

void EngineImpl::handleBeginRendering(const json& msg, JUIEngine& engine) {
    std::string surfaceId = msg.value("surfaceId", engine.currentSurfaceId_);
    auto surface = engine.getSurface(surfaceId);
    if (!surface) return;

    if (msg.contains("root")) {
        surface->setRootWidgetId(msg["root"].get<std::string>());
    }

    refreshLayout(surface, engine.renderer_, engine.layoutEngine_);

    engine.renderer_.setSurface(surface);
    engine.renderer_.invalidate();
}

void EngineImpl::handleDeleteSurface(const json& msg, JUIEngine& engine) {
    std::string surfaceId = msg.value("surfaceId", "");
    if (surfaceId.empty()) return;
    engine.surfaces_.erase(surfaceId);
    if (engine.currentSurfaceId_ == surfaceId) {
        engine.currentSurfaceId_.clear();
        engine.renderer_.setSurface(nullptr);
    }
}

// ============================================================
// EngineImpl JSON → Widget 解析
// ============================================================

JValue EngineImpl::parseValue(const json& jv) {
    if (jv.is_null()) return JValue();
    if (jv.is_string()) return JValue(jv.get<std::string>());
    if (jv.is_number_float()) return JValue(jv.get<double>());
    if (jv.is_number_integer()) return JValue(static_cast<int>(jv.get<int>()));
    if (jv.is_boolean()) return JValue(jv.get<bool>());
    if (jv.is_array()) {
        JValue arr = JValue::fromArray({});
        for (auto& item : jv) arr.push(parseValue(item));
        return arr;
    }
    if (jv.is_object()) {
        JValue obj = JValue::fromObject({});
        for (auto& [k, v] : jv.items()) obj.set(k, parseValue(v));
        return obj;
    }
    return JValue();
}

JValue EngineImpl::parseBoundValue(const json& jv) {
    if (jv.is_string()) return JValue(jv.get<std::string>());
    if (jv.is_number()) return JValue(jv.get<double>());
    if (jv.is_boolean()) return JValue(jv.get<bool>());
    if (jv.contains("literalString")) return JValue(jv["literalString"].get<std::string>());
    if (jv.contains("literalNumber")) return JValue(jv["literalNumber"].get<double>());
    if (jv.contains("literalBoolean")) return JValue(jv["literalBoolean"].get<bool>());
    if (jv.contains("path")) return JValue::fromPath(jv["path"].get<std::string>());
    return JValue();
}

WidgetPtr EngineImpl::parseWidget(const json& componentJson) {
    std::string id = componentJson.value("id", "");
    if (id.empty()) return nullptr;

    std::string componentType;
    if (componentJson.contains("component") && componentJson["component"].is_string()) {
        componentType = componentJson["component"].get<std::string>();
    } else if (componentJson.contains("component") && componentJson["component"].is_object()) {
        auto& compObj = componentJson["component"];
        if (compObj.size() > 0) {
            componentType = compObj.items().begin().key();
        }
    }

    static std::map<std::string, WidgetType> typeMap = {
        {"Text", WidgetType::Text},
        {"Button", WidgetType::Button},
        {"TextField", WidgetType::TextField},
        {"CheckBox", WidgetType::CheckBox},
        {"Checkbox", WidgetType::CheckBox},
        {"Toggle", WidgetType::Toggle},
        {"ChoicePicker", WidgetType::ChoicePicker},
        {"Choicepicker", WidgetType::ChoicePicker},
        {"Slider", WidgetType::Slider},
        {"List", WidgetType::List},
        {"Grid", WidgetType::Grid},
        {"Tabs", WidgetType::Tabs},
        {"Image", WidgetType::Image},
        {"Row", WidgetType::Row},
        {"Column", WidgetType::Column},
        {"Card", WidgetType::Card},
        {"List", WidgetType::List},
        {"Divider", WidgetType::Divider},
    };

    WidgetType wType = WidgetType::Custom;
    auto it = typeMap.find(componentType);
    if (it != typeMap.end()) wType = it->second;
    else return nullptr;

    auto widget = std::make_shared<Widget>(id, wType);

    const json* props = &componentJson;
    json v08props;
    if (componentJson.contains("component") && componentJson["component"].is_object()) {
        auto& compObj = componentJson["component"];
        if (compObj.size() > 0) {
            v08props = compObj.items().begin().value();
            props = &v08props;
        }
    }

    for (auto& [key, val] : props->items()) {
        if (key == "id" || key == "component" || key == "children") continue;
        if (val.is_object() && (val.contains("literalString") ||
            val.contains("literalNumber") || val.contains("literalBoolean") ||
            val.contains("path"))) {
            widget->setProperty(key, parseBoundValue(val));
            widget->setBoundValue(key, parseBoundValue(val));
        } else {
            widget->setProperty(key, parseValue(val));
        }
    }

    if (props->contains("children")) {
        auto& children = (*props)["children"];
        Children c;
        if (children.contains("explicitList")) {
            c.mode = Children::Explicit;
            for (auto& child : children["explicitList"]) {
                c.explicitList.push_back(child.get<std::string>());
            }
        } else if (children.contains("template")) {
            c.mode = Children::Template;
            c.templateInfo.dataBinding = children["template"].value("dataBinding", "");
            c.templateInfo.componentId = children["template"].value("componentId", "");
        }
        widget->setChildren(c);
    }

    if (props->contains("action")) {
        auto& act = (*props)["action"];
        if (act.contains("name")) {
            widget->setAction(act["name"].get<std::string>());
        } else if (act.is_string()) {
            widget->setAction(act.get<std::string>());
        }
    }

    return widget;
}

std::vector<WidgetPtr> EngineImpl::parseComponents(const json& componentsArray) {
    std::vector<WidgetPtr> result;
    if (!componentsArray.is_array()) return result;
    for (auto& compJson : componentsArray) {
        auto widget = parseWidget(compJson);
        if (widget) result.push_back(widget);
    }
    return result;
}

// ============================================================
// EngineImpl 数据绑定与布局
// ============================================================

void EngineImpl::bindDataToWidget(WidgetPtr widget, const DataModel& model) {
    if (!widget) return;
    for (auto& [key, val] : widget->properties()) {
        if (val.isPath()) {
            auto bound = model.getValue(val.asString());
            if (!bound.isNull()) {
                widget->setProperty(key, bound);
            }
        }
    }
}

void EngineImpl::refreshLayout(SurfacePtr surface, D2DRenderer& renderer, LayoutEngine& layoutEngine) {
    if (!surface) return;

    for (auto& [id, widget] : surface->widgets()) {
        bindDataToWidget(widget, surface->dataModel());
    }

    auto root = surface->rootWidget();
    if (root) {
        Size containerSize = {
            static_cast<float>(renderer.width()),
            static_cast<float>(renderer.height())
        };
        if (containerSize.w <= 0) containerSize.w = 800;
        if (containerSize.h <= 0) containerSize.h = 600;

        // 传入 Surface 的 widget 查找器，使 LayoutEngine 能解析子组件 ID
        auto resolver = [&surface](const std::string& id) -> WidgetPtr {
            return surface->getWidget(id);
        };
        layoutEngine.layout(root, containerSize, resolver);
    }
    surface->setNeedsLayout(false);
}

// ============================================================
// JUIEngine 方法转发
// ============================================================

SurfacePtr JUIEngine::getSurface(const std::string& id) {
    auto it = surfaces_.find(id);
    return it != surfaces_.end() ? it->second : nullptr;
}

SurfacePtr JUIEngine::createSurface(const std::string& id) {
    auto surface = std::make_shared<Surface>(id);
    surfaces_[id] = surface;
    currentSurfaceId_ = id;
    return surface;
}

void JUIEngine::render() {
    if (!currentSurfaceId_.empty()) {
        auto surface = getSurface(currentSurfaceId_);
        if (surface && surface->needsLayout()) {
            impl_->refreshLayout(surface, renderer_, layoutEngine_);
        }
    }
    renderer_.render();
}

void JUIEngine::onSize(int w, int h)     { renderer_.onSize(w, h); }
void JUIEngine::onMouseMove(int x, int y)   { renderer_.onMouseMove(x, y); }
void JUIEngine::onMouseDown(int x, int y, int b) { renderer_.onMouseDown(x, y, b); }
void JUIEngine::onMouseUp(int x, int y, int b)   { renderer_.onMouseUp(x, y, b); }
void JUIEngine::onCharInput(uint32_t ch)  { renderer_.onCharInput(ch); }
void JUIEngine::onKeyDown(int vk)         { renderer_.onKeyDown(vk); }
void JUIEngine::onKeyUp(int vk)           { renderer_.onKeyUp(vk); }
void JUIEngine::onMouseWheel(float d)     { renderer_.onMouseWheel(d); }
void JUIEngine::onIMEStart()              { renderer_.onIMEStart(); }
void JUIEngine::onIMEComposition(const std::string& s) { renderer_.onIMEComposition(s); }
void JUIEngine::onIMEEnd(const std::string& r)         { renderer_.onIMEEnd(r); }

} // namespace jui
