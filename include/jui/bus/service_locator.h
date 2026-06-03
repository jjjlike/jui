#pragma once
#include <memory>
#include <map>
#include <typeindex>
#include <stdexcept>
#include <string>

namespace jui::bus {

class ServiceLocator {
public:
    template<typename Interface, typename Impl>
    static void bind(std::shared_ptr<Impl> instance) {
        instances()[std::type_index(typeid(Interface))] =
            std::static_pointer_cast<void>(instance);
    }

    template<typename Interface>
    static std::shared_ptr<Interface> resolve() {
        auto it = instances().find(std::type_index(typeid(Interface)));
        if (it == instances().end())
            throw std::runtime_error(std::string("Service not registered: ") +
                                     typeid(Interface).name());
        return std::static_pointer_cast<Interface>(it->second);
    }

    template<typename Interface>
    static bool has() {
        return instances().count(std::type_index(typeid(Interface))) > 0;
    }

    static void clear() { instances().clear(); }

private:
    static std::map<std::type_index, std::shared_ptr<void>>& instances() {
        static std::map<std::type_index, std::shared_ptr<void>> inst;
        return inst;
    }
};

} // namespace jui::bus
