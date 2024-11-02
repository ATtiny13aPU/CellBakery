#pragma once

#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <OSL/include.h>

#define NOMINMAX
#undef min
#undef max

class Context;

#include "context.h"
#include "starting.h"
#include "control.h"
#include "compute.h"
#include "graphics.h"
#include "gui.h"