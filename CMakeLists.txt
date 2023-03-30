# Copyright (c) 2020 by Robert Bosch GmbH. All rights reserved.
# Copyright (c) 2020 - 2021 by Apex.AI Inc. All rights reserved.
# Copyright (c) 2022 NXP. All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# SPDX-License-Identifier: Apache-2.0

##############################################################################
# Modifications by NXP 2022
##############################################################################

cmake_minimum_required(VERSION 3.11)

set(IOX_VERSION_STRING "2.0.2")

project(p3com VERSION ${IOX_VERSION_STRING})

find_package(iceoryx_hoofs REQUIRED)
find_package(iceoryx_posh REQUIRED)

include(IceoryxPackageHelper)
include(IceoryxPlatform)

#option(PCIE_TRANSPORT "Builds the iceoryx PCIe transport - enables internode communication via PCIe" OFF)
option(UDP_TRANSPORT "Builds the iceoryx UDP transport - enables internode communication via UDP" ON)
option(TCP_TRANSPORT "Builds the iceoryx TCP transport - enables internode communication via TCP" OFF)

#
########## set variables for export ##########
#
setup_package_name_and_create_files(
    NAME ${PROJECT_NAME}
    NAMESPACE p3com
    PROJECT_PREFIX ${PREFIX}
)

#
########## find_package in source tree ##########
#
set(${PROJECT_NAME}_DIR ${CMAKE_CURRENT_LIST_DIR}/cmake
    CACHE FILEPATH
    "${PROJECT_NAME}Config.cmake to make find_package(${PROJECT_NAME}) work in source tree!"
    FORCE
)

#
########## build building-block library ##########
#
if(PCIE_TRANSPORT OR UDP_TRANSPORT OR TCP_TRANSPORT)
    add_library(p3com STATIC)
    add_library(${PROJECT_NAMESPACE}::p3com ALIAS p3com)

    set_target_properties(p3com PROPERTIES
        CXX_STANDARD_REQUIRED ON
        CXX_STANDARD ${ICEORYX_CXX_STANDARD}
        POSITION_INDEPENDENT_CODE ON
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    )

    target_include_directories(p3com
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/dependencies/install/include>
        $<INSTALL_INTERFACE:include/${PREFIX}>
    )

    if(FREERTOS)
        find_package(Threads REQUIRED)
        find_package(arm32m7_sdk REQUIRED
                     COMPONENTS
                       runtime_core0
                       std++_freertos
                       freertos
        )

        target_link_libraries(p3com
            PUBLIC
            arm32m7_sdk::runtime_core0
            arm32m7_sdk::std++_freertos
            Threads::Threads
            arm32m7_sdk::freertos
        )

        target_sources(p3com
            PRIVATE
            $ENV{OVSP_ROOT}/arm32m7_sdk/modules/runtime/sys_common/sys.cpp
            $ENV{OVSP_ROOT}/arm32m7_sdk/modules/runtime/sys_common/FreeRTOS_memory.cpp
        )

    else()
        add_executable(p3com-gw
            source/p3com/gateway/main.cpp
        )

        set_target_properties(p3com-gw PROPERTIES
            CXX_STANDARD_REQUIRED ON
            CXX_STANDARD ${ICEORYX_CXX_STANDARD}
        )

        target_compile_options(p3com-gw
            PRIVATE
            ${ICEORYX_WARNINGS}
            ${ICEORYX_SANITIZER_FLAGS}
        )

        target_link_libraries(p3com-gw PRIVATE p3com::p3com)

        target_include_directories(p3com-gw
            PRIVATE
            $<INSTALL_INTERFACE:include/${PREFIX}>
        )

        setup_install_directories_and_export_package(
            TARGETS p3com-gw
            INCLUDE_DIRECTORY include/
        )
    endif()

    target_link_libraries(p3com
        PUBLIC
        iceoryx_posh::iceoryx_posh
        iceoryx_hoofs::iceoryx_hoofs
    )

    target_sources(p3com
        PRIVATE
        source/p3com/generic/data_reader.cpp
        source/p3com/generic/data_writer.cpp
        source/p3com/generic/discovery.cpp
        source/p3com/generic/serialization.cpp
        source/p3com/generic/pending_messages.cpp
        source/p3com/generic/segmented_messages.cpp
        source/p3com/generic/transport_forwarder.cpp
        source/p3com/gateway/iox_to_transport.cpp
        source/p3com/gateway/transport_to_iox.cpp
        source/p3com/gateway/gateway_config.cpp
        source/p3com/gateway/cmd_line.cpp
        source/p3com/gateway/gateway_app.cpp
        source/p3com/transport/transport_info.cpp
        source/p3com/log/logging.cpp
        source/p3com/introspection/gw_introspection.cpp
    )

    target_compile_options(p3com
        PRIVATE
        ${ICEORYX_WARNINGS}
        ${ICEORYX_SANITIZER_FLAGS}
    )

    if(TOML_CONFIG)
        find_package(cpptoml REQUIRED)

        target_link_libraries(p3com
            PUBLIC
            iceoryx_posh::iceoryx_posh_config
            cpptoml
        )

        target_compile_definitions(p3com
            PRIVATE
            TOML_CONFIG
        )

    install(FILES roudi_config.toml DESTINATION ${CMAKE_SYSROOT}/etc/iceoryx)
    endif()

    setup_install_directories_and_export_package(
        TARGETS p3com
        INCLUDE_DIRECTORY include/
    )
endif()

if(PCIE_TRANSPORT)
    set(PCIE_BB_TYPE "RC" CACHE STRING "Compiling for EP or RC.")
    if(PCIE_BB_TYPE STREQUAL "RC")
        set(PCIE_DEFS "PCIEBB_RC")
        find_package(lx2_sdk REQUIRED
                     COMPONENTS
                       pcie-bb-rc
                       pcie-bb-dmalib-rc)
        target_link_libraries(p3com
            PUBLIC
            lx2_sdk::pcie-bb-rc
            lx2_sdk::pcie-bb-dmalib-rc)
    else()
        set(PCIE_DEFS "PCIEBB_EP")
        if(NOT FREERTOS)
            find_package(s32g_sdk REQUIRED
                         COMPONENTS
                           pcie-bb-ep
                           pcie-bb-dmalib-ep)
            target_link_libraries(p3com
                PUBLIC
                s32g_sdk::pcie-bb-ep
                s32g_sdk::pcie-bb-dmalib-ep)
        endif()
    endif()


    target_sources(p3com
        PRIVATE
        source/pcie/pcie_transport.cpp
    )

    target_compile_definitions(p3com
        PUBLIC
        PCIE_TRANSPORT
        ${PCIE_DEFS}
    )
endif()

if(UDP_TRANSPORT)
    target_sources(p3com
        PRIVATE
        source/udp/udp_transport.cpp
        source/udp/udp_transport_broadcast.cpp
    )

    target_compile_definitions(p3com
        PUBLIC
        UDP_TRANSPORT
    )
endif()

if(TCP_TRANSPORT)
    target_sources(p3com
        PRIVATE
        source/tcp/tcp_transport.cpp
        source/tcp/tcp_transport_session.cpp
        source/tcp/tcp_client_transport_session.cpp
        source/tcp/tcp_server_transport_session.cpp
        source/udp/udp_transport_broadcast.cpp
    )

    target_compile_definitions(p3com
        PUBLIC
        TCP_TRANSPORT
    )
endif()
