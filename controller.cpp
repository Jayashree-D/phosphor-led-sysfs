/**
 * Copyright © 2016 IBM Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "controller.hpp"

#include "argument.hpp"
#include "physical.hpp"
#include "sysfs.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/container/flat_map.hpp>
#include <sdeventplus/event.hpp>

#include <algorithm>
#include <iostream>
#include <string>

static constexpr auto busName = "xyz.openbmc_project.LED.Controller";
static constexpr auto objectPath = "/xyz/openbmc_project/led/physical";
static constexpr auto devPath = "/sys/class/leds/";

boost::container::flat_map<std::string,
                           std::unique_ptr<phosphor::led::Physical>>
    leds;

struct LedDescr
{
    std::string devicename;
    std::string color;
    std::string function;
};

/** @brief parse LED name in sysfs
 *  Parse sysfs LED name in format "devicename:colour:function"
 *  or "devicename:colour" or "devicename" and sets corresponding
 *  fields in LedDescr struct.
 *
 *  @param[in] name      - LED name in sysfs
 *  @param[out] ledDescr - LED description
 */
void getLedDescr(const std::string& name, LedDescr& ledDescr)
{
    std::vector<std::string> words;
    boost::split(words, name, boost::is_any_of(":"));
    try
    {
        ledDescr.devicename = words.at(0);
        ledDescr.color = words.at(1);
        ledDescr.function = words.at(2);
    }
    catch (const std::out_of_range&)
    {
        return;
    }
}

/** @brief generates LED DBus name from LED description
 *
 *  @param[in] name      - LED description
 *  @return              - DBus LED name
 */
std::string getDbusName(const LedDescr& ledDescr)
{
    std::vector<std::string> words;
    if (!ledDescr.function.empty())
        words.emplace_back(ledDescr.function);
    if (!ledDescr.color.empty())
        words.emplace_back(ledDescr.color);
    if (words.empty())
        words.emplace_back(ledDescr.devicename);
    return boost::join(words, "_");
}

namespace phosphor
{
namespace led
{

void Controller::ledEventHandle(std::vector<std::string>& names)
{
    std::vector<std::string> ledName = {
        "group1:blue:power1",    "group2:blue:power2",    "group3:blue:power3",
        "group4:blue:power4",    "group1:yellow:system1", "group2",
        "group3:yellow:system3", "group4:yellow:system4"};

    names = ledName;
}

// void Controller::ledEventHandle()
void Controller::createLEDPath(sdbusplus::bus::bus& bus)
{
    std::cerr << " In led event handle \n";

    namespace fs = std::filesystem;

    std::vector<std::string> ledNames;
    ledEventHandle(ledNames);

    for (auto name : ledNames)
    {
        // LED names may have a hyphen and that would be an issue for
        // dbus paths and hence need to convert them to underscores.
        std::replace(name.begin(), name.end(), '/', '-');
        auto path = devPath + name;

        // Convert to lowercase just in case some are not and that
        // we follow lowercase all over
        std::transform(name.begin(), name.end(), name.begin(), ::tolower);

        // LED names may have a hyphen and that would be an issue for
        // dbus paths and hence need to convert them to underscores.
        std::replace(name.begin(), name.end(), '-', '_');

        // Convert LED name in sysfs into DBus name
        LedDescr ledDescr;
        getLedDescr(name, ledDescr);
        name = getDbusName(ledDescr);

        // Unique path name representing a single LED.
        auto objPath =
            std::string(objectPath) + '/' + ledDescr.devicename + '/' + name;

        // Create the Physical LED objects for directing actions.
        // Need to save this else sdbusplus destructor will wipe this off.
        phosphor::led::SysfsLed sled{fs::path(path)};

        std::cerr << objPath << " \n";
        std::cerr << ledDescr.color << "\n";

        auto& obj = leds[objPath];
        obj = std::make_unique<phosphor::led::Physical>(bus, objPath, sled,
                                                        ledDescr.color);
    }
    std::cerr << "End \n";
}
} // namespace led
} // namespace phosphor

int main()
{
    std::cerr << " In main \n";
    // Get a default event loop
    auto event = sdeventplus::Event::get_default();

    // Get a handle to system dbus
    auto bus = sdbusplus::bus::new_default();

    // Add the ObjectManager interface
    sdbusplus::server::manager::manager objManager(bus, "/");

    // Create an led controller object
    phosphor::led::Controller controller(bus);

    // Request service bus name
    bus.request_name(busName);

    // Attach the bus to sd_event to service user requests
    bus.attach_event(event.get(), SD_EVENT_PRIORITY_NORMAL);
    event.loop();

    return 0;
}
