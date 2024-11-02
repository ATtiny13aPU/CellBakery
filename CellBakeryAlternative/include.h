#pragma once

#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <stdio.h>

#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

#define NOMINMAX
#undef min
#undef max

#include <OSL/include.h>

#include <filesystem>
namespace fs = std::filesystem;

class Context;

#include "context.h"
#include "starting.h"
#include "control.h"
#include "compute.h"
#include "graphics.h"
#include "gui.h"