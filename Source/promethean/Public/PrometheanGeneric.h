// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "PrometheanMesh.h"
#include "Engine/StaticMeshActor.h"
#include <limits> // infinity stuff

// --------------------------------------------------------------------
// --- Constants
// --------------------------------------------------------------------
const float NegativeInfinity = -std::numeric_limits<float>::infinity();
const int NegativeInfinityInt = -std::numeric_limits<int>::infinity();
const FVector NegativeInfinityVector = FVector(NegativeInfinity, NegativeInfinity, NegativeInfinity);
const FRotator NegativeInfinityRotator = FRotator(NegativeInfinity, NegativeInfinity, NegativeInfinity);

// --------------------------------------------------------------------
// --- Editor Functions
// --------------------------------------------------------------------
UWorld* GetEditorWorld();

// --------------------------------------------------------------------
// --- Transform Functions
// --------------------------------------------------------------------

void TransformActorsByName(const TArray<FString>& ObjectNames, FTransform Transform, UWorld* World);
void TransformActorByName(FString ObjectName, FTransform Transform, UWorld* World);

void TranslateActorsByName(const TArray<FString>& ObjectNames, FVector NewLocation, UWorld* World);
void TranslateActorByName(FString ObjectName, FVector NewLocation, UWorld* World);

void TranslateAndRayTraceActorsByName(const TArray<FString>& ObjectNames, FVector NewLocation, float RaytraceDistance, float MaxNormalDeviation, const TArray<FString>& IgnoreObjectNames, UWorld* World);
void TranslateAndRaytraceActorByName(FString ObjectName, FVector NewLocation, float RaytraceDistance, float MaxNormalDeviation, TArray<AActor*> IgnoreActors, UWorld* World);

void RelativeTranslateActorsByName(const TArray<FString>& ObjectNames, FVector LocationDelta, UWorld* World);
void RelativeTranslateActorByName(FString ObjectName, FVector LocationDelta, UWorld* World);

void RotateActorsByName(const TArray<FString>& ObjectNames, FVector NewRotationVec, UWorld* World);
void RotateActorByName(FString ObjectName, FVector NewRotationVec, UWorld* World);

void RelativeRotateActorsByName(const TArray<FString>& ObjectNames, FVector NewRotationVec, UWorld* World);
void RelativeRotateActorByName(FString ObjectName, FVector NewRotationVec, UWorld* World);

void ScaleActorsByName(const TArray<FString>& ObjectNames, FVector NewScale, UWorld* World);
void ScaleActorByName(FString ObjectName, FVector NewScale, UWorld* World);

void RelativeScaleActorsByName(const TArray<FString>& ObjectNames, FVector NewScale, UWorld* World);
void RelativeScaleActorByName(FString ObjectName, FVector NewScale, UWorld* World);

void RemoveActorByName(FString ObjectName, UWorld* World);
void RemoveActorsByName(const TArray<FString>& ObjectNames, UWorld* World);
void RemoveDescendentsFromActorsByName(const TArray<FString>& ObjectNames, UWorld* World);
void RemoveDescendentsFromActorByName(FString ObjectName, UWorld* World);
void RemoveSelectedActors();

FString GetValidActorsTransformJSONDict(const TArray<AActor*>& ValidActors);
FString GetValidActorsExpandedTransformJSONDict(const TArray<AActor*>& ValidActors, UWorld* World);
FString GetValidActorsLocationJSONDict(const TArray<AActor*>& ValidActors);
FString GetValidActorsBottomCenterLocationJSONDict(const TArray<AActor*>& ValidActors);

FString getStaticMeshActorsTransformJSONDictByName(const TArray<FString>& ObjectNames, UWorld* World);
FString getStaticMeshActorsTransformJSONDict(const TArray<AStaticMeshActor*>& StaticMeshActors);

FString getCameraInfoJSONDict(bool ObjectsOnScreen = true);

// --------------------------------------------------------------------
// --- Utility Functions
// --------------------------------------------------------------------

// Main function to check the validity
AActor* ValidateActor(AActor* Actor);

// Find valid actors
AActor* GetValidActorByName(const FString& ActorName, UWorld* World);

// Extract components
USceneComponent* GetValidSceneComponent(AActor* Actor);
UMeshComponent* GetValidMeshComponent(AActor* Actor);
UStaticMeshComponent* GetValidStaticMeshComponent(AActor* Actor);

// Named
TArray<AActor*> GetNamedValidActors(const TArray<FString>& ActorNames, UWorld* World);

