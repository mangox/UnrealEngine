// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"
#include "ActorPositioning.h"
#include "SnappingUtils.h"
#include "Landscape/LandscapeHeightfieldCollisionComponent.h"
#include "Landscape/LandscapeComponent.h"

FActorPositionTraceResult FActorPositioning::TraceWorldForPositionWithDefault(const FViewportCursorLocation& Cursor, const FSceneView& View, const TArray<AActor*>* IgnoreActors)
{
	FActorPositionTraceResult Results = TraceWorldForPosition(Cursor, View, IgnoreActors);
	if (Results.State == FActorPositionTraceResult::Failed)
	{
		Results.State = FActorPositionTraceResult::Default;

		// And put it in front of the camera
		const float DistanceMultiplier = ( Cursor.GetViewportType() == LVT_Perspective ) ? GetDefault<ULevelEditorViewportSettings>()->BackgroundDropDistance : 0.0f;
		Results.Location = Cursor.GetOrigin() + Cursor.GetDirection() * DistanceMultiplier;
	}
	return Results;
}

FActorPositionTraceResult FActorPositioning::TraceWorldForPosition(const FViewportCursorLocation& Cursor, const FSceneView& View, const TArray<AActor*>* IgnoreActors)
{
	const auto* ViewportClient = Cursor.GetViewportClient();

	FActorPositionTraceResult Results;

	const auto ViewportType = ViewportClient->GetViewportType();

	// Start with a ray that encapsulates the entire world
	FVector RayStart = Cursor.GetOrigin();
	if (ViewportType == LVT_OrthoXY || ViewportType == LVT_OrthoXZ || ViewportType == LVT_OrthoYZ)
	{
		RayStart -= Cursor.GetDirection() * HALF_WORLD_MAX/2;
	}

	const FVector RayEnd = RayStart + Cursor.GetDirection() * HALF_WORLD_MAX;
	return TraceWorldForPosition(*ViewportClient->GetWorld(), View, RayStart, RayEnd, IgnoreActors);
}

/** Check to see if the specified hit result should be ignored from actor positioning calculations for the specified scene view */
bool IsHitIgnored(const FHitResult& InHit, const FSceneView& InSceneView)
{
	const auto* Actor = InHit.GetActor();
	
	// Try and find a primitive component for the hit
	const UPrimitiveComponent* PrimitiveComponent = Actor ? Actor->GetRootPrimitiveComponent() : nullptr;

	if (!PrimitiveComponent)
	{
		PrimitiveComponent = InHit.Component.Get();
	}
	if (PrimitiveComponent && PrimitiveComponent->IsA(ULandscapeHeightfieldCollisionComponent::StaticClass()))
	{
		PrimitiveComponent = CastChecked<ULandscapeHeightfieldCollisionComponent>(PrimitiveComponent)->RenderComponent.Get();
	}

	if (InHit.bStartPenetrating || !PrimitiveComponent)
	{
		return true;
	}

	// Ignore volumes and shapes
	if (Actor && Actor->IsA(AVolume::StaticClass()))
	{
		return true;
	}
	else if (PrimitiveComponent->IsA(UShapeComponent::StaticClass()))
	{
		return true;
	}

	// Only use this component if it is visible in the specified scene views
	bool bIsRenderedOnScreen = false;
	{				
		if (PrimitiveComponent && PrimitiveComponent->SceneProxy)
		{
			const FPrimitiveViewRelevance ViewRelevance = PrimitiveComponent->SceneProxy->GetViewRelevance(&InSceneView);
			// BSP is a bit special in that its bDrawRelevance is false even when drawn as wireframe because InSceneView.Family->EngineShowFlags.BSPTriangles is off
			bIsRenderedOnScreen = ViewRelevance.bDrawRelevance || (PrimitiveComponent->IsA(UModelComponent::StaticClass()) && InSceneView.Family->EngineShowFlags.BSP);
		}
	}

	return !bIsRenderedOnScreen;
}

FActorPositionTraceResult FActorPositioning::TraceWorldForPosition(const UWorld& InWorld, const FSceneView& InSceneView, const FVector& RayStart, const FVector& RayEnd, const TArray<AActor*>* IgnoreActors)
{
	TArray<FHitResult> Hits;

	FCollisionQueryParams Param(TEXT("DragDropTrace"), true);
	if (IgnoreActors)
	{
		Param.AddIgnoredActors(*IgnoreActors);
	}

	FActorPositionTraceResult Results;
	if ( InWorld.LineTraceMulti(Hits, RayStart, RayEnd, Param, FCollisionObjectQueryParams(FCollisionObjectQueryParams::InitType::AllObjects)) )
	{
		{
			// Filter out anything that should be ignored
			FSuspendRenderingThread SuspendRendering(false);
			Hits.RemoveAll([&](const FHitResult& Hit){
				return IsHitIgnored(Hit, InSceneView);
			});
		}

		// Go through all hits and find closest
		float ClosestHitDistance = TNumericLimits<float>::Max();

		for (const auto& Hit : Hits)
		{
			const float DistanceToHit = (Hit.ImpactPoint - RayStart).Size();
			if (DistanceToHit < ClosestHitDistance)
			{
				ClosestHitDistance = DistanceToHit;
				Results.Location = Hit.Location;
				Results.SurfaceNormal = Hit.Normal.SafeNormal();
				Results.State = FActorPositionTraceResult::HitSuccess;
				Results.HitActor = Hit.Actor;
			}
		}
	}

	return Results;
}

