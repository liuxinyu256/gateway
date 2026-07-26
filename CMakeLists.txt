cmake_minimum_required(VERSION 3.14)
project(gateway C)
set(CMAKE_C_STANDARD 99)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ============================================================
# Options
# ============================================================
option(PC_SIM "Build for PC simulation" ON)
option(CH579   "Build for CH579 embedded target" OFF)

# ============================================================
# HAL (平台无关)
# ============================================================
add_library(hal_core STATIC
    src/hal/ring.c
    src/hal/receiver.c
    src/hal/receiver_timeout.c src/hal/phy_factory.c src/hal/sender.c
)
target_include_directories(hal_core PUBLIC ${CMAKE_SOURCE_DIR}/src/hal)

# ============================================================
# HAL — PC 软件定时器
# ============================================================
if(PC_SIM)
    add_library(hal_sw STATIC src/hal/frame_timer_sw.c)
    target_include_directories(hal_sw PUBLIC ${CMAKE_SOURCE_DIR}/src/hal)
    target_link_libraries(hal_sw PUBLIC hal_core)
    add_compile_definitions(FAKE_FREERTOS)
endif()

# ============================================================
# HAL — CH579 硬件定时器 + UART1 PHY
# ============================================================
if(CH579)
    add_library(hal_ch579 STATIC
        src/hal/frame_timer_hw.c
        src/hal/phy_uart1.c
    )
    target_include_directories(hal_ch579 PUBLIC
        ${CMAKE_SOURCE_DIR}/src/hal
        ${CMAKE_SOURCE_DIR}/bsp/CH579/StdPeriphDriver/inc
    )
    target_link_libraries(hal_ch579 PUBLIC hal_core)
    add_compile_definitions(__CH579__)
endif()

# ============================================================
# Core 框架
# ============================================================
add_library(gateway_core STATIC
    src/hal/bus.c 
    src/core/module.c
    src/core/gateway_device.c
    src/managers/ac_brand_manager.c
    src/managers/wireless_module_mgr.c
    src/managers/third_party_mgr.c
    src/managers/wifi_mgr.c
)
target_include_directories(gateway_core PUBLIC
    ${CMAKE_SOURCE_DIR}/src/core ${CMAKE_SOURCE_DIR}/src/managers
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
    ${CMAKE_SOURCE_DIR}/src/core ${CMAKE_SOURCE_DIR}/src/managers ${CMAKE_SOURCE_DIR}/src/hal
    ${CMAKE_SOURCE_DIR}/src/brands
)
target_link_libraries(gateway_brands PUBLIC gateway_core)

# ============================================================
# CH579 BSP + FreeRTOS (仅嵌入式目标)
# ============================================================
if(CH579)
    # BSP
    file(GLOB BSP_SOURCES
        bsp/CH579/StdPeriphDriver/CH57x_*.c
    )
    add_library(bsp_ch579 STATIC ${BSP_SOURCES})
    target_include_directories(bsp_ch579 PUBLIC
        ${CMAKE_SOURCE_DIR}/bsp/CH579/StdPeriphDriver/inc
    )

    # FreeRTOS
    set(FREERTOS_SOURCES
        lib/FreeRTOS/Source/tasks.c
        lib/FreeRTOS/Source/queue.c
        lib/FreeRTOS/Source/list.c
        lib/FreeRTOS/Source/timers.c
        lib/FreeRTOS/Source/event_groups.c
        lib/FreeRTOS/Source/stream_buffer.c
        lib/FreeRTOS/Portable/MemMang/heap_4.c
        lib/FreeRTOS/Portable/RVDS/ARM_CM0/port.c
        lib/FreeRTOS/freertos_port.c
    )
    add_library(freertos_ch579 STATIC ${FREERTOS_SOURCES})
    target_include_directories(freertos_ch579 PUBLIC
        ${CMAKE_SOURCE_DIR}/lib/FreeRTOS/include
        ${CMAKE_SOURCE_DIR}/lib/FreeRTOS/Portable/RVDS/ARM_CM0
        ${CMAKE_SOURCE_DIR}/lib/FreeRTOS
        ${CMAKE_SOURCE_DIR}/lib/CMSIS/Include
        ${CMAKE_SOURCE_DIR}/bsp/CH579/StdPeriphDriver/inc
    )
    target_compile_definitions(freertos_ch579 PRIVATE __CH579__)

    # App (入口)
    add_executable(gateway_ch579.elf
        app/main.c
        app/hvac_init.c
    )
    target_include_directories(gateway_ch579.elf PRIVATE
        ${CMAKE_SOURCE_DIR}/app
        ${CMAKE_SOURCE_DIR}/src/core ${CMAKE_SOURCE_DIR}/src/managers
        ${CMAKE_SOURCE_DIR}/src/hal
        ${CMAKE_SOURCE_DIR}/src/brands
        ${CMAKE_SOURCE_DIR}/bsp/CH579/StdPeriphDriver/inc
        ${CMAKE_SOURCE_DIR}/lib/FreeRTOS/include
        ${CMAKE_SOURCE_DIR}/lib/FreeRTOS/Portable/RVDS/ARM_CM0
        ${CMAKE_SOURCE_DIR}/lib/FreeRTOS
        ${CMAKE_SOURCE_DIR}/lib/CMSIS/Include
    )
    target_link_libraries(gateway_ch579.elf
        hal_ch579 gateway_core gateway_brands bsp_ch579 freertos_ch579
    )
endif()

# ============================================================
# PC 模拟器
# ============================================================
if(PC_SIM)
    add_executable(sim_app
        sim/sim_app.c
        src/hal/phy_uart1.c  # PC空实现
    )
    target_include_directories(sim_app PRIVATE
        ${CMAKE_SOURCE_DIR}/src/hal ${CMAKE_SOURCE_DIR}/src/core ${CMAKE_SOURCE_DIR}/src/managers
        ${CMAKE_SOURCE_DIR}/src/brands ${CMAKE_SOURCE_DIR}/sim
        ${CMAKE_SOURCE_DIR}/app
    )
    target_link_libraries(sim_app PRIVATE hal_core hal_sw gateway_core gateway_brands)
endif()

# ============================================================
# 单元测试
# ============================================================
add_executable(test_packetizer test/test_receiver.c)
target_include_directories(test_packetizer PRIVATE ${CMAKE_SOURCE_DIR}/src/hal)
target_link_libraries(test_packetizer PRIVATE hal_core hal_sw)

enable_testing()
add_test(NAME packetizer_test COMMAND test_packetizer)
