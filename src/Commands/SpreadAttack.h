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

class SpreadAttackCommandClass : public PhobosCommandClass
{
public:
	// CommandClass
	virtual const char* GetName() const override
	{
		return "Spread to attack adjecant target";
	}

	virtual const wchar_t* GetUIName() const override
	{
		return GeneralUtils::LoadStringUnlessMissing("TXT_SPREAD", L"Spread Attack");
	}

	virtual const wchar_t* GetUICategory() const override
	{
		return StringTable::LoadString("TXT_SELECTION");
	}

	virtual const wchar_t* GetUIDescription() const override
	{
		return GeneralUtils::LoadStringUnlessMissing("TXT_SPREAD_DESC", L"Let selected units to attack adjecant target around their original target.");
	}

	static bool CanAttack(FootClass* src, TechnoClass* dest)
	{
		auto srcType = src->GetTechnoType();
		auto destType = dest->GetTechnoType();
		if (!srcType || !destType)
			return false;

		//auto pWeapon = src->GetWeapon(src->SelectWeapon(dest))->WeaponType;

		//int damage = damage = MapClass::GetTotalDamage(pWeapon->Damage, pWeapon->Warhead, destType->Armor, 0);
		//if (damage == 0)
		//	return false;

		if (dest->InWhichLayer() == Layer::Underground)
			return false;
		if (dest->CloakState == CloakState::Cloaked && !srcType->Naval)
			return false;
		if (dest->InLimbo || !dest->IsAlive || dest->BeingWarpedOut || dest->Absorbed || destType->Immune)
			return false;

		return true;
	}

	virtual void Execute(WWKey eInput) const override
	{
		// Debug::Log("[Phobos] Dummy command runs.\n");
		// MessageListClass::Instance->PrintMessage(L"[Phobos] Dummy command rums");
		int nCount = ObjectClass::CurrentObjects->Count;
		if (nCount <= 1)
			return;

		DynamicVectorClass<AbstractClass*> targets;
		for (int i = 0; i < nCount; i++)
		{
			auto pObj = ObjectClass::CurrentObjects->GetItem(i);
			auto pAttacker = abstract_cast<FootClass*>(pObj);
			if (pAttacker)
			{
				auto pTarget = abstract_cast<TechnoClass*>(pAttacker->Target);
				if (!pTarget)
					pTarget = abstract_cast<TechnoClass*>(pAttacker->Destination);
				if (!pTarget)
					continue;

				auto pTargetOwner = pTarget->Owner;
				auto pAttackerType = pAttacker->GetTechnoType();
				AbstractType targetType = pTarget->WhatAmI();
				if (targets.FindItemIndex(pTarget) == -1)
				{
					targets.AddItem(pTarget);
					continue;
				}
				int nRange = 15;
				//for (auto weaponType : pAttackerType->Weapon)
				//{
				//	if (weaponType.WeaponType)
				//	{
				//		nRange = weaponType.WeaponType->Range / Unsorted::LeptonsPerCell;
				//		break;
				//	}
				//}
				auto cells = GeneralUtils::AdjacentCellsInRange(nRange);
				int nFound = 0;
			search:
				for (int pos = 0; pos < cells.size(); pos++)
				{
					bool bQuit = false;
					CellStruct cell = pTarget->GetCell()->MapCoords + cells[pos];
					CellClass* targetCell = MapClass::Instance->GetCellAt(cell);
					TechnoClass* pCandidate = abstract_cast<TechnoClass*>(targetCell->FirstObject);
					while (pCandidate)
					{
						if ((pCandidate->Owner == pTargetOwner) && pCandidate->WhatAmI() == targetType)
						{
							if (targets.FindItemIndex(pCandidate) == -1)
							{
								if (pAttacker->WhatAmI() == AbstractType::Aircraft)
								{
									if (CanAttack(pAttacker, pCandidate))
									{
										targets.AddItem(pCandidate);
										pAttacker->SetTarget(pCandidate);
										pAttacker->SetDestination(pCandidate, false);
										pAttacker->QueueMission(Mission::Attack, true);
										bQuit = true;
										nFound += 2;
										break;
									}
								}
								else if (pAttacker->CurrentMission == Mission::Capture && pCandidate->WhatAmI() == AbstractType::Building)
								{
									auto pOrgType = abstract_cast<BuildingTypeClass*>(pTarget->GetType());
									auto pDestType = abstract_cast<BuildingTypeClass*>(pCandidate->GetType());
									if (pOrgType && pDestType)
									{
										if (pOrgType->MaxNumberOccupants > 0)
										{
											if (pDestType->MaxNumberOccupants > 0)
											{
												targets.AddItem(pCandidate);
												pAttacker->QueueMission(Mission::Capture, true);
												pAttacker->SetDestination(pCandidate, true);
												bQuit = true;
												nFound += 2;
												break;
											}
										}
										else
										{
											targets.AddItem(pCandidate);
											pAttacker->QueueMission(Mission::Capture, true);
											pAttacker->SetDestination(pCandidate, true);
											bQuit = true;
											nFound += 2;
											break;
										}
									}
								}
								else
								{
									if (CanAttack(pAttacker, pCandidate))
									{
										targets.AddItem(pCandidate);
										pAttacker->SetTarget(pCandidate);
										pAttacker->QueueMission(Mission::Attack, true);
										bQuit = true;
										nFound += 2;
										break;
									}
								}
							}
						}
						pCandidate = abstract_cast<TechnoClass*>(pCandidate->NextObject);
					}

					if (bQuit)
						break;
				}
				if (nFound == 0)
				{
					targets.Clear();
					nFound--;
					goto search;
				}
			}
		}
	}
};
