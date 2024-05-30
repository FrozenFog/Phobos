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

class CancelTeam : public PhobosCommandClass
{
public:
	virtual const char* GetName() const override
	{
		return "Cancel Out Team";
	}

	virtual const wchar_t* GetUIName() const override
	{
		return GeneralUtils::LoadStringUnlessMissing("TXT_DISLOAD", L"Cancel Team");
	}

	virtual const wchar_t* GetUICategory() const override
	{
		return StringTable::LoadString("TXT_SELECTION");
	}

	virtual const wchar_t* GetUIDescription() const override
	{
		return GeneralUtils::LoadStringUnlessMissing("TXT_DISLOAD_DESC", L"Cancel current selected unit's team info");
	}

	virtual void Execute(WWKey eInput) const override
	{
		int nCount = ObjectClass::CurrentObjects->Count;

		if (nCount > 0)
		{
			for (int i = 0; i < nCount; i++)
			{
				auto pUnit = abstract_cast<TechnoClass*>(ObjectClass::CurrentObjects->GetItem(i));
				pUnit->Group = -1;
			}
		}
	}
};
