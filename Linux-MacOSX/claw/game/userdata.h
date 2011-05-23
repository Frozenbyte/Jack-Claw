#ifndef USERDATA_H
#define USERDATA_H

#include <string>

std::string getUserDataPrefix();
std::string mapUserDataPrefix(const std::string &path);
std::string unmapUserDataPrefix(const std::string &path);
void initializeUserData();

#endif