// Actors in the world
TArray<AActor*> GetWorldValidActors(UWorld* World);

// Selected
TArray<AActor*> GetSelectedValidActors();

// Rendered
TArray<AActor*> GetRenderedValidActors();

// Using asset path
TArray<AActor*> GetValidActorsByMeshAssetPath(const FString& AssetPath, UWorld* World);

// Using material path
TArray<AActor*> GetValidActorsByMaterialPath(const FString& MaterialPath, UWorld* World);

// --------------------------------------------------------------------
// --- Material Functions
// --------------------------------------------------------------------

// set material attributes
void SetVecMatAttrsForValidActor(AActor* Actor, FString AttributeName, FLinearColor Value, int Index);
void SetScalarMatAttrsForValidActor(AActor* Actor, FString AttributeName, float Value);
// set materials
void SetMaterialForSelectedValidActors(FString MaterialPath, int Index);
void SetMaterialForValidActor(AActor* Actor, const FString& MaterialPath, int Index);
bool SetMaterialForMeshComponent(UMeshComponent* MeshComponent, const FString& MaterialPath, int Index);
// get material attributes
TArray<FLinearColor> GetVecMatAttrsForValidActors(const TArray<AActor*>& ValidActors, const FString& AttributeName, int Index);
TArray<FLinearColor> GetVecMatAttrsForValidActor(AActor* ValidActor, const FString& AttributeName, int Index);
TArray<float> GetScalarMatAttrsForValidActors(const TArray<AActor*>& ValidActors, FString AttributeName);
TArray<float> GetScalarMatAttrsForValidActor(AActor* ValidActor, FString AttributeName);
// get material attribute names
TArray<FString> getMaterialAttributeNamesFromSelectedStaticMeshActors(FString AttributeType);
TArray<FString> GetMaterialAttributeNamesFromValidActors(const TArray<FString>& ActorNames, FString AttributeType, UWorld* World);
TArray<FString> GetMaterialAttributeNamesFromValidActor(AActor* Actor, FString AttributeType);
// get material names
TArray<FString> GetMaterialPathsFromSelectedValidActors();
FString GetMaterialPathJSONDictFromActorsByName(const TArray<FString>& ActorNames, UWorld* World);
TArray<FString> getMaterialPathsFromStaticMeshActorByName(FString ActorName, UWorld* World);
TArray<FString> GetMaterialPathsFromValidActor(AActor* Actor);

// get material function dependencies
TArray<UMaterialFunctionInterface*> GetMaterialFunctionDependencies(const UMaterial& Material);

// generic
void CreateMaterialInstancesFromJsonString(const FString& JsonString);
void SetMeshAssetMaterialFromJsonString(const FString& JsonString);
void RemoveMaterialOverride(const FString& ActorName, UWorld* World);
TArray<UMaterialInterface*> GetAllMaterialInterfacesFromValidActor(AActor* Actor);


// --------------------------------------------------------------------
// --- Physics Simulation
// --------------------------------------------------------------------

FString GetActorsPhysicsSimulationStateByName(const TArray<FString>& ActorNames);
void SetActorsToBePhysicallySimulatedByName(const TArray<FString>& ActorNames, bool State);
void SetActorToBePhysicallySimulatedByName(const FString& ActorName, bool State);
void SetActorToBePhysicallySimulated(AActor* Actor, bool State);
TArray<AActor*> GetSimulatingActorsByName(const TArray<FString>& ActorNames);
FString GetSimulatingActorsTransformJSONDictByNames(const TArray<FString>& ActorNames);
bool GetActorsPhysicalSimulationState(AActor* Actor);

template<class ActorType>
ActorType* GetSimulatingActorByName(const FString& ActorName);

void EnablePlayInEditor();
void DisablePlayInEditor();

// --------------------------------------------------------------------
// --- Scene Management Functions
// --------------------------------------------------------------------

FString addStaticMeshActors(UWorld* World, FString JsonString);

TArray<AStaticMeshActor*> addStaticMeshActorOnSelection(FString MeshPath, UWorld* World);  // TODO: THIS MOST LIKELY NEEDS FIXING. - why though? :) 
TArray<AStaticMeshActor*> addStaticMeshActorsOnSelection(TArray<FString> MeshPaths, UWorld* World);
template<class ActorType>
ActorType* AddActorToWorld(UWorld* World, const FString& Name, const FVector& Location, const FVector& RotationVec = FVector::ZeroVector, const FVector& Scale = FVector::OneVector);
AStaticMeshActor* addStaticMeshActor(UWorld* World, FString MeshPath, FString Name = "", FVector Location = FVector(0, 0, 0), FVector RotationVec = FVector(0, 0, 0), FVector Scale = FVector(1, 1, 1)); // Could probably be replaced by templated version above, AddActorToWorld(..).
AStaticMeshActor* addEmptyStaticMeshActor(UWorld* World, FString Name = "", FVector Location = FVector(0, 0, 0), FVector RotationVec = FVector(0, 0, 0), FVector Scale = FVector(1, 1, 1)); // Could probably be replaced by templated version above, AddActorToWorld(..).

