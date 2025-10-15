#pragma once

#include <memory>
#include <unordered_map>
#include <functional>

#include "base_protocol.h"
#include "oneshot125_protocol.h"
#include "dshot_protocol.h"

namespace EspFc
{

class EscProtocolFactory
{
public:
    using FactoryFunc = std::function<std::unique_ptr<BaseProtocol>()>;

    static EscProtocolFactory& instance()
    {
        static EscProtocolFactory factory;
        return factory;
    }

    template <typename T>
    void registerType(EscProtocolType type)
    {
        protocol_registry_[type] = [type]() { return std::make_unique<T>(type); };
    }

    std::unique_ptr<BaseProtocol> create(EscProtocolType type)
    {
        auto it = protocol_registry_.find(type);
        if (it != protocol_registry_.end())
            return it->second();
        return nullptr;
    }

private:
    EscProtocolFactory()
    {
        registerType<OneShot125Protocol>(ONESHOT125);
        registerType<DShotProtocol>(DSHOT150);
        registerType<DShotProtocol>(DSHOT300);
        registerType<DShotProtocol>(DSHOT600);
    }

    std::unordered_map<EscProtocolType, FactoryFunc> protocol_registry_;
};

}