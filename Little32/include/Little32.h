#ifndef Little32_h_
#define Little32_h_
#pragma once

#include "L32_Types.h"
#include "L32_String.h"

// Abstract interfaces
#include "L32_IDevice.h"
#include "L32_IMemoryMapped.h"
#include "L32_IMappedDevice.h"
#include "L32_ICore.h"

// Helpers
#include "L32_GUIButton.h"
#include "L32_IO.h"
#include "L32_Sprite.h"
#include "L32_ConfigParser.h"

// System
#include "L32_Computer.h"
#include "L32_DebugCore.h"
#include "L32_L32Assembler.h"
#include "L32_L32Core.h"

// Devices
#include "L32_CharDisplay.h"
#include "L32_ColourCharDisplay.h"
#include "L32_ComputerInfo.h"
#include "L32_KeyboardDevice.h"
#include "L32_NullDevice.h"
#include "L32_RAM.h"
#include "L32_ROM.h"

#endif