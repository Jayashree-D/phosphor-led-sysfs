/**
 * Copyright © 2016,2018 IBM Corporation
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

#include "physical.hpp"

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <string>
namespace phosphor
{
namespace led
{

/** @brief Populates key parameters */
void Physical::setInitialState()
{
    assert = getMaxBrightness();
    auto trigger = getTrigger();
    if (trigger == "timer")
    {
        // LED is blinking. Get the on and off delays and derive percent duty
        auto delayOn = getDelayOn();
        uint16_t periodMs = delayOn + getDelayOff();
        auto percentScale = periodMs / 100;
        this->dutyOn(delayOn / percentScale);
        this->period(periodMs);
    }
    else
    {
        // Cache current LED state
        auto brightness = getBrightness();
        if (brightness && assert)
        {
    //        std::cerr << " ON Led \n";
            sdbusplus::xyz::openbmc_project::Led::server::Physical::state(
                Action::On);
        }
        else
        {
      //      std::cerr << " Off Led \n";
            sdbusplus::xyz::openbmc_project::Led::server::Physical::state(
                Action::Off);
        }
    }
    return;
}

auto Physical::state() const -> Action
{
	std::cerr << " Get property \n";
    return sdbusplus::xyz::openbmc_project::Led::server::Physical::state();
}

auto Physical::state(Action value) -> Action
{
	std::cerr << "Set property \n";
	std::cerr << "obj path : " << objPath << "\n";
    auto current =
        sdbusplus::xyz::openbmc_project::Led::server::Physical::state();

    auto requested =
        sdbusplus::xyz::openbmc_project::Led::server::Physical::state(value);

    driveLED(current, requested);

//    std::cerr << "Val : " << static_cast<int>(value) << "\n";
    return value;
}

void Physical::driveLED(Action current, Action request)
{
  //  std::cerr << " --------- drive led ------\n";

    if (current == request)
    {
        return;
    }

    if (request == Action::On || request == Action::Off)
    {
        return stableStateOperation(request);
    }

    assert(request == Action::Blink);
    blinkOperation();
}

void Physical::stableStateOperation(Action action)
{
    //std::cerr << " stable opertaion\n";

    auto value = (action == Action::On) ? assert : DEASSERT;

    //std::cerr << "After value \n";
    //std::cerr << " Value  : " << value << "\n";

    setTrigger("none");

    //std::cerr << " After set trigger \n";

    setBrightness(value);

    //std::cerr << " After set brightness \n";

    return;
}

void Physical::blinkOperation()
{
    //std::cerr << " In blink operation \n";
    auto dutyOn = this->dutyOn();

    //std::cerr << " Before set trigger \n";

    /*
      The configuration of the trigger type must precede the configuration of
      the trigger type properties. From the kernel documentation:
      "You can change triggers in a similar manner to the way an IO scheduler
      is chosen (via /sys/class/leds/<device>/trigger). Trigger specific
      parameters can appear in /sys/class/leds/<device> once a given trigger is
      selected."
      Refer:
      https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/Documentation/leds/leds-class.txt?h=v5.2#n26
    */
    setTrigger("timer");

    //std::cerr << " After set trigger \n";

    // Convert percent duty to milliseconds for sysfs interface
    auto factor = this->period() / 100.0;

    //std::cerr << " Before set delayon \n";

    setDelayOn(dutyOn * factor);

    //std::cerr << " After set delayon \n";

    setDelayOff((100 - dutyOn) * factor);

    //std::cerr << " After set delayoff \n";

    return;
}

/** @brief set led color property in DBus*/
void Physical::setLedColor(const std::string& color)
{
    static const std::string prefix =
        "xyz.openbmc_project.Led.Physical.Palette.";
    if (!color.length())
        return;
    std::string tmp = color;
    tmp[0] = toupper(tmp[0]);
    try
    {
        auto palette = convertPaletteFromString(prefix + tmp);
        setPropertyByName("Color", palette);
    }
    catch (const sdbusplus::exception::InvalidEnumString&)
    {
        // if color var contains invalid color,
        // Color property will have default value
    }
}

} // namespace led
} // namespace phosphor
