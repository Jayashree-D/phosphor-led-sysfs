#pragma once

#include <sdbusplus/bus.hpp>

#include <iostream>

namespace phosphor
{
namespace led
{

class Controller
{
  public:
    Controller() = delete;
    ~Controller() = default;

    Controller(sdbusplus::bus::bus& bus) : bus(bus)
    {
        std::cerr << " In controller \n";
        createLEDPath(bus);
    }

  private:
    sdbusplus::bus::bus& bus;

    void ledEventHandle(std::vector<std::string>& name);

    void createLEDPath(sdbusplus::bus::bus& bus);
};

} // namespace led
} // namespace phosphor
