// Ninja Bear Studio Inc., all rights reserved.
#pragma once

#include "Modules/ModuleManager.h"

/**
 * Hosts fork-specific integrations between GASP and Ninja Bear Studio systems.
 */
class FGASPNBSModule : public IModuleInterface
{

public:

	// -- Begin ModuleInterface implementation
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	// -- End ModuleInterface implementation

};
