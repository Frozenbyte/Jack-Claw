// Copyright 2002-2004 Frozenbyte Ltd.

// from 1700

#define GS_CMD_BASE 1700

GS_CMD(0, GS_CMD_SETPOSITION, "setPosition", STRING)
GS_CMD(1, GS_CMD_POSITIONRANDOMOFFSET, "positionRandomOffset", INT)
GS_CMD(2, GS_CMD_MOVEPOSITIONZ, "movePositionZ", INT)
GS_CMD(3, GS_CMD_MOVEPOSITIONX, "movePositionX", INT)
GS_CMD(4, GS_CMD_GETPOSITIONZ, "getPositionZ", NONE)
GS_CMD(5, GS_CMD_GETPOSITIONX, "getPositionX", NONE)
GS_CMD(6, GS_CMD_GETACCURATEPOSITIONX, "getAccuratePositionX", NONE)
GS_CMD(7, GS_CMD_GETACCURATEPOSITIONZ, "getAccuratePositionZ", NONE)
GS_CMD(8, GS_CMD_SETPOSITIONHEIGHT, "setPositionHeight", STRING)
GS_CMD(9, GS_CMD_SETPOSITIONHEIGHTONGROUND, "setPositionHeightOnGround", NONE)
GS_CMD(10, GS_CMD_POSITIONACCURATERANDOMOFFSET, "positionAccurateRandomOffset", INT)
GS_CMD(11, GS_CMD_MOVEPOSITIONHEIGHT, "movePositionHeight", INT)

GS_CMD(12, GS_CMD_SETSECONDARYPOSITION, "setSecondaryPosition", NONE)
GS_CMD(13, GS_CMD_GETSECONDARYPOSITION, "getSecondaryPosition", NONE)

GS_CMD(14, GS_CMD_SETPOSITIONX, "setPositionX", NONE)
GS_CMD(15, GS_CMD_SETPOSITIONZ, "setPositionZ", NONE)
GS_CMD(16, GS_CMD_SETACCURATEPOSITIONX, "setAccuratePositionX", NONE)
GS_CMD(17, GS_CMD_SETACCURATEPOSITIONZ, "setAccuratePositionZ", NONE)

GS_CMD(18, GS_CMD_PUSHGLOBALTEMPPOSITION, "pushGlobalTempPosition", NONE)
GS_CMD(19, GS_CMD_POPGLOBALTEMPPOSITION, "popGlobalTempPosition", NONE)

GS_CMD(20, GS_CMD_MOVEPOSITIONTOANGLEVALUE, "movePositionToAngleValue", INT)

GS_CMD(21, GS_CMD_GETPOSITIONHEIGHT, "getPositionHeight", NONE)

GS_CMD(22, GS_CMD_MOVEPOSITIONZFLOAT, "movePositionZFloat", FLOAT)
GS_CMD(23, GS_CMD_MOVEPOSITIONXFLOAT, "movePositionXFloat", FLOAT)

GS_CMD(24, GS_CMD_ISPOSITIONINSIDEBUILDING, "isPositionInsideBuilding", NONE)

GS_CMD_SIMPLE(25, setPositionVariable, STRING)
GS_CMD_SIMPLE(26, getPositionVariable, STRING)

GS_CMD_SIMPLE(27, isPositionBlockedByUnmoving, NONE)
GS_CMD_SIMPLE(28, isPositionBlockedByMoving, NONE)
GS_CMD_SIMPLE(29, isPositionBlockedByUnmovingOrDoor, NONE)

GS_CMD_SIMPLE(30, setPositionKeepingHeight, STRING)
GS_CMD_SIMPLE(31, setPositionXToFloat, FLOAT)
GS_CMD_SIMPLE(32, setPositionZToFloat, FLOAT)

GS_CMD_SIMPLE(33, addPositionVariableToPosition, STRING)

GS_CMD_SIMPLE(34, setAccuratePositionHeight, NONE)
GS_CMD_SIMPLE(35, getAccuratePositionHeight, NONE)

GS_CMD_SIMPLE(36, rayTraceToSecondaryPosition, STRING)

#undef GS_CMD_BASE

// up to 1749
