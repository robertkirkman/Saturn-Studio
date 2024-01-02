#include "saturn/saturn_colors.h"

#include <string>
#include <iostream>
#include <vector>
#include <SDL2/SDL.h>

#include "saturn/saturn.h"
#include "saturn/imgui/saturn_imgui.h"
#include "saturn/saturn_textures.h"
#include "saturn/saturn_actors.h"

#include "saturn/libs/imgui/imgui.h"
#include "saturn/libs/imgui/imgui_internal.h"
#include "saturn/libs/imgui/imgui_impl_sdl.h"
#include "saturn/libs/imgui/imgui_impl_opengl3.h"

#include "pc/configfile.h"

extern "C" {
#include "game/camera.h"
#include "game/level_update.h"
#include "sm64.h"
}

#include <dirent.h>
#include <filesystem>
#include <fstream>
#include <assert.h>
#include <stdlib.h>
#include <array>
namespace fs = std::filesystem;
#include "pc/fs/fs.h"

std::vector<std::string> color_code_list;
std::vector<std::string> model_color_code_list;

GameSharkCode current_color_code;

// Palette as used by GFX
ColorTemplate defaultColorHat               {255, 127, 0,   0,   0,   0  };
ColorTemplate defaultColorOveralls          {0,   0,   0,   0,   255, 127};
ColorTemplate defaultColorGloves            {255, 127, 255, 127, 255, 127};
ColorTemplate defaultColorShoes             {114, 57,  28,  14,  14,  7  };
ColorTemplate defaultColorSkin              {254, 127, 193, 96,  121, 60 };
ColorTemplate defaultColorHair              {115, 57,  6,   3,   0,   0  };

ColorTemplate sparkColorShirt               {255, 127, 255, 127, 0,   0  };
ColorTemplate sparkColorShoulders           {0,   0,   255, 127, 255, 127};
ColorTemplate sparkColorArms                {0,   0,   255, 127, 127, 64 };
ColorTemplate sparkColorOverallsBottom      {255, 127, 0,   0,   255, 127}; 
ColorTemplate sparkColorLegTop              {255, 127, 0,   0,   127, 64 };
ColorTemplate sparkColorLegBottom           {127, 64,  0,   0,   255, 127};

std::string HexifyColorTemplate(ColorTemplate &colorBodyPart) {
    char colour[64];
    ImFormatString(colour, IM_ARRAYSIZE(colour), "%02X%02X%02X%02X%02X%02X"
            , ImClamp((int)colorBodyPart.red[0], 0, 255)
            , ImClamp((int)colorBodyPart.green[0], 0, 255)
            , ImClamp((int)colorBodyPart.blue[0], 0, 255)
            , ImClamp((int)colorBodyPart.red[1], 0, 255)
            , ImClamp((int)colorBodyPart.green[1], 0, 255)
            , ImClamp((int)colorBodyPart.blue[1], 0, 255));
    std::string col = colour;
    return col;
}

ColorTemplate chromaColor                   {0,   0,   255, 255, 0,   0  };

std::string last_model_cc_address;

/* Global toggle for Saturn's color code compatibility. True by default */
bool support_color_codes = true;
/* Global toggle for Saturn's SPARK extension. False by default */
bool support_spark = true;

