// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameFramework/Volume.h"
#include "MeshMergeCullingVolume.generated.h"

/** A volume that can be added a level in order to remove triangles from source meshes before generating HLOD or merged meshes */
UCLASS(Experimental, MinimalAPI)
class AMeshMergeCullingVolume : public AVolume
{
	GENERATED_UCLASS_BODY()

public:

};
