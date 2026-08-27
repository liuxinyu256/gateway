cmake_minimum_required(VERSION 3.14)
project(gateway C)
set(CMAKE_C_STANDARD 99)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ============================================================
# Options
# ============================================================
option(PC_SIM "Build for PC simulation" ON)
option(CH579 "Build for CH579 target" OFF)

# ============================================================
# HAL (平台无关)
# ============================================================
add_library(hal_core STATIC
    src/hal/ring.c
    src/hal/frame_queue.c
    src/hal/encoder.c
    src/hal/decoder.c
    src/hal/uart_decoder.c
    src/hal/uart_encoder.c
    src/hal/uart.c
    src/hal/uart_instance.c
    src/hal/receiver.c
    src/hal/receiver_timeout.c
    src/hal/sender.c
    src/hal/timer.c
)
target_include_directories(hal_core PUBLIC ${CMAKE_SOURCE_DIR}/src/hal)
if(PC_SIM)
    target_sources(hal_core PRIVATE src/hal/timer_sw.c)
endif()

if(CH579)
    target_compile_definitions(hal_core PUBLIC __CH579__)
    target_sources(hal_core PRIVATE
        src/hal/uart_ch579.c
        src/hal/timer_ch579.c
        src/hal/timer_hw.c
    )
    target_include_directories(hal_core PUBLIC
        ${CMAKE_SOURCE_DIR}/bsp/CH579/StdPeriphDriver/inc
    )
endif()

# ============================================================
# HAL — PC 软件定时器
# ============================================================
if(PC_SIM)
    add_compile_definitions(FAKE_FREERTOS)
endif()

# ============================================================
# Core 框架
# ============================================================
add_library(gateway_core STATIC
    src/hal/bus.c 
    src/core/module.c
    src/core/gateway_device.c
    src/modules/ac_module.c
    src/modules/wireless_module.c
    src/modules/third_party_module.c
    src/modules/wifi_module.c
)
target_include_directories(gateway_core PUBLIC
    ${CMAKE_SOURCE_DIR}/src/core ${CMAKE_SOURCE_DIR}/src/modules
    ${CMAKE_SOURCE_DIR}/src/hal
)
if(PC_SIM)
    target_include_directories(gateway_core PUBLIC ${CMAKE_SOURCE_DIR}/sim)
endif()
target_link_libraries(gateway_core PUBLIC hal_core)

# ============================================================
# 品牌库
# ============================================================
add_library(gateway_brands STATIC src/brands/ac_gree.c)
target_include_directories(gateway_brands PUBLIC
    ${CMAKE_SOURCE_DIR}/src/core ${CMAKE_SOURCE_DIR}/src/modules ${CMAKE_SOURCE_DIR}/src/hal
    ${CMAKE_SOURCE_DIR}/src/brands
)
target_link_libraries(gateway_brands PUBLIC gateway_core)

# ============================================================
# PC 模拟器
# ============================================================
if(PC_SIM)
    add_executable(sim_app
        sim/sim_app.c
        sim/fake_freertos_timer.c
    )
    target_include_directories(sim_app PRIVATE
        ${CMAKE_SOURCE_DIR}/src/hal ${CMAKE_SOURCE_DIR}/src/core ${CMAKE_SOURCE_DIR}/src/modules
        ${CMAKE_SOURCE_DIR}/src/brands ${CMAKE_SOURCE_DIR}/sim
        ${CMAKE_SOURCE_DIR}/app
    )
    target_link_libraries(sim_app PRIVATE hal_core gateway_core gateway_brands)
endif()

# ============================================================
# CH579 实机目标
# ============================================================
if(CH579)
    add_executable(gateway_ch579
        app/main.c
        app/hvac_init.c
        bsp/CH579/startup_ARMCM0.s
        bsp/CH579/StdPeriphDriver/CH57x_clk.c
        bsp/CH579/StdPeriphDriver/CH57x_flash.c
        bsp/CH579/StdPeriphDriver/CH57x_gpio.c
        bsp/CH579/StdPeriphDriver/CH57x_int.c
        bsp/CH579/StdPeriphDriver/CH57x_pwr.c
        bsp/CH579/StdPeriphDriver/CH57x_sys.c
        bsp/CH579/StdPeriphDriver/CH57x_timer0.c
        bsp/CH579/StdPeriphDriver/CH57x_timer1.c
        bsp/CH579/StdPeriphDriver/CH57x_timer2.c
        bsp/CH579/StdPeriphDriver/CH57x_timer3.c
        bsp/CH579/StdPeriphDriver/CH57x_uart0.c
        bsp/CH579/StdPeriphDriver/CH57x_uart1.c
        bsp/CH579/StdPeriphDriver/CH57x_uart2.c
        bsp/CH579/StdPeriphDriver/CH57x_uart3.c
    )
    target_include_directories(gateway_ch579 PRIVATE
        ${CMAKE_SOURCE_DIR}/src/hal
        ${CMAKE_SOURCE_DIR}/src/core
        ${CMAKE_SOURCE_DIR}/src/modules
        ${CMAKE_SOURCE_DIR}/src/brands
        ${CMAKE_SOURCE_DIR}/app
        ${CMAKE_SOURCE_DIR}/bsp/CH579/StdPeriphDriver/inc
    )
    target_link_libraries(gateway_ch579 PRIVATE hal_core gateway_core gateway_brands)
endif()
