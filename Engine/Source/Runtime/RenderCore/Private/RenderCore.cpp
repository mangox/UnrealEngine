// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	RenderCore.h: Render core module implementation.
=============================================================================*/

#include "RenderCore.h"
#include "UniformBuffer.h"
#include "ModuleManager.h"

IMPLEMENT_MODULE(FDefaultModuleImpl, RenderCore);

DEFINE_LOG_CATEGORY(LogRendererCore);

/*-----------------------------------------------------------------------------
	Stat declarations.
-----------------------------------------------------------------------------*/
// Cycle stats are rendered in reverse order from what they are declared in.
// They are organized so that stats at the top of the screen are earlier in the frame, 
// And stats that are indented are lower in the call hierarchy.

// The purpose of the SceneRendering stat group is to show where rendering thread time is going from a high level.
// It should only contain stats that are likely to track a lot of time in a typical scene, not edge case stats that are rarely non-zero.


// Amount of time measured by 'RenderViewFamily' that is not accounted for in its children stats
// Use a more detailed profiler (like an instruction trace or sampling capture on Xbox 360) to track down where this time is going if needed
DEFINE_STAT(STAT_RenderVelocities);
DEFINE_STAT(STAT_FinishRenderViewTargetTime);
DEFINE_STAT(STAT_CacheUniformExpressions);
DEFINE_STAT(STAT_TranslucencyDrawTime);
DEFINE_STAT(STAT_BeginOcclusionTestsTime);
// Use 'stat shadowrendering' to get more detail
DEFINE_STAT(STAT_ProjectedShadowDrawTime);
DEFINE_STAT(STAT_LightingDrawTime);
DEFINE_STAT(STAT_DynamicPrimitiveDrawTime);
DEFINE_STAT(STAT_StaticDrawListDrawTime);
DEFINE_STAT(STAT_BasePassDrawTime);
DEFINE_STAT(STAT_DepthDrawTime);
DEFINE_STAT(STAT_DynamicShadowSetupTime);
DEFINE_STAT(STAT_RenderQueryResultTime);
// Use 'stat initviews' to get more detail
DEFINE_STAT(STAT_InitViewsTime);
// Measures the time spent in RenderViewFamily_RenderThread
// Note that this is not the total rendering thread time, any other rendering commands will not be counted here
DEFINE_STAT(STAT_TotalSceneRenderingTime);
DEFINE_STAT(STAT_TotalGPUFrameTime);
DEFINE_STAT(STAT_PresentTime);

DEFINE_STAT(STAT_SceneLights);
DEFINE_STAT(STAT_MeshDrawCalls);
DEFINE_STAT(STAT_DynamicPathMeshDrawCalls);
DEFINE_STAT(STAT_StaticDrawListMeshDrawCalls);

DEFINE_STAT(STAT_SceneDecals);
DEFINE_STAT(STAT_Decals);

// Memory stats for tracking virtual allocations used by the renderer to represent the scene
// The purpose of these memory stats is to capture where most of the renderer allocated memory is going, 
// Not to track all of the allocations, and not to track resource memory (index buffers, vertex buffers, etc).


DEFINE_STAT(STAT_StaticDrawListMemory);
DEFINE_STAT(STAT_PrimitiveInfoMemory);
DEFINE_STAT(STAT_RenderingSceneMemory);
DEFINE_STAT(STAT_ViewStateMemory);
DEFINE_STAT(STAT_RenderingMemStackMemory);
DEFINE_STAT(STAT_LightInteractionMemory);

// The InitViews stats group contains information on how long visibility culling took and how effective it was

DEFINE_STAT(STAT_GatherShadowPrimitivesTime);
DEFINE_STAT(STAT_UpdateIndirectLightingCache);
DEFINE_STAT(STAT_SortStaticDrawLists);
DEFINE_STAT(STAT_InitDynamicShadowsTime);
DEFINE_STAT(STAT_PreRenderView);
DEFINE_STAT(STAT_UpdateStaticMeshesTime);
DEFINE_STAT(STAT_StaticRelevance);
DEFINE_STAT(STAT_ComputeViewRelevance);
DEFINE_STAT(STAT_OcclusionCull);
DEFINE_STAT(STAT_UpdatePrimitiveFading);
DEFINE_STAT(STAT_FrustumCull);
DEFINE_STAT(STAT_DecompressPrecomputedOcclusion);
DEFINE_STAT(STAT_ViewVisibilityTime);

DEFINE_STAT(STAT_ProcessedPrimitives);
DEFINE_STAT(STAT_CulledPrimitives);
DEFINE_STAT(STAT_StaticallyOccludedPrimitives);
DEFINE_STAT(STAT_OccludedPrimitives);
DEFINE_STAT(STAT_OcclusionQueries);
DEFINE_STAT(STAT_VisibleStaticMeshElements);
DEFINE_STAT(STAT_VisibleDynamicPrimitives);
DEFINE_STAT(STAT_IndirectLightingCacheUpdates);


// The ShadowRendering stats group shows what kind of shadows are taking a lot of rendering thread time to render
// Shadow setup is tracked in the InitViews group

DEFINE_STAT(STAT_RenderWholeSceneShadowProjectionsTime);
DEFINE_STAT(STAT_WholeSceneDynamicShadowDepthsTime);
DEFINE_STAT(STAT_WholeSceneStaticShadowDepthsTime);
DEFINE_STAT(STAT_WholeSceneStaticDrawListShadowDepthsTime);
DEFINE_STAT(STAT_RenderWholeSceneShadowDepthsTime);
DEFINE_STAT(STAT_RenderPerObjectShadowProjectionsTime);
DEFINE_STAT(STAT_RenderPerObjectShadowDepthsTime);