FTransform FActorPositioning::GetCurrentViewportPlacementTransform(const AActor& Actor, bool bSnap)
{
	FVector Collision = Actor.GetPlacementExtent();
	const UActorFactory* Factory = GEditor->FindActorFactoryForActorClass(Actor.GetClass());

	// Get cursor origin and direction in world space.
	FViewportCursorLocation CursorLocation = GCurrentLevelEditingViewportClient->GetCursorWorldLocationFromMousePos();
	const auto CursorPos = CursorLocation.GetCursorPos();

	FTransform ActorTransform = FTransform::Identity;

	if (CursorLocation.GetViewportType() == LVT_Perspective && !GCurrentLevelEditingViewportClient->Viewport->GetHitProxy( CursorPos.X, CursorPos.Y ))
	{
		ActorTransform.SetTranslation(FActorPositioning::GetActorPositionInFrontOfCamera(Actor, CursorLocation.GetOrigin(), CursorLocation.GetDirection()));
	}
	else if (bSnap)
	{
		ActorTransform = FActorPositioning::GetSnappedSurfaceAlignedTransform(GCurrentLevelEditingViewportClient, Factory, GEditor->ClickLocation, GEditor->ClickPlane, Actor.GetPlacementExtent());
	}
	else
	{
		ActorTransform = FActorPositioning::GetSurfaceAlignedTransform(Factory, GEditor->ClickLocation, GEditor->ClickPlane, Actor.GetPlacementExtent());
	}

	return ActorTransform;
}

FVector FActorPositioning::GetActorPositionInFrontOfCamera(const AActor& InActor, const FVector& InCameraOrigin, const FVector& InCameraDirection)
{
	// Get the  radius of the actors bounding cylinder.  Height is not needed.
	float CylRadius, CylHeight;
	InActor.GetComponentsBoundingCylinder(CylRadius, CylHeight);

	// a default cylinder radius if no bounding cylinder exists.  
	const float	DefaultCylinderRadius = 50.0f;

	if( CylRadius == 0.0f )
	{
		// If the actor does not have a bounding cylinder, use a default value.
		CylRadius = DefaultCylinderRadius;
	}

	// The new location the cameras origin offset by the actors bounding cylinder radius down the direction of the cameras view. 
	FVector NewLocation = InCameraOrigin + InCameraDirection * CylRadius + InCameraDirection * GetDefault<ULevelEditorViewportSettings>()->BackgroundDropDistance;

	// Snap the new location if snapping is enabled
	FSnappingUtils::SnapPointToGrid( NewLocation, FVector::ZeroVector );
	return NewLocation;
}

FTransform FActorPositioning::GetSurfaceAlignedTransform(const UActorFactory* InFactory, const FVector& InSurfaceLocation, const FVector& SurfaceNormal, const FVector& InPlacementExtent, const FTransform& InStartTransform)
{
	// Sort out the rotation first, then do the location
	FQuat RotatorQuat = InStartTransform.GetRotation();
	if (InFactory && GetDefault<ULevelEditorViewportSettings>()->SnapToSurface.bSnapRotation)
	{
		RotatorQuat = InFactory->AlignObjectToSurfaceNormal(SurfaceNormal, RotatorQuat);
	}

	// Choose the largest location offset of the various options (global viewport settings, collision, factory offset)
	const float SnapOffsetExtent = GetDefault<ULevelEditorViewportSettings>()->SnapToSurface.SnapOffsetExtent;
	const float CollisionOffsetExtent = FVector::BoxPushOut(SurfaceNormal, InPlacementExtent);

	FVector LocationOffset = SurfaceNormal * FMath::Max(SnapOffsetExtent, CollisionOffsetExtent);
	if (InFactory && LocationOffset.SizeSquared() < InFactory->SpawnPositionOffset.SizeSquared())
	{
		// Rotate the Spawn Position Offset to match our rotation
		LocationOffset = RotatorQuat.RotateVector(-InFactory->SpawnPositionOffset);
	}

	return FTransform(RotatorQuat, InSurfaceLocation + LocationOffset);
}

FTransform FActorPositioning::GetSnappedSurfaceAlignedTransform(FLevelEditorViewportClient* InViewportClient, const UActorFactory* InFactory, FVector InSurfaceLocation, const FVector& SurfaceNormal, const FVector& InPlacementExtent, const FTransform& InStartTransform)
{
	FSnappingUtils::SnapPointToGrid(InSurfaceLocation, FVector(0.f));

	bool bDrawVertexHelpers = false;
	// Secondly, attempt vertex snapping
	FVector AlignToNormal;
	if (!InViewportClient || !FSnappingUtils::SnapLocationToNearestVertex( InSurfaceLocation, InViewportClient->GetDropPreviewLocation(), InViewportClient, AlignToNormal, bDrawVertexHelpers ))
	{
		AlignToNormal = SurfaceNormal;
	}

	return GetSurfaceAlignedTransform(InFactory, InSurfaceLocation, AlignToNormal, InPlacementExtent, InStartTransform);
}