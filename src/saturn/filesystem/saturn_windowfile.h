#ifndef SaturnWindowFile
#define SaturnWindowFile

#include <string>
#include <map>

void saturn_load_window_visibility(char* filename, std::map<std::string, bool>* windows);
void saturn_save_window_visibility(char* filename, std::map<std::string, bool>* windows);

#endif