# LVGL v9 Demo
## Introduction

This example demonstrates LVGL V9 official examples using officially provided demo applications.
You can use menuconfig to select the demo application to demonstrate. The included applications are:
- Show some widget - demonstrates the use of lvgl widgets
    - Benchmark your system - demonstrates benchmark (depends on "Show some widget")
- Demonstrate the usage of encoder and keyboard - demonstrates keyboard
- Stress test for LVGL - stress test
- Music player demo - demonstrates music playback

## Project Compilation and Download:
Board projects in the project directory can be compiled for corresponding boards by specifying the board parameter,
- For example, to compile a project that can run on HDK 563, execute scons --board=eh-lb563 to generate the project
- Download can be performed through download.bat in the build directory. For example, to flash the 563 project generated in the previous step, execute .\build_eh-lb563\download.bat to download via jlink
- Special note: for SF32LB52x/SF32LB56x series, an additional uart_download.bat will be generated. You can execute this script and input the download UART port number to perform the download
Simulator projects are in the simulator directory,
- Use scons for compilation. The simulator/msvc_setup.bat file needs to be modified accordingly to match the local MSVC configuration
- You can also use scons --target=vs2017 to generate MSVC project project.vcxproj and compile using Visual Studio.

```{note}
If not using VS2017, for example VS2022, when loading the project, it will prompt to upgrade the MSVC SDK. After upgrading, it can be used.
```