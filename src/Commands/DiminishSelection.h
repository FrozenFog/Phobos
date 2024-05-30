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


class DiminishSelectionCommandClass : public PhobosCommandClass
{
public:
	virtual const char* GetName() const override
	{
		return "Diminish selection by 1";
	}

	virtual const wchar_t* GetUIName() const override
	{
		return GeneralUtils::LoadStringUnlessMissing("TXT_DISLOAD", L"Diminish Selection");
	}

	virtual const wchar_t* GetUICategory() const override
	{
		return StringTable::LoadString("TXT_SELECTION");
	}

	virtual const wchar_t* GetUIDescription() const override
	{
		return GeneralUtils::LoadStringUnlessMissing("TXT_DISLOAD_DESC", L"Diminish current selection by 1, unordered");
	}

	virtual void Execute(WWKey eInput) const override
	{
		int nCount = ObjectClass::CurrentObjects->Count;

		if (nCount > 0)
		{
			// yeah
			ObjectClass::CurrentObjects[0]->Deselect();
		}
	}
};
