#include "saturn_windowfile.h"

#include "saturn_format.h"

std::map<std::string, bool>* save_to;

bool saturn_window_visibility_handler(SaturnFormatStream* stream, int version) {
    char buf[256];
    saturn_format_read_string(stream, buf, 255);
    std::string id = buf;
    bool visible = saturn_format_read_bool(stream);
    if (save_to->find(id) == save_to->end()) save_to->insert({ id, visible });
    else (*save_to)[id] = visible;
    return true;
}

void saturn_load_window_visibility(char* filename, std::map<std::string, bool>* windows) {
    save_to = windows;
    saturn_format_input(filename, "WNDV", {
        { "ENTR", saturn_window_visibility_handler }
    });
}

void saturn_save_window_visibility(char* filename, std::map<std::string, bool>* windows) {
    SaturnFormatStream _stream = saturn_format_output("WNDV", 1);
    SaturnFormatStream* stream = &_stream;
    for (auto& window : *windows) {
        saturn_format_new_section(stream, "ENTR");
        saturn_format_write_string(stream, (char*)window.first.c_str());
        saturn_format_write_bool(stream, window.second);
        saturn_format_close_section(stream);
    }
    saturn_format_write(filename, stream);
}