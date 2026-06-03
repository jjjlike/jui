#pragma once
#include "jui/core/value.h"
#include <string>

namespace jui::core {

class IDataModel {
public:
    virtual ~IDataModel() = default;
    virtual void setValue(const std::string& path, const JValue& value) = 0;
    virtual JValue getValue(const std::string& path) const = 0;
    virtual bool hasValue(const std::string& path) const = 0;
    virtual void clear() = 0;
};

} // namespace jui::core
