// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"
#include "FoliageTypeFactory.h"
#include "IAssetTypeActions.h"
#include "FoliageType_InstancedStaticMesh.h"

UFoliageTypeFactory::UFoliageTypeFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = UFoliageType_InstancedStaticMesh::StaticClass();
}

UObject* UFoliageTypeFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	auto NewFoliageType = NewObject<UFoliageType_InstancedStaticMesh>(InParent, Class, Name, Flags | RF_Transactional);

	return NewFoliageType;
}

uint32 UFoliageTypeFactory::GetMenuCategories() const
{
	return EAssetTypeCategories::Misc;
}