AActor* addBlueprintActor(UWorld* World, FString MeshPath, FString Name, FVector Location, FVector RotationVec, FVector Scale);

void SetStaticMeshAssetForValidActor(AActor* ValidActor, FString MeshPath);

template<class ActorType>
ActorType* GetActorByName(const FString& ActorName, UWorld* World);

TArray<AActor*> GetRenderedActors();

bool ActorHasStaticMeshComponent(AActor* Actor);

void SetActorsHiddenByName(const TArray<FString>& ObjectNames, UWorld* World);
void SetActorsVisibleByName(const TArray<FString>& ObjectNames, UWorld* World);

void SetStaticMeshActorsHiddenByName(const TArray<FString>& ObjectNames, UWorld* World);
void SetStaticMeshActorsVisibleByName(const TArray<FString>& ObjectNames, UWorld* World);

void FocusOnActorsByName(const TArray<FString>& ObjectNames, UWorld* World);
void SelectSceneActorsByName(const TArray<FString>& ObjectNames);

TArray<AActor*> GetAllObjectsIntersectingGivenObjects(TArray<AActor*> Objects);

TArray<FString> GetParentsForObjectsByName(const TArray<FString>& ObjectNames, UWorld* World);
FString GetParentForObjectByName(FString ObjectName, UWorld* World);
TArray<AActor*> GetAllDescendentsForObjectsRecursive(TArray<AActor*> Objects, UWorld* World);
TArray<AActor*> GetAllDescendentsForObjectRecursive(AActor* Object, UWorld* World);

void ParentObjectsByName(FString ParentName, FString ChildName, UWorld* World);
void ParentAllObjectsByName(FString ParentName, const TArray<FString>& ChildNames, UWorld* World);

void UnparentObjectsByName(const TArray<FString>& Names, UWorld* World);
void UnparentObjectByName(FString Name, UWorld* World);

void FixObjectNames(UWorld* World);
void RenameActor(FString SourceName, FString TargetName, UWorld* World);
void ToggleKillNamesOnSelection();

void OpenLevel(FString LevelPath);
void LoadLevel(FString LevelPath, UWorld* World);
void UnloadLevel(FString LevelPath, UWorld* World);
void SetLevelVisibility(FString LevelPath, bool IsVisible, UWorld* World);
void SetLevelCurrent(FString LevelPath, UWorld* World);
FString GetCurrentLevelPath(UWorld* World);

// --------------------------------------------------------------------
// --- Asset Library Functions
// --------------------------------------------------------------------

FString getSceneName(UWorld* World);
FString GetAssetBrowserSelection();
void SetAssetBrowserSelection(const TArray<UObject*>& SelectedObjects);
FString getJSONAssetDataFromAssetBrowserSelection();
template<class AssetType>
FString GetJSONImportSourceDataFromAsset(const FString& AssetPath);
void EditAsset(FString AssetReferencePath);
void FindAsset(FString AssetReferencePath);
void FindAssets(TArray<FString> AssetReferencePaths);
void LoadAssets(TArray<FString> AssetReferencePaths);
void ImportAssetToCurrentPath();
void ImportAsset(FString AssetImportPath);
void DeleteSelectedAssets();

TArray<UObject*> ImportFBXAssetsFromJsonString(const FString& JsonString);
TArray<UObject*> ImportTextureAssetsFromJsonString(const FString& JsonString);
UObject* LoadFBXFile(const FString& AbsoluteSourcePath, const FString& AssetTarget);
UObject* LoadTextureFile(const FString& AbsoluteSourcePath, const FString& AssetTarget);
template<class ObjectType>
UObject* AddObjectToContentBrowser(ObjectType* AssetObject, const FString& TargetPath);


FString getAllExistingAssetByType(FString OutputPath, FString AssetType);

// --------------------------------------------------------------------
// --- Ray Tracing
// --------------------------------------------------------------------

