#ifndef SaturnColors
#define SaturnColors

#include <stdio.h>
#include <stdbool.h>

// Struct definition:
/*    
    struct ColorTemplate {
        unsigned int red[2];
        unsigned int green[2];
        unsigned int blue[2];
    };
*/
// [0] for light
// [1] for dark

struct ColorTemplate {
    unsigned int red[2];
    unsigned int green[2];
    unsigned int blue[2];
};

#define CC_HAT             0
#define CC_OVERALLS        1
#define CC_GLOVES          2
#define CC_SHOES           3
#define CC_SKIN            4
#define CC_HAIR            5

#define CC_SHIRT           6
#define CC_SHOULDERS       7
#define CC_ARMS            8
#define CC_OVERALLS_BOTTOM 9
#define CC_LEG_TOP         10
#define CC_LEG_BOTTOM      11

typedef struct ColorTemplate ColorCode[12];

extern struct ColorTemplate chromaColor;

#ifdef __cplusplus

#include <vector>
#include <string>
#include "saturn/saturn_models.h"
#include "saturn/libs/imgui/imgui_internal.h"

#define DEFAULT_VANILLA_GS_CODE "8107EC40 FF00\n8107EC42 0000\n8107EC38 7F00\n8107EC3A 0000\n" \
                                "8107EC28 0000\n8107EC2A FF00\n8107EC20 0000\n8107EC22 7F00\n"  \
                                "8107EC58 FFFF\n8107EC5A FF00\n8107EC50 7F7F\n8107EC52 7F00\n"   \
                                "8107EC70 721C\n8107EC72 0E00\n8107EC68 390E\n8107EC6A 0700\n"    \
                                "8107EC88 FEC1\n8107EC8A 7900\n8107EC80 7F60\n8107EC82 3C00\n"     \
                                "8107ECA0 7306\n8107ECA2 0000\n8107EC98 3903\n8107EC9A 0000"
#define DEFAULT_SPARK_GS_CODE "\n8107ECB8 FFFF\n8107ECBA 0000\n8107ECB0 7F7F\n8107ECB2 0000\n" \
                                "8107ECD0 00FF\n8107ECD2 FF00\n8107ECC8 007F\n8107ECCA 7F00\n"  \
                                "8107ECE8 00FF\n8107ECEA 7F00\n8107ECE0 007F\n8107ECE2 4000\n"   \
                                "8107ED00 FF00\n8107ED02 FF00\n8107ECF8 7F00\n8107ECFA 7F00\n"    \
                                "8107ED18 FF00\n8107ED1A 7F00\n8107ED10 7F00\n8107ED12 4000\n"     \
                                "8107ED30 7F00\n8107ED32 FF00\n8107ED28 4000\n8107ED2A 7F00"
#define DEFAULT_COLOR_CODE DEFAULT_VANILLA_GS_CODE DEFAULT_SPARK_GS_CODE

extern std::string HexifyColorTemplate(ColorTemplate &colorBodyPart);

class GameSharkCode {
    public:
        std::string Name = "Sample";
        std::string GameShark = DEFAULT_COLOR_CODE;

        bool IsModel;

        /* Returns true if the CC colors match the vanilla palette */
        bool IsDefaultColors() {
            return
                this->GameShark == DEFAULT_VANILLA_GS_CODE ||
                this->GameShark == DEFAULT_COLOR_CODE;
        }
};

class MarioActor;

extern void PasteGameShark(std::string, ColorCode cc);
extern void ApplyColorCode(GameSharkCode, MarioActor*);

extern std::vector<std::string> GetColorCodeList(std::string);
extern GameSharkCode LoadGSFile(std::string, std::string);
extern void SaveGSFile(GameSharkCode, std::string);
extern void DeleteGSFile(std::string);

extern std::vector<std::string> color_code_list;
extern std::vector<std::string> model_color_code_list;

extern GameSharkCode current_color_code;

extern GameSharkCode ParseGameShark(MarioActor* actor);

#endif

extern bool support_color_codes;
extern bool support_spark;

#ifdef __cplusplus
#include <string>
#include <vector>

extern std::string last_model_cc_address;
void saturn_refresh_cc_count();

#endif

#endif
