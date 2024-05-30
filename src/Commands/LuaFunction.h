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

static AbstractClass* GetItemByUID(DWORD dwId)
{
	for (auto iter = AbstractClass::Array->begin(); iter != AbstractClass::Array->end(); ++iter)
	{
		auto pItem = *iter;
		if (pItem != nullptr && pItem->UniqueID == dwId)
		{
			return pItem;
		}
	}
	return nullptr;
}


static int GetAllSelectedUnit(lua_State* L)
{
	lua_newtable(L);
	int nCount = ObjectClass::CurrentObjects->Count;
	for (int i = 0; i < nCount; i++)
	{
		lua_pushinteger(L, ObjectClass::CurrentObjects->GetItem(i)->UniqueID);
		lua_rawseti(L, -2, i + 1);
	}
	return 1;
}
static int GetPositionOfUnit(lua_State* L)
{
	DWORD dwId = luaL_checkinteger(L, 1);
	auto pUnit = abstract_cast<FootClass*>(GetItemByUID(dwId));
	if (pUnit)
	{
		lua_pushnumber(L, pUnit->GetMapCoords().X);
		lua_pushnumber(L, pUnit->GetMapCoords().Y);
		lua_pushboolean(L, true);
	}
	else
	{
		lua_pushnumber(L, -1);
		lua_pushnumber(L, -1);
		lua_pushboolean(L, false);
	}
	return 3;
}
static int GetPositionOfUnitTarget(lua_State* L)
{
	DWORD dwId = luaL_checkinteger(L, 1);
	auto pUnit = abstract_cast<FootClass*>(GetItemByUID(dwId));
	if (pUnit)
	{
		auto pTarget = pUnit->Target;
		auto pTargetTechno = abstract_cast<FootClass*>(pTarget);
		if (pTargetTechno)
		{
			lua_pushnumber(L, pTargetTechno->GetMapCoords().X);
			lua_pushnumber(L, pTargetTechno->GetMapCoords().Y);
			lua_pushboolean(L, true);
		}
	}
	lua_pushnumber(L, -1);
	lua_pushnumber(L, -1);
	lua_pushboolean(L, false);
	return 3;
}
static int GetObjectTypeLua(lua_State* L)
{
	DWORD dwId = luaL_checkinteger(L, 1);
	auto pObject = GetItemByUID(dwId);
	if (pObject)
	{
		int id = (int)(pObject->AbsID);
		lua_pushnumber(L, id);
		lua_pushboolean(L, true);
	}
	else
	{
		lua_pushnumber(L, -1);
		lua_pushboolean(L, false);
	}
	return 2;
}
static int MoveUnitToPosition(lua_State* L)
{
	DWORD dwId = luaL_checkinteger(L, 1);
	int x = luaL_checkinteger(L, 2);
	int y = luaL_checkinteger(L, 3);
	auto pUnit = abstract_cast<FootClass*>(GetItemByUID(dwId));
	if (pUnit)
	{
		CellStruct cell;
		cell.X = x;
		cell.Y = y;
		auto coord = CellClass::Cell2Coord(cell);
		pUnit->MoveTo(&coord);
		lua_pushboolean(L, true);
		return 1;
	}
	lua_pushboolean(L, false);
	return 1;
}
static int GetUnitHp(lua_State* L)
{
	DWORD dwId = luaL_checkinteger(L, 1);
	auto pUnit = abstract_cast<ObjectClass*>(GetItemByUID(dwId));
	if (pUnit)
	{
		lua_pushinteger(L, pUnit->Health);
		lua_pushinteger(L, pUnit->GetType()->Strength);
		lua_pushboolean(L, true);
	}
	else
	{
		lua_pushinteger(L, -1);
		lua_pushinteger(L, -1);
		lua_pushboolean(L, false);
	}
	return 3;
}
static int SetUnitGroup(lua_State* L)
{
	DWORD dwId = luaL_checkinteger(L, 1);
	int group = luaL_checkinteger(L, 2);
	auto pUnit = abstract_cast<TechnoClass*>(GetItemByUID(dwId));
	if (pUnit)
	{
		pUnit->Group = group;
		lua_pushboolean(L, true);
	}
	else
	{
		lua_pushboolean(L, false);
	}
	return 1;
}
static int CancelSelect(lua_State* L)
{
	DWORD dwId = luaL_checkinteger(L, 1);
	auto pUnit = abstract_cast<ObjectClass*>(GetItemByUID(dwId));
	if (pUnit)
	{
		pUnit->Deselect();
		lua_pushboolean(L, true);
	}
	else
	{
		lua_pushboolean(L, false);
	}
	return 1;
}
