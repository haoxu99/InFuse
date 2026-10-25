cmake_minimum_required(VERSION 3.21)
project(InFuse VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
find_package(OpenCV 4 REQUIRED COMPONENTS core imgproc)

add_executable(infuse
    opp/infuse_main.cpp opp/data.cpp opp/graph.cpp opp/input.cpp opp/polygon.cpp
    opp/layer_graph.cpp opp/fusion.cpp opp/Merge_Graph.cpp opp/TRA_Graph.cpp
    slice/config.cpp slice/gcode.cpp slice/GeneralMesh.cpp slice/katana.cpp
    slice/slicer.cpp slice/stl.cpp slice/Vector3.cpp slice/visual.cpp
    slice/clipper.cpp
)
target_include_directories(infuse PRIVATE opp slice ${OpenCV_INCLUDE_DIRS})
target_link_libraries(infuse PRIVATE ${OpenCV_LIBS})
if(MSVC)
    target_compile_options(infuse PRIVATE /utf-8 /W3 /EHsc)
    target_compile_definitions(infuse PRIVATE _CRT_SECURE_NO_WARNINGS NOMINMAX)
    target_link_options(infuse PRIVATE /STACK:52428800)
else()
    target_compile_options(infuse PRIVATE -Wall -Wextra)
endif()
add_custom_target(infuse_assets ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/config.ini" "$<TARGET_FILE_DIR:infuse>/config.ini"
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:infuse>/slice"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/slice/gcode_header.txt"
        "${CMAKE_CURRENT_SOURCE_DIR}/slice/gcode_footer.txt" "$<TARGET_FILE_DIR:infuse>/slice"
    DEPENDS infuse
)
if(WIN32 AND TARGET opencv_world)
    add_custom_command(TARGET infuse POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:opencv_world>" "$<TARGET_FILE_DIR:infuse>")
endif()
include(CTest)
if(BUILD_TESTING)
    find_package(Python3 COMPONENTS Interpreter)
    if(Python3_Interpreter_FOUND)
        add_test(NAME pipeline_smoke COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/smoke.py" "$<TARGET_FILE:infuse>" "${CMAKE_CURRENT_SOURCE_DIR}")
        set_tests_properties(pipeline_smoke PROPERTIES TIMEOUT 900)
        add_test(NAME source_distribution COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/source_distribution.py")
        add_test(NAME paper_result_export COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/paper_result.py")
    endif()
endif()
