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
		/// this has insane amount of conditions that needs to test and block,
		/// like chono, subterrain, garrisoned, (invalid target anyway)
		/// submarine, anti-submarine, (special state vs anti special state)
		/// mindcontrol priority system, (priority system)
		/// 0 damage but still can attack, 1% 2% 3% versus type, (special attack value)
		/// ally, one-side-ally, cloak, shrouded, (different status in different POV)
		/// and your favorite AIRCRAFT (from and to, all above)
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

	static void SendEvent(TechnoClass* src, TechnoClass* target, bool setTarget, bool setDest, Mission mission)
	{
		TargetClass pThis(src);
		TargetClass pTarget(target);
		TargetClass pEmpty(nullptr);
		EventClass pEvent(src->Owner->ArrayIndex, pThis, mission, setTarget ? pTarget : pEmpty, setDest ? pTarget : pEmpty, pEmpty);
		EventClass::AddEvent(pEvent);
	}

	virtual void Execute(WWKey eInput) const override
	{
		// this is a HotKey Command, execute immediately after user input
		// so the cost can be mediocre 
		
		// Debug::Log("[Phobos] Dummy command runs.\n");
		// MessageListClass::Instance->PrintMessage(L"[Phobos] Dummy command runs");

		// Spread on 1 target is useless
		int nCount = ObjectClass::CurrentObjects->Count;
		if (nCount <= 1)
			return;

		DynamicVectorClass<AbstractClass*> targets;
		for (int i = 0; i < nCount; i++)
		{
			// foreach attacker
			// iterate until every attacker is assigned
			auto pObj = ObjectClass::CurrentObjects->GetItem(i);
			auto pAttacker = abstract_cast<FootClass*>(pObj);
			if (pAttacker)
			{
				// find target, must have a target
				auto pTarget = abstract_cast<TechnoClass*>(pAttacker->Target);
				if (!pTarget)
					pTarget = abstract_cast<TechnoClass*>(pAttacker->Destination);
				if (!pTarget)
					continue;

				auto pTargetOwner = pTarget->Owner;
				auto pAttackerType = pAttacker->GetTechnoType();
				AbstractType targetType = pTarget->WhatAmI();

				// if this target is not recorded,
				// record it, and don't spread on this
				if (targets.FindItemIndex(pTarget) == -1)
				{
					targets.AddItem(pTarget);
					continue;
				}
				// now the selected target is already been fired upon at least one of my unit
				// the attacker now is the second one who aimed on this, we need to spread it


				// make it const, or make it configable in rules.general or somewhere else
				int nRange = 15;

				// attempt to set range by attacker's weapon
				// acts like shit, player get confused by result, abandoned.
				// you can resore this and test yourself, trust me you won't like it
				//for (auto weaponType : pAttackerType->Weapon)
				//{
				//	if (weaponType.WeaponType)
				//	{
				//		nRange = weaponType.WeaponType->Range / Unsorted::LeptonsPerCell;
				//		break;
				//	}
				//}

				// built-in cell iterator
				// find nearest cell around selected target
				// noted that this won't work for aircraft TARGET, their position is magic
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
						// same owner and same type as the previous attack target
						// building-building, inf-inf etc
						if ((pCandidate->Owner == pTargetOwner) && pCandidate->WhatAmI() == targetType)
						{
							// this target is not recorded, aim and record it
							if (targets.FindItemIndex(pCandidate) == -1)
							{
								// aircraft is shit, separate from ordinary attack
								if (pAttacker->WhatAmI() == AbstractType::Aircraft)
								{
									if (CanAttack(pAttacker, pCandidate))
									{
										targets.AddItem(pCandidate);
										SendEvent(pAttacker, pCandidate, true, true, Mission::Attack);
										// found target
										bQuit = true;
										nFound += 2;
										break;
									}
								}
								// capture or occupy type, spread capture, essential for tech building and civilian building / battle bunker
								else if (pAttacker->CurrentMission == Mission::Capture && pCandidate->WhatAmI() == AbstractType::Building)
								{
									auto pOrgType = abstract_cast<BuildingTypeClass*>(pTarget->GetType());
									auto pDestType = abstract_cast<BuildingTypeClass*>(pCandidate->GetType());
									if (pOrgType && pDestType)
									{
										// occupy type
										if (pOrgType->MaxNumberOccupants > 0)
										{
											// can occupy
											if (pDestType->MaxNumberOccupants > 0)
											{
												targets.AddItem(pCandidate);
												SendEvent(pAttacker, pCandidate, false, true, Mission::Capture);
												bQuit = true;
												nFound += 2;
												break;
											}
										}
										// capture type
										else
										{
											targets.AddItem(pCandidate);
											SendEvent(pAttacker, pCandidate, false, true, Mission::Capture);
											bQuit = true;
											nFound += 2;
											break;
										}
									}
								}
								// ordinary attack
								else
								{
									if (CanAttack(pAttacker, pCandidate))
									{
										targets.AddItem(pCandidate);
										SendEvent(pAttacker, pCandidate, true, false, Mission::Attack);
										bQuit = true;
										nFound += 2;
										break;
									}
								}
							}
						}
						//p.next
						pCandidate = abstract_cast<TechnoClass*>(pCandidate->NextObject);
					}

					// found, quit, next attacker
					// otherwise continue test adjacent target
					if (bQuit)
						break;
				}
				// no targets found
				// means all targets within the range have been recorded (will have at least 1 valid target, the pTaraget)
				// clear the record, from now on, every target will now have max+1 of our unit aim them at once
				// go back and iterate cell again, still this attacker
				// at least three state (found, no found, quit), so no bool here. make it enum if you prefer
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
