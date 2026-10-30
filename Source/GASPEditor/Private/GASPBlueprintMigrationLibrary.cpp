// Ninja Bear Studio Inc., all rights reserved.
#include "GASPBlueprintMigrationLibrary.h"

#include "Components/ActorComponent.h"
#include "Engine/Blueprint.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "UObject/UnrealType.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GASPBlueprintMigrationLibrary)

bool UGASPBlueprintMigrationLibrary::RestoreDefaultComponentReference(UBlueprint* Blueprint, const FName PropertyName, const FName ComponentName)
{
	if (!IsValid(Blueprint) || !IsValid(Blueprint->GeneratedClass))
	{
		return false;
	}
	UObject* Default = Blueprint->GeneratedClass->GetDefaultObject();
	FObjectPropertyBase* Property = FindFProperty<FObjectPropertyBase>(Blueprint->GeneratedClass, PropertyName);
	UActorComponent* Component = Cast<UActorComponent>(Default->GetDefaultSubobjectByName(ComponentName));
	if (Property == nullptr || !IsValid(Component) || !Component->IsA(Property->PropertyClass))
	{
		return false;
	}

	if (Property->GetObjectPropertyValue_InContainer(Default) != Component)
	{
		Blueprint->Modify();
		Default->Modify();
		Property->SetObjectPropertyValue_InContainer(Default, Component);
		FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
	}
	return true;
}
