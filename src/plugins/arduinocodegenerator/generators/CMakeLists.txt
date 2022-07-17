cmake_minimum_required(VERSION 3.0)

find_package(Kumir2 REQUIRED)

kumir2_use_qt(Core)

set(SOURCES
        LoopsGenerator/LoopGenerator.cpp
        CommonMethodsProvider.cpp
        ExpressionCalculator.cpp
        )

set(MOC_HEADERS
        LoopsGenerator/LoopGenerator.hpp
        CommonMethodsProvider.hpp
        ExpressionCalculator.hpp
        )

kumir2_add_library(
        NAME        CodeGenerators
        SOURCES     ${SOURCES}
        LIBRARIES   ${QT_LIBRARIES} ErrorMessages
)