FString RayTraceFromPointsJSON(const TArray<FVector>& Points, FVector RayDirection, UWorld* World, TArray<AActor*> IgnoreActors);
FString RayTraceFromActorsByNameJSON(const TArray<FString>& ActorNames, FVector RayDirection, UWorld* World);
FVector RayTraceFromActorByName(FString ActorName, FVector RayDirection, UWorld* World);
FVector RayTraceFromActor(AActor* Actor, FVector RayDirection, UWorld* World);
FHitResult RayTrace(FVector StartPoint, FVector EndPoint, UWorld* World, TArray<AActor*> IgnoreActors = {});
FVector RayTraceFromCamera(UWorld* World);
// MULTI RAY TRACE
FString RayTraceMultiFromActorsByNameJSON(const TArray<FString>& ActorNames, FVector RayDirection, UWorld* World);
FVector RayTraceMultiFromActorByName(FString ActorName, FVector RayDirection, UWorld* World);
FVector RayTraceMultiFromActor(AActor* Actor, FVector RayDirection, UWorld* World, TArray<AActor*> IgnoreActors = {});
TArray<FHitResult> RayTraceMulti(FVector StartPoint, FVector EndPoint, UWorld* World, TArray<AActor*> IgnoreActors = {});

// --------------------------------------------------------------------
// --- Mesh Functions
// --------------------------------------------------------------------

TArray<FVector> GetMeshVertexPositions(AStaticMeshActor* StaticMeshActor);
TArray<FVector> GetMeshTrianglePositionsFromActor(AActor* MeshActor);
TArray<FVector> GetMeshTrianglePositionsFromStaticMesh(UStaticMeshComponent* StaticMeshComponent);
TArray<FVector> GetMeshTrianglePositionsFromGeneratedMesh(APrometheanMeshActor* PrometheanMeshActor);
TArray<FVector> GetMeshTrianglePositionsFromLibraryStaticMesh(UStaticMesh* StaticMesh);

// --------------------------------------------------------------------
// --- Misc Functions
// --------------------------------------------------------------------

FQuat RotationFromNormal(FVector NormalVector);
TArray<FVector> getSelectionPositions(UWorld* World);
TArray<TSharedPtr<class FJsonValue>> FVectorToJsonArray(FVector Vector);
TArray<TSharedPtr<FJsonValue>> FVectorArrayToJsonArray(const TArray<FVector>& VectorArray);
TArray<TSharedPtr<FJsonValue>> FStringArrayToJsonArray(TArray<FString> StringArray);
TSharedPtr<FJsonObject> FStringArrayToJsonIndexDict(const TArray<FString>& StringArray);
FString FVectorArrayToJsonString(const TArray<FVector>& VectorArray, FString Label);
void CaptureScreenshot(FString OutputPath);

/**
 *	Save a string to a text file
 *	@param OutputPath	- path where to save the file
 *	@param TextToSave	- str to write into the file. most likely will be a json string
 *	@return				
 */
void SaveToFile(FString OutputPath, FString TextToSave);

// string to stuff
FTransform StringArrayToTransform(const TArray<FString>& StringArray);
FVector StringToVector(FString String);
FVector StringArrayToVector(const TArray<FString>& StringArray);
FString StringArrayToString(const TArray<FString>& Strings);
FLinearColor StringToLinearColor(FString String);
// stuff to string
FString LinearColorArrayToString(const TArray<FLinearColor>& ColorArray);
FString FloatArrayToString(const TArray<float>& Floats);
FString FVectorToString(FVector Vector);
FString StaticMeshActorArrayToNamesString(const TArray<AStaticMeshActor*>& Actors);
FString StaticMeshActorArrayToAssetPathsString(const TArray<AStaticMeshActor*>& Actors);
FString ActorArrayToNamesString(const TArray<AActor*>& Actors);
FString ActorArrayToAssetPathsString(const TArray<AActor*>& Actors);
// stuff to string array
TArray<FString> ActorArrayToNamesArray(const TArray<AActor*>& Actors);
TArray<FString> ActorArrayToAssetPathsArray(const TArray<AActor*>& Actors);
TArray<FVector> ActorArrayToLocationsArray(TArray<AActor*> Actors);
TArray<FVector> ActorArrayToPivotArray(TArray<AActor*> Actors);

FRotator FRotatorFromXYZVec(FVector RotationVec);

bool ParseJsonString(const FString& String, TSharedPtr<FJsonObject>& Result);

USceneComponent* GetBiggestVolumeBoundComponent(const TArray<UStaticMeshComponent*>& Components);
bool SaveToAsset(UObject* ObjectToSave);

