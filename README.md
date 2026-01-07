# Smart Citizen Kit 2.X

[![PRs Welcome](https://img.shields.io/badge/PRs-welcome-brightgreen.svg)]()
[![DOI](https://zenodo.org/badge/109865611.svg)](https://zenodo.org/badge/latestdoi/109865611)
[![CI](https://github.com/fablabbcn/smartcitizen-kit-2x/actions/workflows/build.yml/badge.svg)](https://github.com/fablabbcn/smartcitizen-kit-2x/actions/workflows/build.yml)

Welcome to the Smart Citizen Kit main repository. Here you will find the hardware design files and the firmware for the SCK 2.1, SCK 2.2 and SCK 2.3 versions. [Click here for version 2.0](https://github.com/fablabbcn/smartcitizen-kit-20)

![](https://live.staticflickr.com/65535/47950912168_fcf8fa398c_h.jpg)

## Documentation

* If you want to read more about the Smart Citizen Kit or its sensors please check our [documentation](http://docs.smartcitizen.me/)
* If you just want to **upgrade** your Smart Citizen kit **firmware** please refer to the [Upgrading the Smart Citizen Kit Firmware](https://docs.smartcitizen.me/guides/firmware/upgrading-the-firmware/) guide.
* If you want to change something and build the firmware, use the [Building and flashing the Smart Citizen Kit firmware](https://docs.smartcitizen.me/guides/firmware/edit-the-firmware/) guide.
* The SCK provides a comprehensive command shell over USB to manage all the kits functionalities for advanced users: [Using the Smartcitizen Kit shell](https://docs.smartcitizen.me/guides/getting-started/using-the-shell/) guide.
* A compilation of Smartcitizen hardware work in a single **open publication:** [_Hardware X: Special Issue on Open-Hardware for Environmental Sensing and Instruments_](https://doi.org/10.1016/j.ohx.2019.e00070).
* You can also follow the [forum](https://forum.smartcitizen.me/) for updates.

## Development

In this repository you can find:

* The Smart Citizen Kit hardware [design files](./hardware).
* The [bootloader](bootloader) and [main firmware](./sam) (C++) for the SAMD21 microcontroller.
* The [main firmware](./esp) for the ESP8266 microcontroller.
* A [mobile web UI](./mock-api) for setting up the kit.

## Related Smart Citizen repositories

* Platform Core API [github.com/fablabbcn/smartcitizen-api](https://github.com/fablabbcn/smartcitizen-api)
* Platform Web [github.com/fablabbcn/smartcitizen-web](https://github.com/fablabbcn/smartcitizen-web)
* Platform Onboarding [github.com/fablabbcn/smartcitizen-onboarding-app](https://github.com/fablabbcn/smartcitizen-onboarding-app)
* Kit Enclosures [github.com/fablabbcn/smartcitizen-enclosures](https://github.com/fablabbcn/smartcitizen-enclosures)
* Useful software resources for communities [github.com/fablabbcn/smartcitizen-toolkit](https://github.com/fablabbcn/smartcitizen-toolkit)
* Advanced data analysis tools [github.com/fablabbcn/smartcitizen-data](https://github.com/fablabbcn/smartcitizen-data)
* Python API connector [github.com/fablabbcn/smartcitizen-connector](https://github.com/fablabbcn/smartcitizen-connector)
* Analog Sensor Board [github.com/fablabbcn/smartcitizen-analog-sensor-board](https://github.com/fablabbcn/smartcitizen-analog-sensor-board)
* PM Board [github.com/fablabbcn/smartcitizen-kit-pm-board](https://github.com/fablabbcn/smartcitizen-kit-pm-board)

## License

All the software unless stated is released under [GNU GPL v3.0](https://github.com/fablabbcn/smartcitizen-kit-20/blob/master/LICENSE) and the hardware design files under [CERN OHL v1.2](https://github.com/fablabbcn/smartcitizen-kit-20/blob/master/hardware/LICENSE)

## Funding

This work has received funding from the European Union's Horizon 2020 research and innovation program under the grant agreement [No. 689954](https://cordis.europa.eu/project/rcn/202639_en.html)
