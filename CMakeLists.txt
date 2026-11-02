cmake_minimum_required(VERSION 3.16)
project(pr-mediad VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra -Wpedantic)

find_package(PkgConfig REQUIRED)
pkg_check_modules(UDEV   REQUIRED libudev)
pkg_check_modules(BLKID  REQUIRED blkid)
pkg_check_modules(NOTIFY REQUIRED libnotify)

add_library(cjson STATIC vendor/cjson/cJSON.c)
target_include_directories(cjson PUBLIC vendor/cjson)

set(MEDIAD_SOURCES
    src/platform_db.c
    src/media_detect.c
    src/ipc.c
    src/notify.c
)

add_executable(pr-mediad src/main.c ${MEDIAD_SOURCES})
target_include_directories(pr-mediad PRIVATE src
    ${UDEV_INCLUDE_DIRS} ${BLKID_INCLUDE_DIRS} ${NOTIFY_INCLUDE_DIRS})
target_link_libraries(pr-mediad
    ${UDEV_LIBRARIES} ${BLKID_LIBRARIES} ${NOTIFY_LIBRARIES} cjson)

# Tests (each test target is registered by the task that creates its source)
enable_testing()

install(TARGETS pr-mediad DESTINATION /usr/bin)
install(FILES pr-mediad.service
    DESTINATION /usr/lib/systemd/system)
