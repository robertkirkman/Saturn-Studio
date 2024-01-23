#ifndef SaturnImGuiCcEditor
#define SaturnImGuiCcEditor

#ifdef __cplusplus
    #include <string>
    #include "saturn/libs/imgui/imgui.h"
    #include "saturn/saturn_models.h"
    #include "saturn/saturn_actors.h"

    extern int uiCcListId;

    extern void UpdateEditorFromPalette();

    extern void ResetColorCode(bool);
    extern void RefreshColorCodeList();

    extern void OpenCCSelector(MarioActor* actor);
    extern void OpenModelCCSelector(Model, std::vector<std::string>, std::string);
    extern void OpenCCEditor(MarioActor* actor);

    extern bool has_open_any_model_cc;

    extern ImVec4 uiColors[][2];
#endif

#endif