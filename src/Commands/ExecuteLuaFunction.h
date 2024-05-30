#pragma once
#include <BuildingTypeClass.h>
#include <MessageListClass.h>
#include <MapClass.h>
#include <ObjectClass.h>

#include "Commands.h"
#include <Utilities/GeneralUtils.h>
#include <Utilities/Debug.h>
#include <Ext/Techno/Body.h>
#include <Ext/TechnoType/Body.h>
#include <EventClass.h>
#include <lua.hpp>
#include <lualib.h>
#include "LuaFunction.h"


class ExecuteLuaFunction : public PhobosCommandClass
{
public:
	virtual const char* GetName() const override
	{
		return "Execute lua script";
	}

	virtual const wchar_t* GetUIName() const override
	{
		return GeneralUtils::LoadStringUnlessMissing("TXT_EXECLUA", L"Execute Lua");
	}

	virtual const wchar_t* GetUICategory() const override
	{
		return StringTable::LoadString("TXT_SELECTION");
	}

	virtual const wchar_t* GetUIDescription() const override
	{
		return GeneralUtils::LoadStringUnlessMissing("TXT_EXECLUA_DESC", L"Execute lua script named main.lua");
	}

	virtual void Execute(WWKey eInput) const override
	{
		lua_State* L;
		if (L == nullptr)
		{
			L = luaL_newstate();
			luaL_openlibs(L);
			lua_register(L, "game_AllSelected", GetAllSelectedUnit);
			lua_register(L, "unit_getPos", GetPositionOfUnit);
			lua_register(L, "unit_getTargetPos", GetPositionOfUnitTarget);
			lua_register(L, "unit_moveTo", MoveUnitToPosition);
			lua_register(L, "obj_getType", GetObjectTypeLua);
			lua_register(L, "obj_getHp", GetUnitHp);
			lua_register(L, "obj_cancelSelect", CancelSelect);
			lua_register(L, "unit_setGroup", SetUnitGroup);
		}
		if (luaL_dofile(L, "main.lua") != 0)
		{
			auto err = lua_tostring(L, -1);

		}
	}
};


