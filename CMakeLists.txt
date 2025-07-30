cmake_minimum_required(VERSION 3.22)

#
# 这个文件经过修改，可以自动发现并添加用户自己的源文件和头文件。
# 由 STM32CubeMX 生成的文件已经通过其自带的脚本处理，无需在此重复添加。
#

# 设置编译器选项
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

# 定义构建类型
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# 设置项目名称
set(CMAKE_PROJECT_NAME H7Sampling)

# --- 1. 项目和语言配置 ---
# 自动从当前目录名获取项目名称
get_filename_component(CMAKE_PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
string(REGEX REPLACE "[^a-zA-Z0-9_]" "_" CMAKE_PROJECT_NAME ${CMAKE_PROJECT_NAME})
set(CMAKE_PROJECT_NAME ${CMAKE_PROJECT_NAME})

message(STATUS "Project Name: ${CMAKE_PROJECT_NAME}")

# Enable compile_commands.json for indexing by tools like clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build Type: " ${CMAKE_BUILD_TYPE})

# 启用 CMake 对 ASM 和 C 语言的支持
enable_language(C ASM)

# 创建一个可执行文件目标
add_executable(${CMAKE_PROJECT_NAME} "") # 源文件将在后面添加

# 添加 STM32CubeMX 生成的源代码
# 此命令会自动处理 Core 和 Drivers 目录中由 CubeMX 生成的所有文件
add_subdirectory(cmake/stm32cubemx)

# --- 自动化添加用户自定义文件脚本 ---


# --- 2. MCU 和库配置 ---
# !!! 用户配置: 在这里选择您的 MCU 系列 !!!
# 设置为 ON 以使用 H7 系列的配置, 设置为 OFF 以使用 F4 系列的配置。
option(MCU_FAMILY_H7 "Configure for STM32H7 series" OFF)

if(MCU_FAMILY_H7)
    message(STATUS "Configuring for STM32H7 series")
    # H7 系列 (如 H743) 使用 Cortex-M7 内核和双精度 FPU
    # 库名: libarm_cortexM7lfd_math.a
    set(CMSIS_DSP_LIB "arm_cortexM7lfdp_math")
else()
    message(STATUS "Configuring for STM32F4 series")
    # F4 系列 (如 F407) 使用 Cortex-M4 内核和单精度 FPU
    # 库名: libarm_cortexM4lf_math.a
    set(CMSIS_DSP_LIB "arm_cortexM4lf_math")
endif()


# 在这里定义您自己创建的代码文件夹。
# !!! 重要 !!!: 不要将 "Core" 或 "Drivers" 添加到此列表，
# 因为它们已由上面的 add_subdirectory 命令处理。
set(USER_FOLDERS
    # 例如: 如果您创建了一个 "App" 文件夹来存放您的应用代码，
    # 就在这里添加它: "App"
    "App" 
    "Lib"
)

# 初始化用户源文件和头文件目录列表
set(USER_SOURCES)
set(USER_INCLUDE_DIRS)

message(STATUS "Searching for user-defined files...")
foreach(FOLDER ${USER_FOLDERS})
    set(FOLDER_PATH "${CMAKE_CURRENT_SOURCE_DIR}/${FOLDER}")
    if(NOT EXISTS ${FOLDER_PATH})
        continue()
    endif()

    # **修正 1: 真正递归搜索所有 .c 文件**
    # 在指定的用户文件夹下递归查找所有 .c 文件，无论它们在多深的子目录中。
    file(GLOB_RECURSE FOLDER_SOURCES "${FOLDER_PATH}/*.c")
    if(FOLDER_SOURCES)
        list(APPEND USER_SOURCES ${FOLDER_SOURCES})
        message(STATUS "  - Found source files in '${FOLDER}': ${FOLDER_SOURCES}")
    endif()

    # **修正 2: 真正递归搜索所有包含头文件的目录**
    # 查找所有 .h 文件，然后获取它们所在的目录，并添加到包含路径中。
    file(GLOB_RECURSE FOLDER_HEADERS "${FOLDER_PATH}/*.h")
    if(FOLDER_HEADERS)
        foreach(HEADER_FILE ${FOLDER_HEADERS})
            get_filename_component(HEADER_DIR ${HEADER_FILE} DIRECTORY)
            list(APPEND USER_INCLUDE_DIRS ${HEADER_DIR})
        endforeach()
    endif()
endforeach()

# 移除重复的头文件路径，保持整洁
if(USER_INCLUDE_DIRS)
    list(REMOVE_DUPLICATES USER_INCLUDE_DIRS)
    message(STATUS "  - Added user include paths: ${USER_INCLUDE_DIRS}")
endif()



# --- 自动化脚本结束 ---


# 将用户自定义的源文件添加到可执行文件中
if(USER_SOURCES)
    target_sources(${CMAKE_PROJECT_NAME} PRIVATE
        ${USER_SOURCES}
    )
endif()

# 添加用户自定义的头文件路径
if(USER_INCLUDE_DIRS)
    target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
        ${USER_INCLUDE_DIRS}
    )
endif()

# 添加项目符号 (宏定义)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # 在这里添加用户自定义的符号
)

# 移除在使用 cpp 文件时错误的 libob.a 库依赖
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)


# 链接所有需要的库
target_link_libraries(${CMAKE_PROJECT_NAME} 
    ${CMSIS_DSP_LIB} # 链接根据 MCU 选择的 CMSIS-DSP 库
    m                # 链接标准数学库 (例如 sin, cos, sqrt)
)

# 为 printf 启用浮点数支持
target_link_options(${CMAKE_PROJECT_NAME} PRIVATE "-Wl,-u_printf_float")



# 设置链接器搜索目录
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/ST/ARM/DSP/Lib
)


# --- 7. 后期构建步骤 ---
# 定义输出的 .hex 和 .bin 文件路径
set(HEX_FILE ${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}.hex)
set(BIN_FILE ${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}.bin)

# 添加自定义命令, 在构建完成后生成 .hex 和 .bin 文件
add_custom_command(
    TARGET ${CMAKE_PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -Oihex $<TARGET_FILE:${CMAKE_PROJECT_NAME}> ${HEX_FILE}
    COMMAND ${CMAKE_OBJCOPY} -Obinary $<TARGET_FILE:${CMAKE_PROJECT_NAME}> ${BIN_FILE}
    COMMENT "正在生成 ${HEX_FILE} 和 ${BIN_FILE}..."
)