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
# HAL 公共头文件目录
# ============================================================
set(HAL_INCLUDE_DIRS
    ${CMAKE_SOURCE_DIR}/src/hal
    ${CMAKE_SOURCE_DIR}/src/hal/bus
    ${CMAKE_SOURCE_DIR}/src/hal/queue
    ${CMAKE_SOURCE_DIR}/src/hal/comm
    ${CMAKE_SOURCE_DIR}/src/hal/frame
    ${CMAKE_SOURCE_DIR}/src/hal/uart
    ${CMAKE_SOURCE_DIR}/src/hal/timer
    ${CMAKE_SOURCE_DIR}/src/hal/led
    ${CMAKE_SOURCE_DIR}/src/hal/rs485
)

# ============================================================
# HAL (平台无关)
# ============================================================
add_library(hal_core STATIC
    src/hal/queue/ring.c
    src/hal/queue/frame_queue.c
    src/hal/comm/encoder.c
    src/hal/comm/decoder.c
    src/hal/comm/uart_decoder.c
    src/hal/comm/uart_encoder.c
    src/hal/uart/uart.c
    src/hal/uart/uart_instance.c
    src/hal/frame/receiver.c
    src/hal/frame/receiver_timeout.c
    src/hal/frame/sender.c
    src/hal/timer/timer.c
    src/hal/timer/timer_instance.c
    src/hal/rs485/rs485.c
)
target_include_directories(hal_core PUBLIC ${HAL_INCLUDE_DIRS})
if(PC_SIM)
    target_sources(hal_core PRIVATE src/hal/timer/timer_sw.c)
endif()

if(CH579)
    target_compile_definitions(hal_core PUBLIC __CH579__)
    target_sources(hal_core PRIVATE
        src/hal/led/led_ch579.c
        src/hal/rs485/rs485_ch579.c
        src/hal/uart/uart_ch579.c
        src/hal/timer/timer_ch579.c
        src/hal/timer/timer_hw.c
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
    src/hal/bus/bus.c
    src/core/module.c
    src/core/gateway_device.c
    src/modules/ac_module.c
    src/modules/wireless_module.c
    src/modules/third_party_module.c
    src/modules/wifi_module.c
)
target_include_directories(gateway_core PUBLIC
    ${CMAKE_SOURCE_DIR}/src/core ${CMAKE_SOURCE_DIR}/src/modules
    ${HAL_INCLUDE_DIRS}
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
    ${CMAKE_SOURCE_DIR}/src/core ${CMAKE_SOURCE_DIR}/src/modules ${CMAKE_SOURCE_DIR}/src/brands
    ${HAL_INCLUDE_DIRS}
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
        ${HAL_INCLUDE_DIRS}
        ${CMAKE_SOURCE_DIR}/src/core ${CMAKE_SOURCE_DIR}/src/modules
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
        app/isr.c
        src/modules/debug_module.c
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
        lib/FreeRTOS/freertos_port.c
    )
    target_include_directories(gateway_ch579 PRIVATE
        ${HAL_INCLUDE_DIRS}
        ${CMAKE_SOURCE_DIR}/src/core
        ${CMAKE_SOURCE_DIR}/src/modules
        ${CMAKE_SOURCE_DIR}/src/brands
        ${CMAKE_SOURCE_DIR}/app
        ${CMAKE_SOURCE_DIR}/bsp/CH579/StdPeriphDriver/inc
        ${CMAKE_SOURCE_DIR}/lib/CMSIS/Include
        ${CMAKE_SOURCE_DIR}/lib/FreeRTOS/include
        ${CMAKE_SOURCE_DIR}/lib/FreeRTOS
        ${CMAKE_SOURCE_DIR}/lib/FreeRTOS/Portable/RVDS/ARM_CM0
    )
    target_link_libraries(gateway_ch579 PRIVATE hal_core gateway_core gateway_brands)
endif()
