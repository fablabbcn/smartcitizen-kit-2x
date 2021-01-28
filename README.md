# Smart Citizen Kit 2.1
[![Travis](https://travis-ci.org/fablabbcn/smartcitizen-kit-21.svg?branch=master)](https://travis-ci.org/fablabbcn/smartcitizen-kit-21)
[![PRs Welcome](https://img.shields.io/badge/PRs-welcome-brightgreen.svg)]()
[![DOI](https://zenodo.org/badge/109865611.svg)](https://zenodo.org/badge/latestdoi/109865611)
![](https://live.staticflickr.com/65535/47950912168_fcf8fa398c_h.jpg)

[Click here for version 2.0](https://github.com/fablabbcn/smartcitizen-kit-20)

## Documentation

* If you want to read more about the Smartcitizen Kit or its sensors please check our [documentation](http://docs.smartcitizen.me/)
* If you just want to **upgrade** your Smartcitizen kit **firmware** please refer to the [Updating the Smartcitizen Kit Firmware](http://docs.smartcitizen.me/Guides/firmware/Update%20the%20firmware/) guide.
* If you want to change something and build the firmware, use the [Building and flashing the Smartcitizen Kit firmware](http://docs.smartcitizen.me/Guides/firmware/Edit%20the%20Firmware/) guide.
* The SCK provides a comprehensive command shell over USB to manage all the kits functionalities for advanced users: [Using the Smartcitizen Kit shell](http://docs.smartcitizen.me/Guides/getting%20started/Using%20the%20Shell/) guide.
* A compilation of Smartcitizen hardware work in a single **open publication:** [_Hardware X: Special Issue on Open-Hardware for Environmental Sensing and Instruments_](https://doi.org/10.1016/j.ohx.2019.e00070)
* You can also follow the [forum](https://forum.smartcitizen.me/) and [twitter](https://twitter.com/SmartCitizenKit) for updates.

## Development

In this repository you can find:

* The Smartcitizen Kit 2.1 hardware [design files](./hardware).
* The [bootloader](bootloader) and [main firmware](./sam) (C++) for the SAMD21 microcontroller.
* The [main firmware](./esp) for the ESP8266 microcontroller.
* A [mobile web UI](./mock-api) for setting up the kit.

All branches and pull requests on Github are auto tested with Travis

## Related Smart Citizen repositories

* Platform Core API [github.com/fablabbcn/smartcitizen-api](https://github.com/fablabbcn/smartcitizen-api)
* Platform Web [github.com/fablabbcn/smartcitizen-web](https://github.com/fablabbcn/smartcitizen-web)
* Platform Onboarding [github.com/fablabbcn/smartcitizen-onboarding-app](https://github.com/fablabbcn/smartcitizen-onboarding-app)
* Kit Enclosures [github.com/fablabbcn/smartcitizen-enclosures](https://github.com/fablabbcn/smartcitizen-enclosures)
* Useful software resources for communities [github.com/fablabbcn/smartcitizen-toolkit](https://github.com/fablabbcn/smartcitizen-toolkit)
* Advanced data analysis tools [github.com/fablabbcn/smartcitizen-data](https://github.com/fablabbcn/smartcitizen-data)
* Analog Sensor Board [github.com/fablabbcn/smartcitizen-analog-sensor-board](https://github.com/fablabbcn/smartcitizen-analog-sensor-board)
* PM Board [github.com/fablabbcn/smartcitizen-kit-pm-board](https://github.com/fablabbcn/smartcitizen-kit-pm-board)

## License

All the software unless stated is released under [GNU GPL v3.0](https://github.com/fablabbcn/smartcitizen-kit-20/blob/master/LICENSE) and the hardware design files under [CERN OHL v1.2](https://github.com/fablabbcn/smartcitizen-kit-20/blob/master/hardware/LICENSE)

## Funding

This work has received funding from the European Union's Horizon 2020 research and innovation program under the grant agreement [No. 689954](https://cordis.europa.eu/project/rcn/202639_en.html)
