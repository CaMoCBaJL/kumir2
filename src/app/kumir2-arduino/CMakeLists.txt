project(kumir2-arduino)
cmake_minimum_required(VERSION 3.0)

find_package(Kumir2 REQUIRED)

kumir2_add_launcher(
    NAME            kumir2-arduino
    CONFIGURATION   "ActorArduino(tablesOnly),!KumirCompilerTool,ArduinoCodeGenerator,KumirAnalizer\(teacher,preload=Files,preload=Strings\)"
)

