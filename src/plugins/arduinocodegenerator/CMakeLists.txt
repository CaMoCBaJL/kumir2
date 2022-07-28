project(ArduinoCoreGenerator)
cmake_minimum_required(VERSION 3.0)

find_package(Kumir2 REQUIRED)

kumir2_use_qt(Core)

set(SOURCES
    arduinocodegeneratorplugin.cpp
    generator.cpp
)

add_subdirectory(enums/)
add_subdirectory(entities/)
add_subdirectory(generators/)

set(MOC_HEADERS
    arduinocodegeneratorplugin.h
    generator.h
)

kumir2_wrap_cpp(MOC_SOURCES ${MOC_HEADERS})

kumir2_add_plugin(
    NAME        ArduinoCodeGenerator
    SOURCES     ${MOC_SOURCES} ${SOURCES}
    LIBRARIES   ${QT_LIBRARIES} ExtensionSystem DataFormats ErrorMessages CodeGenerators
)
