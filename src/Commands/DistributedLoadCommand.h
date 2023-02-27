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

#define TECHNO_IS_ALIVE(tech) (!tech->InLimbo && tech->Health > 0)

class DistributedLoadCommandClass : public PhobosCommandClass
{
public:
	// CommandClass
	virtual const char* GetName() const override
	{
		return "Distributed load into transport";
	}

	virtual const wchar_t* GetUIName() const override
	{
		return GeneralUtils::LoadStringUnlessMissing("TXT_DISLOAD", L"Distributed Load");
	}

	virtual const wchar_t* GetUICategory() const override
	{
		return StringTable::LoadString("TXT_SELECTION");
	}

	virtual const wchar_t* GetUIDescription() const override
	{
		return GeneralUtils::LoadStringUnlessMissing("TXT_DISLOAD_DESC", L"Distributed load into transport, will auto detect passenger and vehicles.");
	}

	virtual void Execute(WWKey eInput) const override
	{
		const int T_NO_GATHER = 0, T_COUNTDOWN = 1, T_AVGPOS = 2;
		/*
		type 0: stop member from gathering and begin load now
		type 1: don't stop, maintain previous action, countdown LOWORD seconds then begin load
		type 2: don't stop, gether around first member's position range LOWORD then begin load
		*/

		// can proceed to load calculate logic
		const int R_CAN_PROCEED = 1;
		// type 1 waiting stage, start timer and wait timer stop, when stop, proceed to loading
		const int R_WAITING = 2;
		// type 2 waiting stage, start timer and wait timer stop, when stop, check average position
		const int R_WAIT_POS = 3;

		auto remainingSize = [](FootClass* src)
		{
			auto type = src->GetTechnoType();
			return type->Passengers - src->Passengers.GetTotalSize();
		};
		int nCount = ObjectClass::CurrentObjects->Count;
		if (nCount == 0)
			return;

	beginLoad:
		// Now we're talking
		DynamicVectorClass<FootClass*> transports, passengers;
		std::unordered_map<FootClass*, double> transportSpaces;
		// Find max SizeLimit to determine which type is considered as transport
		double maxSizeLimit = 0;
		for (int i = 0; i < nCount; i++)
		{
			auto pUnit = abstract_cast<FootClass*>(ObjectClass::CurrentObjects->GetItem(i));
			auto pType = pUnit->GetTechnoType();
			maxSizeLimit = std::max(maxSizeLimit, pType->SizeLimit);
		}
		for (int i = 0; i < nCount; i++)
		{
			auto pUnit = abstract_cast<FootClass*>(ObjectClass::CurrentObjects->GetItem(i));
			auto pType = pUnit->GetTechnoType();
			if (pType->SizeLimit == maxSizeLimit)
			{
				int space = remainingSize(pUnit);
				transports.AddItem(pUnit);
				transportSpaces[pUnit] = space;
			}
			else
				passengers.AddItem(pUnit);
		}
		// If there are no passengers
		// then this script is done
		if (passengers.Count == 0)
		{
			return;
		}
		// If transport is on building, scatter, and discard this frame
		for (auto pUnit : transports)
		{
			if (pUnit->GetCell()->GetBuilding())
			{
				pUnit->Scatter(pUnit->GetCoords(), true, false);
				return;
			}
		}

		// Load logic
		// range prioritize
		bool passengerLoading = false;
		// larger size first
		auto sizeSort = [](FootClass* a, FootClass* b)
		{
			return a->GetTechnoType()->Size > b->GetTechnoType()->Size;
		};
		std::sort(passengers.begin(), passengers.end(), sizeSort);
		for (auto pPassenger : passengers)
		{
			auto pPassengerType = pPassenger->GetTechnoType();
			// Is legal loadable unit ?
			if (pPassengerType->WhatAmI() != AbstractType::AircraftType &&
					!pPassengerType->ConsideredAircraft &&
					TECHNO_IS_ALIVE(pPassenger))
			{
				FootClass* targetTransport = nullptr;
				double distance = INFINITY;
				for (auto pTransport : transports)
				{
					auto pTransportType = pTransport->GetTechnoType();
					double currentSpace = transportSpaces[pTransport];
					// Can unit load onto this car ?
					if (currentSpace > 0 &&
						pPassengerType->Size > 0 &&
						pPassengerType->Size <= pTransportType->SizeLimit &&
						pPassengerType->Size <= currentSpace)
					{
						double d = pPassenger->DistanceFrom(pTransport);
						if (d < distance)
						{
							targetTransport = pTransport;
							distance = d;
						}
					}
				}
				// This is nearest available transport
				if (targetTransport)
				{
					// Get on the car
					if (pPassenger->GetCurrentMission() != Mission::Enter)
					{
						pPassenger->QueueMission(Mission::Enter, true);
						pPassenger->SetTarget(nullptr);
						pPassenger->SetDestination(targetTransport, false);
						transportSpaces[targetTransport] -= pPassengerType->Size;
						passengerLoading = true;
					}
				}
			}
		}
	}
};
