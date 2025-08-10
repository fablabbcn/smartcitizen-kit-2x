# Daphi Sensor 0.X

Welcome to the Daphi Sensor main repository. Here you will find the firmware for the sensor.

## Documentation
Documentation of the SCK repo, from which it was forked
- Main website: https://smartcitizen.me/
- Documentation: https://docs.smartcitizen.me/guides/firmware/edit-the-firmware/#getting-the-firmware
- Firmware Support Email: support@smartcitizen.me
- Data API: https://developer.smartcitizen.me/#summary

## Development

For new contributors, please read the [contribution guidelines](https://github.com/Daphi-Prevent-FLW/Daphi-Sensor/blob/master/guides%20and%20manuals/How%20to%20contribute.md).

Hardware used is under [Hardware Overview](https://github.com/Daphi-Prevent-FLW/Daphi-Sensor/blob/master/guides%20and%20manuals/Hardware%20Overview.md).

## Code Style and Static Analysis Checks

To ensure code quality and consistency, we provide a Docker-based environment for running `clang-tidy` and `clang-format` checks. You do not need to install these tools locally.

### How to Run Clang Checks

1. **Build the Docker image** (only needed once, from the project root):

    ```sh
    docker build -t daphi-clang-checks guidelines_checks
    ```

2. **Run checks for a specific file** (replace `esp/src/SckESP.cpp` with your file):

    ```sh
    docker run --rm -v "$PWD":/workspace -w /workspace daphi-clang-checks sh /workspace/run-clang-checks.sh esp/src/SckESP.cpp
    ```

   You can specify multiple files separated by spaces.

3. **Run checks for all files in `esp/src`** (default):

    ```sh
    docker run --rm -v "$PWD":/workspace -w /workspace daphi-clang-checks
    ```

### Notes

- The configuration files and scripts are in the `guidelines_checks/` directory.
- These checks enforce naming, style, and magic number rules as described in the [contribution guidelines](guides%20and%20manuals/How%20to%20contribute.md).
- Please run these checks before submitting a pull request.


## Related repositories

This is a fork of the [Smart Citizen Kit 2.X](https://github.com/fablabbcn/smartcitizen-kit-2x).

## License

All the software unless stated otherwise is released under [GNU GPL v3.0](https://github.com/fablabbcn/smartcitizen-kit-20/blob/master/LICENSE).

## Inquireis

Please contact us at: daphi.dnw@gmail.com
