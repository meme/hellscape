#pragma once

#include <gcc-plugin.h>
#include <basic-block.h>
#include <tree.h>
#include <function.h>
#include <iostream>

bool toObfuscate(bool flag, function *f, std::string attribute);