void PasteGameShark(std::string GameShark, ColorCode cc) {
    std::istringstream f(GameShark);
    std::string line;
        
    while (std::getline(f, line)) {
        if (line.rfind("81", 0) == 0) {
            int address = std::stoi(line.substr(2, 6), 0, 16);
            int value1 = std::stoi(line.substr(9, 2), 0, 16);
            int value2 = std::stoi(line.substr(11, 2), 0, 16);

            switch(address) {

                /* Address (80000000 + Offset)  RED     BLUE
                                                GREEN           */

                // Cap
                case 0x07EC40:  cc[CC_HAT].red[0] = value1;        cc[CC_HAT].green[0] = value2;          break;
                case 0x07EC42:  cc[CC_HAT].blue[0] = value1;                                              break;
                case 0x07EC38:  cc[CC_HAT].red[1] = value1;        cc[CC_HAT].green[1] = value2;          break;
                case 0x07EC3A:  cc[CC_HAT].blue[1] = value1;                                              break;
                // Overalls
                case 0x07EC28:  cc[CC_OVERALLS].red[0] = value1;   cc[CC_OVERALLS].green[0] = value2;     break;
                case 0x07EC2A:  cc[CC_OVERALLS].blue[0] = value1;                                         break;
                case 0x07EC20:  cc[CC_OVERALLS].red[1] = value1;   cc[CC_OVERALLS].green[1] = value2;     break;
                case 0x07EC22:  cc[CC_OVERALLS].blue[1] = value1;                                         break;
                // Gloves
                case 0x07EC58:  cc[CC_GLOVES].red[0] = value1;     cc[CC_GLOVES].green[0] = value2;       break;
                case 0x07EC5A:  cc[CC_GLOVES].blue[0] = value1;                                           break;
                case 0x07EC50:  cc[CC_GLOVES].red[1] = value1;     cc[CC_GLOVES].green[1] = value2;       break;
                case 0x07EC52:  cc[CC_GLOVES].blue[1] = value1;                                           break;
                // Shoes
                case 0x07EC70:  cc[CC_SHOES].red[0] = value1;      cc[CC_SHOES].green[0] = value2;        break;
                case 0x07EC72:  cc[CC_SHOES].blue[0] = value1;                                            break;
                case 0x07EC68:  cc[CC_SHOES].red[1] = value1;      cc[CC_SHOES].green[1] = value2;        break;
                case 0x07EC6A:  cc[CC_SHOES].blue[1] = value1;                                            break;
                // Skin
                case 0x07EC88:  cc[CC_SKIN].red[0] = value1;       cc[CC_SKIN].green[0] = value2;         break;
                case 0x07EC8A:  cc[CC_SKIN].blue[0] = value1;                                             break;
                case 0x07EC80:  cc[CC_SKIN].red[1] = value1;       cc[CC_SKIN].green[1] = value2;         break;
                case 0x07EC82:  cc[CC_SKIN].blue[1] = value1;                                             break;
                // Hair
                case 0x07ECA0:  cc[CC_HAIR].red[0] = value1;       cc[CC_HAIR].green[0] = value2;         break;
                case 0x07ECA2:  cc[CC_HAIR].blue[0] = value1;                                             break;
                case 0x07EC98:  cc[CC_HAIR].red[1] = value1;       cc[CC_HAIR].green[1] = value2;         break;
                case 0x07EC9A:  cc[CC_HAIR].blue[1] = value1;                                             break;

                // (Optional) SPARK Addresses

                // Shirt
                case 0x07ECB8:  cc[CC_SHIRT].red[0] = value1;            cc[CC_SHIRT].green[0] = value2;          break;
                case 0x07ECBA:  cc[CC_SHIRT].blue[0] = value1;                                                    break;
                case 0x07ECB0:  cc[CC_SHIRT].red[1] = value1;            cc[CC_SHIRT].green[1] = value2;          break;
                case 0x07ECB2:  cc[CC_SHIRT].blue[1] = value1;                                                    break;
                // Shoulders
                case 0x07ECD0:  cc[CC_SHOULDERS].red[0] = value1;        cc[CC_SHOULDERS].green[0] = value2;      break;
                case 0x07ECD2:  cc[CC_SHOULDERS].blue[0] = value1;                                                break;
                case 0x07ECC8:  cc[CC_SHOULDERS].red[1] = value1;        cc[CC_SHOULDERS].green[1] = value2;      break;
                case 0x07ECCA:  cc[CC_SHOULDERS].blue[1] = value1;                                                break;
                // Arms
                case 0x07ECE8:  cc[CC_ARMS].red[0] = value1;             cc[CC_ARMS].green[0] = value2;           break;
                case 0x07ECEA:  cc[CC_ARMS].blue[0] = value1;                                                     break;
                case 0x07ECE0:  cc[CC_ARMS].red[1] = value1;             cc[CC_ARMS].green[1] = value2;           break;
                case 0x07ECE2:  cc[CC_ARMS].blue[1] = value1;                                                     break;
                // Pelvis
                case 0x07ED00:  cc[CC_OVERALLS_BOTTOM].red[0] = value1;  cc[CC_OVERALLS_BOTTOM].green[0] = value2; break;
                case 0x07ED02:  cc[CC_OVERALLS_BOTTOM].blue[0] = value1;                                           break;
                case 0x07ECF8:  cc[CC_OVERALLS_BOTTOM].red[1] = value1;  cc[CC_OVERALLS_BOTTOM].green[1] = value2; break;
                case 0x07ECFA:  cc[CC_OVERALLS_BOTTOM].blue[1] = value1;                                           break;
                // Thighs
                case 0x07ED18:  cc[CC_LEG_TOP].red[0] = value1;          cc[CC_LEG_TOP].green[0] = value2;         break;
                case 0x07ED1A:  cc[CC_LEG_TOP].blue[0] = value1;                                                   break;
                case 0x07ED10:  cc[CC_LEG_TOP].red[1] = value1;          cc[CC_LEG_TOP].green[1] = value2;         break;
                case 0x07ED12:  cc[CC_LEG_TOP].blue[1] = value1;                                                   break;
                // Calves
                case 0x07ED30:  cc[CC_LEG_BOTTOM].red[0] = value1;       cc[CC_LEG_BOTTOM].green[0] = value2;      break;
                case 0x07ED32:  cc[CC_LEG_BOTTOM].blue[0] = value1;                                                break;
                case 0x07ED28:  cc[CC_LEG_BOTTOM].red[1] = value1;       cc[CC_LEG_BOTTOM].green[1] = value2;      break;
                case 0x07ED2A:  cc[CC_LEG_BOTTOM].blue[1] = value1;                                                break;
            }
        }
    }
}

