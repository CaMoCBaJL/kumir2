project(kumir2-robots)
cmake_minimum_required(VERSION 3.0)

find_package(Kumir2 REQUIRED)

kumir2_add_launcher(
    NAME            kumir2-robots
    SPLASHSCREEN    "splashscreen-classic.png"
    WINDOW_ICON     "window-icon-classic.png"
    APP_ICON_NAME   "kumir2-classic"
    X_ICONS_DIR     "../../../app_icons/linux/hicolor"
    WIN_ICONS_DIR   "../../../app_icons/win32"
    X_NAME          "Kumir Robots Edition"
    X_NAME_ru       "Кумир для роботов"
    X_CATEGORIES    "Education,X-KDE-Edu-Misc"
    CONFIGURATION   "CourseManager,Editor,ActorArduino,ActorRobot,ActorPainter,KumirAnalizer\(preload=Files\),*CodeGenerator,KumirCodeRun(nobreakpoints),!CoreGUI\(notabs,nostartpage,nosessions\)"
)

