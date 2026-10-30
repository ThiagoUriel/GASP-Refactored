// Ninja Bear Studio Inc., all rights reserved.
#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "GASPBlueprintMigrationLibrary.generated.h"

class UBlueprint;

/**
 * Repairs native component references when a Blueprint changes its native base class.
 */
UCLASS()
class GASPEDITOR_API UGASPBlueprintMigrationLibrary : public UBlueprintFunctionLibrary
{

	GENERATED_BODY()

public:

	/**
	 * Reconnects a class-default property to its existing named native component.
	 *
	 * Use during an asset migration when an obsolete serialized property has replaced a native
	 * reference. The component must already exist and match the property's declared class.
	 *
	 * @param Blueprint		Blueprint whose class defaults are being migrated.
	 * @param PropertyName		Native component property to repair.
	 * @param ComponentName	Stable default-subobject name assigned by the native constructor.
	 * @return					Whether the property resolves to a compatible existing component.
	 */
	UFUNCTION(BlueprintCallable, Category = "GASP|Migration")
	static bool RestoreDefaultComponentReference(UBlueprint* Blueprint, FName PropertyName, FName ComponentName);

};