/*
    Applies a ColorCode to the game, overwriting the vanilla palette.
*/
void ApplyColorCode(GameSharkCode colorCode) {
    current_color_code = colorCode;
    //PasteGameShark(colorCode.GameShark);
}

std::vector<std::string> GetColorCodeList(std::string folderPath) {
    std::vector<std::string> cc_list;

    if (folderPath.find("dynos/packs") != std::string::npos) {
        if (fs::exists(folderPath + "/../default.gs")) {
            cc_list.push_back("../default.gs");
        }
        
        if (fs::exists(folderPath)) {
            for (const auto & entry : fs::directory_iterator(folderPath)) {
                fs::path path = entry.path();

                if (path.filename().u8string() != "Mario") {
                    if (path.extension().u8string() == ".gs")
                        cc_list.push_back(path.filename().u8string());
                }
            }
        }
    } else {
        if (fs::exists(folderPath)) {
            cc_list.push_back("Mario.gs");

            for (const auto & entry : fs::directory_iterator(folderPath)) {
                fs::path path = entry.path();
                
                if (fs::is_directory(path)) continue;
                if (path.filename().u8string() != "Mario") {
                    if (path.extension().u8string() == ".gs")
                        cc_list.push_back(path.filename().u8string());
                }
            }
        }
    }

    return cc_list;
}

GameSharkCode LoadGSFile(std::string fileName, std::string filePath) {
    GameSharkCode colorCode;

    if (fileName == "../default.gs") {
        filePath = fs::path(filePath).parent_path().u8string();
        fileName = "default.gs";
    }

    if (fileName != "Mario.gs") {
        std::ifstream file(filePath + "/" + fileName, std::ios::in | std::ios::binary);
        if (file.good()) {
            std::cout << "Loaded CC file: " << filePath << "/" << fileName << std::endl;

            // Read GS File
            const std::size_t& size = std::filesystem::file_size(filePath + "/" + fileName);
            std::string content(size, '\0');
            file.read(content.data(), size);
            file.close();

            // Write to CC
            colorCode.Name = fileName.substr(0, fileName.size() - 3);
            colorCode.GameShark = content;
            colorCode.IsModel = (filePath.find("dynos/packs") != std::string::npos);

        } else {
            // Load failed; Refresh active list
            std::cout << "Failed to load " << fileName.c_str() << std::endl;
            if (filePath.find("dynos/packs") != std::string::npos) model_color_code_list = GetColorCodeList(filePath);
            else color_code_list = GetColorCodeList(filePath);
        }
    }

    // Change conflicting file names
    if (colorCode.Name == "Mario" || colorCode.Name == "default")
        colorCode.Name = "Sample";

    return colorCode;
}

void SaveGSFile(GameSharkCode colorCode, std::string filePath) {
    // Change conflicting file names
    if (colorCode.Name == "Mario" || colorCode.Name == "default")
        colorCode.Name = "Sample";

    // Create "/colorcodes" directory if it doesn't already exist
    if (!fs::exists(filePath))
        fs::create_directory(filePath + "/../colorcodes");

    std::ofstream file(filePath + "/" + colorCode.Name + ".gs");
    file << colorCode.GameShark;
}

void DeleteGSFile(std::string filePath) {
    if (fs::exists(filePath))
        fs::remove(filePath);
}

void saturn_refresh_cc_count() {
    //cc_details = "" + std::to_string(model_color_code_list.size()) + " color code";
    //if (model_color_code_list.size() != 1) cc_details += "s";
}