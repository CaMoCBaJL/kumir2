#ifndef CHECK_STEPS_H
#define CHECK_STEPS_H
namespace ArduinoPlugin {
    enum ArduinoPluginCheckState {
        InstallationCheck,
        AvailabilityCheck,
        CliUnavailable,
        SearchForBoads,
        NoBoardsFound,
        AnyBoardsFound,
        CheckCompleted
    };
}
#endif // CHECK_STEPS_H
