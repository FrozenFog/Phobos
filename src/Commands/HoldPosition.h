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

class HoldPositionCommandClass : public PhobosCommandClass
{
public:
	// CommandClass
	virtual const char* GetName() const override
	{
		return "Hold in current position";
	}

	virtual const wchar_t* GetUIName() const override
	{
		return GeneralUtils::LoadStringUnlessMissing("TXT_SPREAD", L"Hold Position");
	}

	virtual const wchar_t* GetUICategory() const override
	{
		return StringTable::LoadString("TXT_SELECTION");
	}

	virtual const wchar_t* GetUIDescription() const override
	{
		return GeneralUtils::LoadStringUnlessMissing("TXT_HOLD_DESC", L"Let selected units hold in their position.");
	}

	virtual void Execute(WWKey eInput) const override
	{
		// Debug::Log("[Phobos] Dummy command runs.\n");
		// MessageListClass::Instance->PrintMessage(L"[Phobos] Dummy command rums");
		int nCount = ObjectClass::CurrentObjects->Count;
		if (nCount == 0)
			return;

		for (int i = 0; i < nCount; i++)
		{
			auto pObj = ObjectClass::CurrentObjects->GetItem(i);
			auto pUnit = abstract_cast<FootClass*>(pObj);
			if (pUnit)
			{
				pUnit->QueueMission(Mission::Sticky, true);
			}
		}
	}
};