DEFINE_STAT(STAT_WholeSceneShadows);
DEFINE_STAT(STAT_PerObjectShadows);
DEFINE_STAT(STAT_PreShadows);
DEFINE_STAT(STAT_CachedPreShadows);
DEFINE_STAT(STAT_ShadowDynamicPathDrawCalls);

DEFINE_STAT(STAT_TranslucentInjectTime);
DEFINE_STAT(STAT_DirectLightRenderingTime);
DEFINE_STAT(STAT_LightRendering);

DEFINE_STAT(STAT_NumShadowedLights);
DEFINE_STAT(STAT_NumLightFunctionOnlyLights);
DEFINE_STAT(STAT_NumUnshadowedLights);
DEFINE_STAT(STAT_NumLightsInjectedIntoTranslucency);
DEFINE_STAT(STAT_NumLightsUsingTiledDeferred);
DEFINE_STAT(STAT_NumLightsUsingSimpleTiledDeferred);
DEFINE_STAT(STAT_NumLightsUsingStandardDeferred);

DEFINE_STAT(STAT_LightShaftsLights);

DEFINE_STAT(STAT_ParticleUpdateRTTime);
DEFINE_STAT(STAT_InfluenceWeightsUpdateRTTime);
DEFINE_STAT(STAT_GPUSkinUpdateRTTime);
DEFINE_STAT(STAT_CPUSkinUpdateRTTime);

DEFINE_STAT(STAT_RemoveSceneLightTime);
DEFINE_STAT(STAT_UpdateSceneLightTime);
DEFINE_STAT(STAT_AddSceneLightTime);

DEFINE_STAT(STAT_RemoveScenePrimitiveTime);
DEFINE_STAT(STAT_AddScenePrimitiveRenderThreadTime);
DEFINE_STAT(STAT_UpdatePrimitiveTransformRenderThreadTime);

DEFINE_STAT(STAT_RemoveScenePrimitiveGT);
DEFINE_STAT(STAT_AddScenePrimitiveGT);
DEFINE_STAT(STAT_UpdatePrimitiveTransformGT);

DEFINE_STAT(STAT_Scene_SetShaderMapsOnMaterialResources_RT);
DEFINE_STAT(STAT_Scene_UpdateStaticDrawListsForMaterials_RT);
DEFINE_STAT(STAT_GameToRendererMallocTotal);

DEFINE_STAT(STAT_UpdateLPVs);
DEFINE_STAT(STAT_ReflectiveShadowMaps);
DEFINE_STAT(STAT_ReflectiveShadowMapDrawTime);
DEFINE_STAT(STAT_NumReflectiveShadowMapLights);
DEFINE_STAT(STAT_RenderWholeSceneReflectiveShadowMapsTime);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)

static TAutoConsoleVariable<int32> CVarForceLOD(
	TEXT("r.ForceLOD"),
	-1,
	TEXT("LOD level to force, -1 is off."),
	ECVF_Cheat | ECVF_RenderThreadSafe
	);

#endif // !(UE_BUILD_SHIPPING || UE_BUILD_TEST)

/** Whether to pause the global realtime clock for the rendering thread (read and write only on main thread). */
bool GPauseRenderingRealtimeClock;

/** Global realtime clock for the rendering thread. */
FTimer GRenderingRealtimeClock;

FInputLatencyTimer GInputLatencyTimer( 2.0f );

//
// FInputLatencyTimer implementation.
//

/** Potentially starts the timer on the gamethread, based on the UpdateFrequency. */
void FInputLatencyTimer::GameThreadTick()
{
#if STATS
	if (FThreadStats::IsCollectingData())
	{
		if ( !bInitialized )
		{
			LastCaptureTime	= FPlatformTime::Seconds();
			bInitialized	= true;
		}
		float CurrentTimeInSeconds = FPlatformTime::Seconds();
		if ( (CurrentTimeInSeconds - LastCaptureTime) > UpdateFrequency )
		{
			LastCaptureTime		= CurrentTimeInSeconds;
			StartTime			= FPlatformTime::Cycles();
			GameThreadTrigger	= true;
		}
	}
#endif
}

TLinkedList<FUniformBufferStruct*>*& FUniformBufferStruct::GetStructList()
{
	static TLinkedList<FUniformBufferStruct*>* GUniformStructList = NULL;
	return GUniformStructList;
}

FUniformBufferStruct* FindUniformBufferStructByName(const TCHAR* StructName)
{
	for (TLinkedList<FUniformBufferStruct*>::TIterator StructIt(FUniformBufferStruct::GetStructList()); StructIt; StructIt.Next())
	{
		if (!FCString::Stricmp(StructIt->GetStructTypeName(), StructName))
		{
			return *StructIt;
		}
	}
	return NULL;
}

// Can be optimized to avoid the virtual function call but it's compiled out for final release anyway
RENDERCORE_API int32 GetCVarForceLOD()
{
	int32 Ret = -1;

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	{
		Ret = CVarForceLOD.GetValueOnRenderThread();
	}
#endif // !(UE_BUILD_SHIPPING || UE_BUILD_TEST)

	return Ret;
}
