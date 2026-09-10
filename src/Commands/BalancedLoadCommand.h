#pragma once

#include "Commands.h"

class BalancedLoadCommandClass : public CommandClass
{
public:
	// Iteration caps, so malformed data cannot spin forever
	static constexpr int MaxAssignRounds = 16;    // Rounds of round-robin assignment
	static constexpr int MaxSwapIterations = 64;  // Local-search iterations for swapping same-type passengers
	static constexpr int MaxDispatchRounds = 3;   // Rounds of "dispatch, then re-plan the failures" in one frame
	static constexpr int MaxPathQueries = 512;    // Path distance query budget; beyond it, straight-line estimates are used
	static constexpr int LeptonsPerCell = 256;    // 1 cell = 256 leptons, matching the path distance scale
	static constexpr int PathfindImpossible = 0x7FFFFFFF;  // AttemptPath result when no path exists

	virtual const char* GetName() const override;
	virtual const wchar_t* GetUIName() const override;
	virtual const wchar_t* GetUICategory() const override;
	virtual const wchar_t* GetUIDescription() const override;
	virtual void Execute(WWKey eInput) const override;
};
