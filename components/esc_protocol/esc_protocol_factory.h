#pragma once

#include <memory>
#include <unordered_map>
#include <functional>

#include "base_protocol.h"
#include "oneshot125_protocol.h"

namespace EspFc
{

enum EscProtocolType
{
    UNKNOWN = 0,
    ONESHOT125,
    DSHOT150,
    DSHOT300,
    DSHOT600,
};

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
        protocol_registry_[type] = []() { return std::make_unique<T>(); };
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
    }

    std::unordered_map<EscProtocolType, FactoryFunc> protocol_registry_;
};

}