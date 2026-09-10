#include "BalancedLoadCommand.h"

#include <BuildingTypeClass.h>
#include <MessageListClass.h>
#include <MapClass.h>
#include <ObjectClass.h>
#include <AStarClass.h>

#include <Utilities/GeneralUtils.h>
#include <Utilities/Debug.h>
#include <Ext/Techno/Body.h>
#include <Ext/TechnoType/Body.h>

#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <utility>
#include <vector>

#define TECHNO_IS_ALIVE(tech) (!tech->InLimbo && tech->Health > 0)

const char* BalancedLoadCommandClass::GetName() const
{
	return "Balanced load into transport";
}

const wchar_t* BalancedLoadCommandClass::GetUIName() const
{
	return GeneralUtils::LoadStringUnlessMissing("TXT_BALANCED_LOAD", L"Balanced Load");
}

const wchar_t* BalancedLoadCommandClass::GetUICategory() const
{
	return CATEGORY_SELECTION;
}

const wchar_t* BalancedLoadCommandClass::GetUIDescription() const
{
	return GeneralUtils::LoadStringUnlessMissing("TXT_BALANCED_LOAD_DESC", L"One-click balanced loading: evenly distribute selected passengers among the selected transports.");
}

void BalancedLoadCommandClass::Execute(WWKey eInput) const
{
	int nCount = ObjectClass::CurrentObjects.Count;
	if (nCount == 0)
		return;

	// A transport that cannot be entered manually is not a transport: sitting on a building, under a chrono
	// effect (being warped out, phasing in, temporally locked or immobilized), or not ours. Rejecting it while
	// collecting keeps it out of the SizeLimit baseline and the capacity tally instead of refusing it late.
	auto isUsableTransport = [](FootClass* pUnit) -> bool
	{
		auto pCell = pUnit->GetCell();

		return !(pCell && pCell->GetBuilding())
			&& !pUnit->IsBeingWarpedOut()
			&& !pUnit->WarpingOut
			&& !pUnit->TemporalTargetingMe
			&& !pUnit->IsImmobilized
			&& pUnit->Owner && pUnit->Owner->IsControlledByCurrentPlayer();
	};

	// Mouse-click check (can this passenger be ordered into that target?), cached; ignoreForce = true.
	std::unordered_map<FootClass*, std::unordered_map<FootClass*, char>> enterCheckCache;  // 1 = can enter manually, 2 = cannot

	auto canManuallyEnter = [&](FootClass* pPassenger, FootClass* pTransport) -> bool
	{
		auto& cache = enterCheckCache[pPassenger];
		auto cached = cache.find(pTransport);

		if (cached == cache.end())
		{
			const char enterable = pPassenger->MouseOverObject(pTransport, true) == Action::Enter ? char(1) : char(2);
			cached = cache.emplace(pTransport, enterable).first;
		}

		return cached->second == 1;
	};

	// A unit no other selected unit can be ordered into by a mouse click is not a manual-load transport, even
	// if it carries passengers. Mouse commands are the yardstick here, not SizeLimit, so this gate runs before the
	// SizeLimit baseline is taken.
	auto canBeEnteredManually = [&](FootClass* pUnit) -> bool
	{
		for (int i = 0; i < nCount; ++i)
		{
			auto pOther = abstract_cast<FootClass*>(ObjectClass::CurrentObjects.GetItem(i));

			if (pOther && pOther != pUnit && canManuallyEnter(pOther, pUnit))
				return true;
		}

		return false;
	};

	// First pass: take the largest SizeLimit among manually enterable transports as the baseline for "unit that can carry"
	double maxSizeLimit = 0;
	for (int i = 0; i < nCount; ++i)
	{
		auto pUnit = abstract_cast<FootClass*>(ObjectClass::CurrentObjects.GetItem(i));

		if (!pUnit || !isUsableTransport(pUnit) || !canBeEnteredManually(pUnit))
			continue;

		maxSizeLimit = std::max(maxSizeLimit, pUnit->GetTechnoType()->SizeLimit);
	}

	if (maxSizeLimit <= 0)
		return;

	// Second pass: units whose SizeLimit equals the maximum are transports; the rest (not aircraft, alive) are passengers grouped by type
	DynamicVectorClass<FootClass*> transports;
	std::unordered_map<FootClass*, int> remainingSpaces;             // Remaining cargo capacity of each transport
	std::unordered_map<TechnoTypeClass*, std::vector<FootClass*>> poolByType;  // Unassigned passengers, grouped by type
	std::vector<TechnoTypeClass*> typeOrder;                         // Fixed type order, so every type spreads over the same transports

	for (int i = 0; i < nCount; ++i)
	{
		auto pUnit = abstract_cast<FootClass*>(ObjectClass::CurrentObjects.GetItem(i));

		if (!pUnit)
			continue;

		auto pType = pUnit->GetTechnoType();

		if (pType->SizeLimit == maxSizeLimit)
		{
			// An unusable transport is skipped entirely, never demoted to a passenger
			if (isUsableTransport(pUnit) && canBeEnteredManually(pUnit))
			{
				transports.AddItem(pUnit);
				remainingSpaces[pUnit] = pType->Passengers - pUnit->Passengers.GetTotalSize();
			}
		}
		else if (pType->WhatAmI() != AbstractType::AircraftType
			&& !pType->ConsideredAircraft
			&& TECHNO_IS_ALIVE(pUnit))
		{
			if (poolByType.find(pType) == poolByType.end())
				typeOrder.push_back(pType);

			poolByType[pType].push_back(pUnit);
		}
	}

	if (transports.Count == 0 || typeOrder.empty())
		return;

	// ---- Cost function: zone-level path length divided by the current runtime speed
	std::unordered_map<FootClass*, std::unordered_map<FootClass*, double>> costCache;
	int pathQueries = 0;

	auto travelCost = [&](FootClass* pPassenger, FootClass* pTransport) -> double
	{
		auto& cache = costCache[pPassenger];
		auto cached = cache.find(pTransport);

		if (cached != cache.end())
			return cached->second;

		double cost = INFINITY;

		if (pathQueries < MaxPathQueries)
		{
			CellStruct from = pPassenger->GetMapCoords();
			CellStruct to = pTransport->GetMapCoords();
			bool fromOnBridge = pPassenger->OnBridge;
			bool toOnBridge = pTransport->OnBridge;

			++pathQueries;

			int distance = AStarClass::Instance.AttemptPath(&from, &to, pPassenger, fromOnBridge, toOnBridge, MovementZone::None);

			if (distance != PathfindImpossible)
			{
				int speed = pPassenger->GetCurrentSpeed();   // Current runtime speed (house, veteran, multipliers and the like included)
				cost = static_cast<double>(distance) / static_cast<double>(std::max(speed, 1));
			}
			else
			{
				// Fall back to a straight-line estimate.
				double fallback = static_cast<double>(pPassenger->DistanceFrom(pTransport)) / LeptonsPerCell;
				int speed = pPassenger->GetCurrentSpeed();
				cost = fallback / static_cast<double>(std::max(speed, 1));
			}
		}
		else
		{
			// Query budget exhausted: fall back to a straight-line estimate (converted to cells)
			double distance = static_cast<double>(pPassenger->DistanceFrom(pTransport)) / LeptonsPerCell;
			int speed = pPassenger->GetCurrentSpeed();
			cost = static_cast<double>(distance) / static_cast<double>(std::max(speed, 1));
		}

		cache[pTransport] = cost;
		return cost;
	};

	// ---- Assignment: type-aligned round-robin spread, each transport picking its shortest-travelling passenger
	std::vector<std::pair<FootClass*, FootClass*>> plan;   // (passenger, transport)

	for (auto pType : typeOrder)
	{
		auto& pool = poolByType[pType];
		const int unitSize = std::max(1, static_cast<int>(pType->Size));

		for (int round = 0; round < MaxAssignRounds; ++round)
		{
			bool progressed = false;

			for (auto pTransport : transports)   // The transport order is fixed, so each type spreads over the same transports
			{
				if (pool.empty())
					break;

				if (remainingSpaces[pTransport] < unitSize)
					continue;

				if (unitSize > static_cast<int>(pTransport->GetTechnoType()->SizeLimit))
					continue;

				// Transport's point of view: pick the passenger a mouse click can load and that takes the least time
				auto best = pool.end();
				double bestCost = INFINITY;

				for (auto it = pool.begin(); it != pool.end(); ++it)
				{
					// Use the real mouse-click path for the permission check
					if (!canManuallyEnter(*it, pTransport))
						continue;

					double cost = travelCost(*it, pTransport);

					if (cost < bestCost)
					{
						bestCost = cost;
						best = it;
					}
				}

				if (best == pool.end() || !(bestCost < INFINITY))
					continue;

				plan.emplace_back(*best, pTransport);
				remainingSpaces[pTransport] -= unitSize;
				pool.erase(best);
				progressed = true;
			}

			if (!progressed)
				break;
		}
	}

	// ---- Local optimization: swap same-type passengers between transports (per-transport load unchanged)
	for (int iteration = 0; iteration < MaxSwapIterations; ++iteration)
	{
		bool improved = false;

		for (size_t i = 0; i < plan.size(); ++i)
		{
			for (size_t j = i + 1; j < plan.size(); ++j)
			{
				auto pPassengerA = plan[i].first;
				auto pTransportA = plan[i].second;
				auto pPassengerB = plan[j].first;
				auto pTransportB = plan[j].second;

				if (pTransportA == pTransportB)
					continue;

				if (pPassengerA->GetTechnoType() != pPassengerB->GetTechnoType())
					continue;

				double before = travelCost(pPassengerA, pTransportA) + travelCost(pPassengerB, pTransportB);
				double after = travelCost(pPassengerA, pTransportB) + travelCost(pPassengerB, pTransportA);

				if (after < before)
				{
					std::swap(plan[i].second, plan[j].second);
					improved = true;
				}
			}
		}

		if (!improved)
			break;
	}

	// ---- Dispatch in one frame: like the player left-clicking the transport; refused orders are re-planned
	std::vector<std::pair<FootClass*, FootClass*>> pending = std::move(plan);
	std::unordered_map<FootClass*, std::vector<FootClass*>> blockedTargets;   // Pairs the engine already refused (passenger -> transport)

	for (int dispatch = 0; dispatch < MaxDispatchRounds && !pending.empty(); ++dispatch)
	{
		std::vector<std::pair<FootClass*, FootClass*>> rejected;

		for (auto& entry : pending)
		{
			auto pPassenger = entry.first;
			auto pTransport = entry.second;

			// Already entering (ordered earlier by the player, or taken over by the engine): do not dispatch again.
			if (pPassenger->GetCurrentMission() == Mission::Enter)
				continue;

			if (!canManuallyEnter(pPassenger, pTransport))
			{
				rejected.emplace_back(pPassenger, pTransport);
				continue;
			}

			// Execute through the real mouse-click path: ObjectClickedAction(Enter); a false return means the engine refused it.
			if (!pPassenger->ObjectClickedAction(Action::Enter, pTransport, false))
				rejected.emplace_back(pPassenger, pTransport);
		}

		if (rejected.empty())
			break;

		// Re-plan: give the reserved capacity back, then move each refused passenger to a transport with room
		pending.clear();

		for (auto& entry : rejected)
		{
			auto pPassenger = entry.first;
			auto pTransport = entry.second;
			const int unitSize = std::max(1, static_cast<int>(pPassenger->GetTechnoType()->Size));

			remainingSpaces[pTransport] += unitSize;
			blockedTargets[pPassenger].push_back(pTransport);   // Remember the refused target so re-planning does not pick it again

			double bestCost = INFINITY;
			FootClass* bestTransport = nullptr;

			for (auto pCandidate : transports)
			{
				if (remainingSpaces[pCandidate] < unitSize)
					continue;

				if (unitSize > static_cast<int>(pCandidate->GetTechnoType()->SizeLimit))
					continue;

				if (!canManuallyEnter(pPassenger, pCandidate))
					continue;

				auto& blocked = blockedTargets[pPassenger];

				if (std::find(blocked.begin(), blocked.end(), pCandidate) != blocked.end())
					continue;

				double cost = travelCost(pPassenger, pCandidate);

				if (cost < bestCost)
				{
					bestCost = cost;
					bestTransport = pCandidate;
				}
			}

			if (bestTransport)
			{
				remainingSpaces[bestTransport] -= unitSize;
				pending.emplace_back(pPassenger, bestTransport);
			}
			// No usable transport: the passenger stays where it is
		}
	}
}
