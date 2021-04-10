# Install script for directory: D:/software/nrf9160/my_projects/my_test/test_energy_consumption

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "C:/Program Files (x86)/Zephyr-Kernel")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "TRUE")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("D:/software/nrf9160/my_projects/my_test/test_energy_consumption/build_nrf9160_pca10090ns/zephyr/cmake_install.cmake")
  include("D:/software/nrf9160/my_projects/my_test/test_energy_consumption/build_nrf9160_pca10090ns/src/datetime/cmake_install.cmake")
  include("D:/software/nrf9160/my_projects/my_test/test_energy_consumption/build_nrf9160_pca10090ns/src/alarm/cmake_install.cmake")
  include("D:/software/nrf9160/my_projects/my_test/test_energy_consumption/build_nrf9160_pca10090ns/src/font/cmake_install.cmake")
  include("D:/software/nrf9160/my_projects/my_test/test_energy_consumption/build_nrf9160_pca10090ns/src/image/cmake_install.cmake")
  include("D:/software/nrf9160/my_projects/my_test/test_energy_consumption/build_nrf9160_pca10090ns/src/lcd/cmake_install.cmake")
  include("D:/software/nrf9160/my_projects/my_test/test_energy_consumption/build_nrf9160_pca10090ns/src/key/cmake_install.cmake")
  include("D:/software/nrf9160/my_projects/my_test/test_energy_consumption/build_nrf9160_pca10090ns/src/settings/cmake_install.cmake")
  include("D:/software/nrf9160/my_projects/my_test/test_energy_consumption/build_nrf9160_pca10090ns/src/inner_flash/cmake_install.cmake")
  include("D:/software/nrf9160/my_projects/my_test/test_energy_consumption/build_nrf9160_pca10090ns/src/external_flash/cmake_install.cmake")
  include("D:/software/nrf9160/my_projects/my_test/test_energy_consumption/build_nrf9160_pca10090ns/src/uart_ble/cmake_install.cmake")
  include("D:/software/nrf9160/my_projects/my_test/test_energy_consumption/build_nrf9160_pca10090ns/src/imei/cmake_install.cmake")
  include("D:/software/nrf9160/my_projects/my_test/test_energy_consumption/build_nrf9160_pca10090ns/src/gps/cmake_install.cmake")
  include("D:/software/nrf9160/my_projects/my_test/test_energy_consumption/build_nrf9160_pca10090ns/src/nb/cmake_install.cmake")
  include("D:/software/nrf9160/my_projects/my_test/test_energy_consumption/build_nrf9160_pca10090ns/src/tp/cmake_install.cmake")
  include("D:/software/nrf9160/my_projects/my_test/test_energy_consumption/build_nrf9160_pca10090ns/src/imu/cmake_install.cmake")
  include("D:/software/nrf9160/my_projects/my_test/test_energy_consumption/build_nrf9160_pca10090ns/src/screen/cmake_install.cmake")
  include("D:/software/nrf9160/my_projects/my_test/test_energy_consumption/build_nrf9160_pca10090ns/src/pmu/cmake_install.cmake")
  include("D:/software/nrf9160/my_projects/my_test/test_energy_consumption/build_nrf9160_pca10090ns/src/ppg/cmake_install.cmake")
  include("D:/software/nrf9160/my_projects/my_test/test_energy_consumption/build_nrf9160_pca10090ns/src/codetrans/cmake_install.cmake")
  include("D:/software/nrf9160/my_projects/my_test/test_energy_consumption/build_nrf9160_pca10090ns/src/ucs2/cmake_install.cmake")

endif()

if(CMAKE_INSTALL_COMPONENT)
  set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
else()
  set(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
file(WRITE "D:/software/nrf9160/my_projects/my_test/test_energy_consumption/build_nrf9160_pca10090ns/${CMAKE_INSTALL_MANIFEST}"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
