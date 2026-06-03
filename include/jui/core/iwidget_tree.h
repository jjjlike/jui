#pragma once
#include "geometry.h"
#include "widget_types.h"
#include "jui/core/value.h"
#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace jui::core {

class IWidgetNode {
public:
    virtual ~IWidgetNode() = default;
    virtual const std::string& id() const = 0;
    virtual WidgetType type() const = 0;
    virtual bool visible() const = 0;
    virtual bool enabled() const = 0;
    virtual Rect layoutBounds() const = 0;
    virtual void setLayoutBounds(const Rect& r) = 0;
    virtual const std::vector<std::string>& childIds() const = 0;
    virtual JValue property(const std::string& key) const = 0;
    virtual void setProperty(const std::string& key, const JValue& val) = 0;
    virtual bool hasProperty(const std::string& key) const = 0;
};

class IWidgetTree {
public:
    virtual ~IWidgetTree() = default;
    virtual IWidgetNode* root() = 0;
    virtual void setRoot(const std::string& id) = 0;
    virtual IWidgetNode* get(const std::string& id) = 0;
    virtual void add(std::unique_ptr<IWidgetNode> node) = 0;
    virtual void remove(const std::string& id) = 0;
    virtual void forEach(std::function<void(IWidgetNode&)>) = 0;
    virtual size_t size() const = 0;
};

} // namespace jui::core
