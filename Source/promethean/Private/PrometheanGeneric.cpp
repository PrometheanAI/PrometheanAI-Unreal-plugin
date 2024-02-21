// Fill out your copyright notice in the Description page of Project Settings.

#include "PrometheanGeneric.h"  // matching header has to go first

#include "promethean.h"
#include "PrometheanLearning.h"  // get  valid static mesh component is there
#include "PrometheanLibraryData.h"
#include "PrometheanMesh.h"

#include "AssetToolsModule.h"
#include "Editor.h" // GEditor
#include "Editor/ContentBrowser/Public/ContentBrowserModule.h" // Content Browser functionalities
#include "Editor/ContentBrowser/Public/IContentBrowserSingleton.h" // Needed for Content Browser module
#include "Editor/UnrealEd/Classes/Factories/FbxFactory.h" // FBX loader
#include "Editor/UnrealEd/Classes/Factories/FbxImportUI.h" // FBX loader parameters
#include "Editor/UnrealEd/Classes/Factories/FbxStaticMeshImportData.h" // FBX loader parameters
#include "Editor/UnrealEd/Classes/Factories/TextureFactory.h" // Texture loader
#include "EditorLevelUtils.h" // level load/unload
#include "EditorViewportClient.h"  // GEditor viewport client to get camera data
#include "EngineUtils.h"
#include "FileHelpers.h"  // open level from editor
#include "LevelEditor.h" // get current viewport for SIE
#include "ObjectTools.h"  // delete asset from asset library is in here
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 1	
#include "AssetRegistry/AssetRegistryModule.h" // Module to register created asset
#include "Selection.h"
#else
#include "AssetRegistryModule.h" // Module to register created asset
#include "Engine/Selection.h"
#endif
#include "StaticParameterSet.h"
#include "Engine/LevelStreamingDynamic.h"
#include "HAL/PlatformFileManager.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/Object.h"
#include "UObject/SavePackage.h"

// --------------------------------------------------------------------
// --- Editor Functions
// --------------------------------------------------------------------
UWorld* GetEditorWorld()
{
	// FDateTime StartTime = FDateTime::Now();
	auto WorldContext = GEditor->GetEditorWorldContext();
	return WorldContext.World();
	// FDateTime EndTime = FDateTime::Now();
	// FTimespan TimeElapsed = EndTime - StartTime;
	// UE_LOG(LogPromethean, Display, TEXT("Getting world took: %s"), *TimeElapsed.ToString());  // - either this doesn't tick or it happens sub millisecond
}


bool SaveToAsset(UObject* ObjectToSave)
{
	UPackage* Package = ObjectToSave->GetOutermost();
	const FString& PackageName = Package->GetName();
	const FString& PackageFileName = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
#if ENGINE_MAJOR_VERSION >= 5
	FSavePackageArgs SavePackageArgs;
	SavePackageArgs.TopLevelFlags = RF_Public | RF_Standalone;
	SavePackageArgs.Error = GError;
	SavePackageArgs.SaveFlags = SAVE_NoError;
	SavePackageArgs.bForceByteSwapping = false;
	SavePackageArgs.bWarnOfLongFilename = true;
	const bool bSuccess = UPackage::SavePackage(Package, ObjectToSave, *PackageFileName, SavePackageArgs);
#else
	const bool bSuccess = UPackage::SavePackage(Package, nullptr, RF_Public | RF_Standalone, *PackageFileName, GError, nullptr, false, true, SAVE_NoError);
#endif
	
	if (!bSuccess)
	{
		UE_LOG(LogTemp, Error, TEXT("Package '%s' wasn't saved!"), *PackageName)
			return false;
	}

	UE_LOG(LogTemp, Warning, TEXT("Package '%s' was successfully saved"), *PackageName)
		return true;
}


// --------------------------------------------------------------------
// --- Transform Functions
// --------------------------------------------------------------------

void TransformActorsByName(const TArray<FString>& ObjectNames, FTransform Transform, UWorld* World)
{
	for (auto& ObjectName : ObjectNames)
	{
		TransformActorByName(ObjectName, Transform, World);
	}
}

void TransformActorByName(FString ObjectName, FTransform Transform, UWorld* World)
{	
	auto StaticMeshActor = GetActorByName<AActor>(ObjectName.TrimStartAndEnd(), World);
	if (StaticMeshActor != nullptr)
	{
		StaticMeshActor->Modify(); // register undo
		StaticMeshActor->SetActorTransform(Transform, false);  // second argument is sweeping for physics purposes which we don't do or care about
	}
}


void TranslateActorsByName(const TArray<FString>& ObjectNames, FVector NewLocation, UWorld* World)
{
	for (auto& ObjectName : ObjectNames)
	{
		TranslateActorByName(ObjectName, NewLocation, World);
	}
}

void TranslateActorByName(FString ObjectName, FVector NewLocation, UWorld* World)
{	
	auto StaticMeshActor = GetActorByName<AActor>(ObjectName.TrimStartAndEnd(), World);
	if (StaticMeshActor != nullptr)
	{
		StaticMeshActor->Modify(); // register undo
		StaticMeshActor->SetActorLocation(NewLocation, false);  // second argument is sweeping for physics purposes which we don't do or care about
	}
}


void TranslateAndRayTraceActorsByName(const TArray<FString>& ObjectNames, FVector NewLocation, float RaytraceDistance, float MaxNormalDeviation, const TArray<FString>& IgnoreObjectNames, UWorld* World)
{
	TArray<AActor*> IgnoreActors;
	for (auto& ObjectName : IgnoreObjectNames)
	{
		auto StaticMeshActor = GetActorByName<AActor>(ObjectName.TrimStartAndEnd(), World);
		if (StaticMeshActor != nullptr)
		{
			IgnoreActors.Add(StaticMeshActor);
		}
	}	
	for (auto& ObjectName : ObjectNames)
	{
		TranslateAndRaytraceActorByName(ObjectName, NewLocation, RaytraceDistance, MaxNormalDeviation, IgnoreActors, World);
	}
}


void TranslateAndRaytraceActorByName(FString ObjectName, FVector NewLocation, float RaytraceDistance, float MaxNormalDeviation, TArray<AActor*> IgnoreActors, UWorld* World)
{
	auto Actor = GetActorByName<AActor>(ObjectName.TrimStartAndEnd(), World);
	if (Actor != nullptr)
	{
		Actor->Modify(); // register undo
		if (RaytraceDistance)
		{
			FVector RayDirection = FVector(0.0, 0.0, RaytraceDistance);  // *0.5);  // half it to offset up and down				
			FVector StartPoint = NewLocation; // + RayDirection;
			FVector EndPoint = NewLocation - RayDirection;
			FHitResult TraceResult = RayTrace(StartPoint, EndPoint, World, IgnoreActors);
			// set location
			FVector HitLocation = TraceResult.Location; // will return all zeroes if no hit found			
			if (!HitLocation.IsZero())
			{				
				NewLocation = HitLocation;
				NewLocation.Z += FMath::FRandRange(0.0, 0.25); // fix Z-fighting for flat surfaces by offseting them ever so slightly				
			}	
			// set normal
			float UpDotProduct = FVector::DotProduct(FVector(0, 0, 1), TraceResult.Normal);
			UE_LOG(LogPromethean, Display, TEXT("DOT PRODUCT: %f, %f"), UpDotProduct, MaxNormalDeviation);
			if (UpDotProduct > MaxNormalDeviation)
			{
				FQuat Quat = RotationFromNormal(TraceResult.Normal);				
				FRotator Rotator = Actor->GetRootComponent()->GetComponentRotation();
				FVector Rotation = Rotator.Euler();
				Rotation[0] = 0.0;  // zero out everything but rotation around up axis
				Rotation[1] = 0.0;  // zero out everything but rotation around up axis						
				FQuat RootQuat = FQuat::MakeFromEuler(Rotation);
				FQuat NewQuat = Quat * RootQuat;  // this has a bit of a drift and if repeated over time the rotation will settle into Quat
				Actor->SetActorRotation(NewQuat.Rotator());

			}			
		}
		Actor->SetActorLocation(NewLocation, false);  // second argument is sweeping for physics purposes which we don't do or care about					
	}
}


FQuat RotationFromNormal(FVector NormalVector)
{	
	FVector UpVector = FVector(0, 0, 1);
	FQuat Quat;
	FVector RotationAxis = FVector::CrossProduct(UpVector, NormalVector);
	RotationAxis.Normalize();
	float DotProduct = FVector::DotProduct(UpVector, NormalVector);	
	float RotationAngle = acosf(DotProduct);
	Quat = FQuat(RotationAxis, RotationAngle);		
	return Quat;
}


void RelativeTranslateActorsByName(const TArray<FString>& ObjectNames, FVector LocationDelta, UWorld* World)
{
	for (auto& ObjectName : ObjectNames)
	{
		RelativeTranslateActorByName(ObjectName, LocationDelta, World);
	}
}

void RelativeTranslateActorByName(FString ObjectName, FVector LocationDelta, UWorld* World)
{	
	auto StaticMeshActor = GetActorByName<AActor>(ObjectName.TrimStartAndEnd(), World);
	if (StaticMeshActor != nullptr)
	{	
		StaticMeshActor->Modify(); // register undo
		// offset is in world space
		StaticMeshActor->AddActorWorldOffset(LocationDelta, false);  // second argument is sweeping for physics purposes which we don't do or care about
	}
}


void RotateActorsByName(const TArray<FString>& ObjectNames, FVector NewRotationVec, UWorld* World)
{
	for (auto& ObjectName : ObjectNames)
	{
		RotateActorByName(ObjectName, NewRotationVec, World);
	}
}

void RotateActorByName(FString ObjectName, FVector NewRotationVec, UWorld* World)  // takes vector for consistency with other transform functions
{		
	auto StaticMeshActor = GetActorByName<AActor>(ObjectName.TrimStartAndEnd(), World);
	if (StaticMeshActor != nullptr)
	{
		FRotator NewRotation = FRotatorFromXYZVec(NewRotationVec);
		StaticMeshActor->Modify(); // register undo
		StaticMeshActor->SetActorRotation(NewRotation);  // second argument is teleport type by default set to ETeleportType::None
	}
}

void RelativeRotateActorsByName(const TArray<FString>& ObjectNames, FVector NewRotationVec, UWorld* World)
{
	for (auto& ObjectName : ObjectNames)
	{
		RelativeRotateActorByName(ObjectName, NewRotationVec, World);
	}
}

void RelativeRotateActorByName(FString ObjectName, FVector NewRotationVec, UWorld* World)
{
	auto StaticMeshActor = GetActorByName<AActor>(ObjectName.TrimStartAndEnd(), World);
	if (StaticMeshActor != nullptr)
	{
		FRotator NewRotation = FRotatorFromXYZVec(NewRotationVec);
		StaticMeshActor->Modify(); // register undo
		StaticMeshActor->AddActorLocalRotation(NewRotation);
	}
}


void ScaleActorsByName(const TArray<FString>& ObjectNames, FVector NewScale, UWorld* World)
{
	for (auto& ObjectName : ObjectNames)
	{
		ScaleActorByName(ObjectName, NewScale, World);
	}
}

void ScaleActorByName(FString ObjectName, FVector NewScale, UWorld* World)
{
	auto StaticMeshActor = GetActorByName<AActor>(ObjectName.TrimStartAndEnd(), World);
	if (StaticMeshActor != nullptr)
	{
		StaticMeshActor->SetActorScale3D(NewScale);
	}
}


void RelativeScaleActorsByName(const TArray<FString>& ObjectNames, FVector NewScale, UWorld* World)
{
	for (auto& ObjectName : ObjectNames)
	{
		RelativeScaleActorByName(ObjectName, NewScale, World);
	}
}

void RelativeScaleActorByName(FString ObjectName, FVector NewScale, UWorld* World)
{	
	auto StaticMeshActor = GetActorByName<AActor>(ObjectName.TrimStartAndEnd(), World);
	if (StaticMeshActor != nullptr)
	{
		FVector OldScale = StaticMeshActor->GetActorScale3D();
		StaticMeshActor->Modify(); // register undo
		StaticMeshActor->SetActorScale3D(OldScale * NewScale);
	}
}


void RemoveActorsByName(const TArray<FString>& ObjectNames, UWorld* World)
{
	for (auto& ObjectName : ObjectNames)
	{
		RemoveActorByName(ObjectName, World);
	}
}

void RemoveActorByName(FString ObjectName, UWorld* World)
{	
	auto RemoveActor = GetActorByName<AActor>(ObjectName.TrimStartAndEnd(), World);
	if (RemoveActor != nullptr)
	{
		RemoveActor->Destroy();
	}
}


/*
Acceleration function that allows us to skip an extra readback from DCC for correspodance intensive operations
*/
void RemoveDescendentsFromActorsByName(const TArray<FString>& ObjectNames, UWorld* World)
{
	for (auto& ObjectName : ObjectNames)
	{
		RemoveDescendentsFromActorByName(ObjectName, World);
	}
}

/*
Will remove Actor and all of it's descendents 
*/
void RemoveChildrenRecursive(AActor* Actor)
{
	TArray<AActor*> DescendentPointers;
	Actor->GetAttachedActors(DescendentPointers, true);  // second argument is bResetArray
	for (auto & RemoveActor : DescendentPointers)
	{
		RemoveChildrenRecursive(RemoveActor);  // destroy children
	}
	Actor->Destroy(); // desctroy itself
}


/*
Acceleration function that allows us to skip an extra readback from DCC for correspodance intensive operations
*/
void RemoveDescendentsFromActorByName(FString ObjectName, UWorld* World)
{
	auto TopActor = GetActorByName<AActor>(ObjectName.TrimStartAndEnd(), World);
	if (TopActor != nullptr)
	{	
		TArray<AActor*> DescendentPointers;
		TopActor->GetAttachedActors(DescendentPointers, true);  // second argument is bResetArray		
		for (auto& RemoveActor : DescendentPointers)
		{
			RemoveChildrenRecursive(RemoveActor);
		}
	}
}


void RemoveSelectedActors()
{
	USelection* SelectedActors = GEditor->GetSelectedActors();
	for (FSelectionIterator Iter(*SelectedActors); Iter; ++Iter)
	{
		AActor* Actor = Cast<AActor>(*Iter);
		if (Actor)
			Actor->Destroy();
	}
}

FString GetValidActorsBottomCenterLocationJSONDict(const TArray<AActor*>& ValidActors)
{
	TSharedPtr<FJsonObject> RootObject = MakeShared<FJsonObject>();
	for (auto& Valid : ValidActors)
	{
		TArray<TSharedPtr<FJsonValue>> MeshTransformArray;
		// --- pivot
		FVector MeshPivotVec = Valid->GetComponentsBoundingBox().GetCenter();
		MeshPivotVec.Z = MeshPivotVec.Z - Valid->GetComponentsBoundingBox().GetSize().Z / 2.0;

		TArray<TSharedPtr<FJsonValue>> MeshPivotArray;
		TSharedRef<FJsonValueNumber> MeshPivotX = MakeShareable(new FJsonValueNumber(MeshPivotVec.X));
		TSharedRef<FJsonValueNumber> MeshPivotY = MakeShareable(new FJsonValueNumber(MeshPivotVec.Y));
		TSharedRef<FJsonValueNumber> MeshPivotZ = MakeShareable(new FJsonValueNumber(MeshPivotVec.Z));
		MeshTransformArray.Add(MeshPivotX);
		MeshTransformArray.Add(MeshPivotY);
		MeshTransformArray.Add(MeshPivotZ);

		RootObject->SetArrayField(Valid->GetName().ToLower(), MeshTransformArray);
	}

	FString OutputString;
	TSharedRef< TJsonWriter<> > Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);
	return OutputString;
}

FString GetValidActorsLocationJSONDict(const TArray<AActor*>& ValidActors)
{
	TSharedPtr<FJsonObject> RootObject = MakeShared<FJsonObject>();
	for (auto& Valid : ValidActors)
	{
		TArray<TSharedPtr<FJsonValue>> MeshTransformArray;
		// location
		TSharedRef<FJsonValueNumber> MeshTranslationX = MakeShareable(new FJsonValueNumber(Valid->GetActorLocation().X));
		TSharedRef<FJsonValueNumber> MeshTranslationY = MakeShareable(new FJsonValueNumber(Valid->GetActorLocation().Y));
		TSharedRef<FJsonValueNumber> MeshTranslationZ = MakeShareable(new FJsonValueNumber(Valid->GetActorLocation().Z));
		MeshTransformArray.Add(MeshTranslationX);
		MeshTransformArray.Add(MeshTranslationY);
		MeshTransformArray.Add(MeshTranslationZ);		
		RootObject->SetArrayField(Valid->GetName().ToLower(), MeshTransformArray);
	}

	FString OutputString;
	TSharedRef< TJsonWriter<> > Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);

	return OutputString;
}

FString GetValidActorsTransformJSONDict(const TArray<AActor*>& ValidActors)
{
	TSharedPtr<FJsonObject> RootObject = MakeShared<FJsonObject>();
	for (auto& Valid : ValidActors)
	{
		// location		
		TArray<TSharedPtr<FJsonValue>> MeshTransformArray = GetVectorJSONData(Valid->GetActorLocation());
		// rotation
		FRotator ActorRotation = Valid->GetActorRotation();  // reused later to reset rotation back after getting local space bounds
		TArray<TSharedPtr<FJsonValue>> MeshRotationArray = GetRotationJSONData(Valid->GetActorRotation());
		MeshTransformArray.Append(MeshRotationArray);
		// scale
		TArray<TSharedPtr<FJsonValue>> MeshScaleArray = GetVectorJSONData(Valid->GetActorScale3D());
		MeshTransformArray.Append(MeshScaleArray);
		RootObject->SetArrayField(Valid->GetName().ToLower(), MeshTransformArray);
	}

	FString OutputString;
	TSharedRef< TJsonWriter<> > Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);

	return OutputString;
}

FString GetValidActorsExpandedTransformJSONDict(const TArray<AActor*>& ValidActors, UWorld* World)
{
	TSharedPtr<FJsonObject> RootObject = MakeShared<FJsonObject>();
	for (auto& Valid : ValidActors)
	{
		// location		
		TArray<TSharedPtr<FJsonValue>> MeshTransformArray = GetVectorJSONData(Valid->GetActorLocation());
		// rotation
		FRotator ActorRotation = Valid->GetActorRotation();  // reused later to reset rotation back after getting local space bounds
		TArray<TSharedPtr<FJsonValue>> MeshRotationArray = GetRotationJSONData(Valid->GetActorRotation());		
		MeshTransformArray.Append(MeshRotationArray);		
		// scale
		TArray<TSharedPtr<FJsonValue>> MeshScaleArray = GetVectorJSONData(Valid->GetActorScale3D());
		MeshTransformArray.Append(MeshScaleArray);
		// size
		USceneComponent* SceneComponent = GetValidSceneComponent(Valid);
		if (SceneComponent != nullptr)
		{
			Valid->SetActorRotation(FRotator(0.0, 0.0, 0.0));  // reset rotation to get accurate LS size of arbitrary actor
			FVector MeshSize = SceneComponent->Bounds.GetBox().GetSize();  // - this would be world space, if wasn't for rotation
			TArray<TSharedPtr<FJsonValue>> MeshSizeArray = GetVectorJSONData(MeshSize);
			Valid->SetActorRotation(ActorRotation);  // rotate back in place and pretend like nothing happened
			MeshTransformArray.Append(MeshSizeArray);			
		}
		else
		{	// fallback to defaults in case semantic group and no size for example
			MeshTransformArray.Add(MakeShareable(new FJsonValueNumber(0)));
			MeshTransformArray.Add(MakeShareable(new FJsonValueNumber(0)));
			MeshTransformArray.Add(MakeShareable(new FJsonValueNumber(0)));
		}

		// pivot offset: vector from bottom center of the bounding box to the pivot (in local space without rotation and scale)
        FBox LocalBBox = Valid->CalculateComponentsBoundingBoxInLocalSpace();
		FVector PivotOffset = LocalBBox.GetCenter();
		PivotOffset.Z = PivotOffset.Z - LocalBBox.GetExtent().Z;
		TArray<TSharedPtr<FJsonValue>> MeshPivotOffsetArray = GetVectorJSONData(-PivotOffset);
		MeshTransformArray.Append(MeshPivotOffsetArray);

		// parent dcc name
		FString ParentName = GetParentForObjectByName(Valid->GetName(), World);
		MeshTransformArray.Add(MakeShareable(new FJsonValueString(ParentName.ToLower())));  // lowercase name
		// set data
		//RootObject->SetArrayField(TEXT("transform"), MeshTransformArray);
		RootObject->SetArrayField(Valid->GetName().ToLower(), MeshTransformArray);
	}

	FString OutputString;
	TSharedRef< TJsonWriter<> > Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);

	return OutputString;
}

FString getStaticMeshActorsTransformJSONDictByName(const TArray<FString>& ObjectNames, UWorld* World)
{
	TArray<AStaticMeshActor*>  StaticMeshActors;
	for (auto& ObjectName : ObjectNames)
	{
		AStaticMeshActor* StaticMeshActor;
		StaticMeshActor = GetActorByName<AStaticMeshActor>(ObjectName.TrimStartAndEnd(), World);
		if (StaticMeshActor)  // check if nullptr
			StaticMeshActors.Add(StaticMeshActor);
	}
	return getStaticMeshActorsTransformJSONDict(StaticMeshActors);
}

FString getStaticMeshActorsTransformJSONDict(const TArray<AStaticMeshActor*>& StaticMeshActors)
{
	TSharedPtr<FJsonObject> RootObject = MakeShared<FJsonObject>();
	for (auto& StaticMeshActor : StaticMeshActors)
	{
		// location
		TArray<TSharedPtr<FJsonValue>> MeshTransformArray = GetVectorJSONData(StaticMeshActor->GetActorLocation());
		// rotation		
		TArray<TSharedPtr<FJsonValue>> MeshRotationArray = GetRotationJSONData(StaticMeshActor->GetActorRotation());
		MeshTransformArray.Append(MeshRotationArray);
		// scale
		TArray<TSharedPtr<FJsonValue>> MeshScaleArray = GetVectorJSONData(StaticMeshActor->GetActorScale3D());
		MeshTransformArray.Append(MeshScaleArray);
		//RootObject->SetArrayField(TEXT("transform"), MeshTransformArray);
		RootObject->SetArrayField(StaticMeshActor->GetName().ToLower(), MeshTransformArray);
	}

	FString OutputString;
	TSharedRef< TJsonWriter<> > Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);

	return OutputString;
}


FString getCameraInfoJSONDict(bool ObjectsOnScreen)
{
	TSharedPtr<FJsonObject> RootObject = MakeShared<FJsonObject>();
	// get data
	FEditorViewportClient* client = (FEditorViewportClient*)GEditor->GetActiveViewport()->GetClient();
	FVector editorCameraPosition = client->GetViewLocation();
	FVector editorCameraDirection = client->GetViewRotation().Vector();	
	float FOV = client->ViewFOV;
	// do the json struct hubris
	// location
	TArray<TSharedPtr<FJsonValue>> CameraLocationArray = GetVectorJSONData(editorCameraPosition);	
	RootObject->SetArrayField("camera_location", CameraLocationArray);
	// direction
	TArray<TSharedPtr<FJsonValue>> CameraDirectionArray = GetVectorJSONData(editorCameraDirection);
	RootObject->SetArrayField("camera_direction", CameraDirectionArray);
	// FOV
	RootObject->SetNumberField("camera_fov", FOV);
	// Objects on screen
	FString ObjectsNameString = "";
	if (ObjectsOnScreen)
	{
		TArray<AActor*> RenderedActors = GetRenderedValidActors();
		ObjectsNameString = ActorArrayToNamesString(RenderedActors);
	}
	RootObject->SetStringField("objects_on_screen", ObjectsNameString);
	// get final string
	FString OutputString;
	TSharedRef< TJsonWriter<> > Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);
	return OutputString;
}

// --------------------------------------------------------------------
// --- Utility Functions
// --------------------------------------------------------------------

/*
	Checks an actor, whether it is supported by the plugin.
	When not supported, returns nullptr.
*/
AActor* ValidateActor(AActor* Actor)
{
	// if static mesh actor
	if (Cast<AStaticMeshActor>(Actor) != nullptr)
	{
		return Actor;
	}

	// if generated mesh
	if (Cast<APrometheanMeshActor>(Actor) != nullptr)
	{
		return Actor;
	}

	// if custom blueprint
	if (ActorHasStaticMeshComponent(Actor))
	{
		return Actor;
	}

	return nullptr;
}

/*
	Finds an actor in the World, represented by supplied ActorName.
	When not found or not supported by the plugin, returns nullptr.
*/
AActor* GetValidActorByName(const FString& ActorName, UWorld* World)
{
	auto FoundActor = GetActorByName<AActor>(ActorName, World);
	if (FoundActor == nullptr)
	{
		return nullptr;
	}
	return ValidateActor(FoundActor);
}

// Extract components

/*
	Returns main scene component (mesh) from any actor supported by the plugin.
*/
USceneComponent* GetValidSceneComponent(AActor* Actor)
{
	if (Actor == nullptr)
	{
		return nullptr;
	}

	// if static mesh actor
	auto StaticMesh = Cast<AStaticMeshActor>(Actor);
	if (StaticMesh != nullptr)
	{
		return StaticMesh->GetRootComponent();
	}

	// if generated mesh
	auto GeneratedMesh = Cast<APrometheanMeshActor>(Actor);
	if (GeneratedMesh != nullptr)
	{
		return Cast<USceneComponent>(GeneratedMesh->GeneratedMesh);
	}

	// if custom blueprints
	TArray<UStaticMeshComponent*> ActorStaticMeshComponents;
	Actor->GetComponents<UStaticMeshComponent>(ActorStaticMeshComponents);
	return GetBiggestVolumeBoundComponent(ActorStaticMeshComponents);
	// when adding more valid types, check if ActorStaticMeshComponents.Num() > 0
	// and whether BiggestVolumeBoundComponent is not nullptr,
	// then continue extracting other valid components.
}

UMeshComponent* GetValidMeshComponent(AActor* Actor)
{
	return Cast<UMeshComponent>(GetValidSceneComponent(Actor));
}

UStaticMeshComponent* GetValidStaticMeshComponent(AActor* Actor)
{
	return Cast<UStaticMeshComponent>(GetValidSceneComponent(Actor));
}

// Named
TArray<AActor*> GetNamedValidActors(const TArray<FString>& ActorNames, UWorld* World)
{
	TArray<AActor*> SupportedActors;
	for (auto& ActorName : ActorNames)
	{
		auto FoundActor = GetValidActorByName(ActorName, World);
		if (FoundActor != nullptr)
		{
			SupportedActors.Add(FoundActor);
		}
	}
	return SupportedActors;
}

// Actors in the world
TArray<AActor*> GetWorldValidActors(UWorld* World)
{
	TArray<AActor*> ValidActors;
	for (TActorIterator<AActor> ActorItr(World); ActorItr; ++ActorItr)
	{
		auto Validated = ValidateActor(*ActorItr);
		if (Validated != nullptr)
		{
			ValidActors.Add(*ActorItr);
		}
	}
	return ValidActors;
}

// Selected

TArray<AActor*> GetSelectedValidActors()
{
	USelection* SelectedActors = GEditor->GetSelectedActors();
	TArray<AActor*> ValidActors;
	for (FSelectionIterator Iter(*SelectedActors); Iter; ++Iter)
	{
		auto Actor = ValidateActor(Cast<AActor>(*Iter));
		if (Actor != nullptr)
		{
			ValidActors.Add(Actor);
		}
	}
	return ValidActors;
}

// Rendered

TArray<AActor*> GetRenderedValidActors()
{
	TArray<AActor*> ValidActors;
	for (auto& RenderedActor : GetRenderedActors())
	{
		auto Valid = ValidateActor(RenderedActor);
		if (Valid != nullptr)
		{
			ValidActors.Add(Valid);
		}
	}
	return ValidActors;
}

// Using asset path

TArray<AActor*> GetValidActorsByMeshAssetPath(const FString& MeshAssetPath, UWorld* World)
{
	TArray<AActor*> OutArray;
	// search in all actors in the world
	for (TActorIterator<AActor> ActorItr(World); ActorItr; ++ActorItr)
	{
		// We care only about static mesh components as they are connected to asset paths.
		UStaticMeshComponent* StaticMeshComponent = Cast<UStaticMeshComponent>(GetValidMeshComponent(*ActorItr));

		if (StaticMeshComponent == nullptr)
		{
			// skip all non-valid actors and all components which are not static mesh components (for example Generated Meshes don't have assets assigned to them yet).
			continue;
		}

		UStaticMesh* StaticMesh = StaticMeshComponent->GetStaticMesh();
		if (StaticMesh->GetPathName().ToLower() == MeshAssetPath)
		{
			OutArray.Add(*ActorItr);
		}
	}
	return OutArray;
}


// Using material path

TArray<AActor*> GetValidActorsByMaterialPath(const FString& MaterialPath, UWorld* World)
{
	TArray<AActor*> OutArray;
	for (TActorIterator<AStaticMeshActor> ActorItr(World); ActorItr; ++ActorItr)
	{
		TArray<FString> ActorMaterials = GetMaterialPathsFromValidActor(*ActorItr);
		for (FString ActorMaterial : ActorMaterials)
		{
			if (ActorMaterial.ToLower() == MaterialPath)
			{
				OutArray.Add(*ActorItr);
				break;
			}
		}
	}
	return OutArray;
}

// --------------------------------------------------------------------
// --- Material Functions
// --------------------------------------------------------------------

void SetVecMatAttrsForValidActor(AActor* Actor, FString AttributeName, FLinearColor Value, int Index = -1)
{
	TArray<UMaterialInterface*> Materials = GetAllMaterialInterfacesFromValidActor(Actor);
	UE_LOG(LogPromethean, Display, TEXT("Index %i"), Index);
	if (Index >= 0)  // if no index specified carry on as usual
	{
		UE_LOG(LogPromethean, Display, TEXT("Material Num %i"), Materials.Num());
		if (Materials.Num() <= Index)
		{
			UE_LOG(LogPromethean, Display, TEXT("Material index %i out of range in %s"), Index, *Actor->GetPathName());
			return;
		}			
		else  // if index is specified check if there is enough indexes and then try to set. !Warning assuming ParameterInfo matches visible order in editor
		{
			Materials = { Materials[Index] };
		}
	}
	for (UMaterialInterface* Material : Materials)
	{
		if (UMaterialInstanceConstant* MaterialInstance = Cast<UMaterialInstanceConstant>(Material))  // input is material interface
		{
			TArray<FMaterialParameterInfo> OutParameterInfo;
			TArray<FGuid> OutParameterIds;
			Material->GetAllVectorParameterInfo(OutParameterInfo, OutParameterIds);
			bool AttrFound = false;			
			for (FMaterialParameterInfo SingleParameterInfo : OutParameterInfo)
			{
				if (SingleParameterInfo.Name.ToString() == AttributeName)
				{
					MaterialInstance->SetVectorParameterValueEditorOnly(SingleParameterInfo, Value);
					AttrFound = true;
					break;  // stop parsing attributes if found a match, move on to next material
				}
			}
			if (!AttrFound)
				UE_LOG(LogPromethean, Display, TEXT("Attribute %s not found in %s"), *AttributeName, *MaterialInstance->GetPathName());			
		}
		else		
			UE_LOG(LogPromethean, Display, TEXT("Material %s not an instance"), *Material->GetPathName());
	}
}

void SetScalarMatAttrsForValidActor(AActor* Actor, FString AttributeName, float Value)
{
	TArray<UMaterialInterface*> Materials = GetAllMaterialInterfacesFromValidActor(Actor);
	for (UMaterialInterface* Material : Materials)
	{
		if (UMaterialInstanceConstant* MaterialInstance = Cast<UMaterialInstanceConstant>(Material))  // input is material interface
		{
			TArray<FMaterialParameterInfo> OutParameterInfo;
			TArray<FGuid> OutParameterIds;
			Material->GetAllScalarParameterInfo(OutParameterInfo, OutParameterIds);
			bool AttrFound = false;
			for (FMaterialParameterInfo SingleParameterInfo : OutParameterInfo)
			{
				if (SingleParameterInfo.Name.ToString() == AttributeName)
				{
					MaterialInstance->SetScalarParameterValueEditorOnly(SingleParameterInfo, Value);
					AttrFound = true;
					break;  // stop parsing attributes if found a match, move on to next material
				}
			}
			if (!AttrFound)
				UE_LOG(LogPromethean, Display, TEXT("Attribute %s not found in %s"), *AttributeName, *MaterialInstance->GetPathName());
		}
		else
			UE_LOG(LogPromethean, Display, TEXT("Material %s not an instance"), *Material->GetPathName());
	}
}

// set materials

void SetMaterialForSelectedValidActors(FString MaterialPath, int Index)
{
	// Set material for static meshes.
	auto ValidActors = GetSelectedValidActors();
	for (auto Actor : ValidActors)
	{
		SetMaterialForValidActor(Actor, MaterialPath, Index);
	}
}

void SetMaterialForValidActor(AActor* Actor, const FString& MaterialPath, int Index = -1)
{	
	Actor->Modify();
	auto MeshComponent = GetValidMeshComponent(Actor);
	if (MeshComponent == nullptr)
	{
		return;
	}

	if (Index != -1)
	{
		SetMaterialForMeshComponent(MeshComponent, MaterialPath, Index);
	}
	else
	{
		for (int32 i = 0; i < MeshComponent->GetNumMaterials(); ++i)
		{
			SetMaterialForMeshComponent(MeshComponent, MaterialPath, i);
		}
	}
}

bool SetMaterialForMeshComponent(UMeshComponent* MeshComponent, const FString& MaterialPath, int Index)
{
	bool Success = false;
	UMaterialInstanceConstant* MaterialInstance = LoadObject<UMaterialInstanceConstant>(nullptr, *MaterialPath, nullptr, LOAD_None, nullptr);
	if (MaterialInstance)
	{
		MeshComponent->SetMaterial(Index, MaterialInstance);
		Success = true;
	}
	UMaterial* Material = LoadObject<UMaterial>(nullptr, *MaterialPath, nullptr, LOAD_None, nullptr);
	if (Material)
	{
		MeshComponent->SetMaterial(Index, Material);
		Success = true;
	}
	return Success;
}

// get material attributes
TArray<FLinearColor> GetVecMatAttrsForValidActors(const TArray<AActor*>& ValidActors, const FString& AttributeName, int Index = -1)
{
	TArray<FLinearColor> OutVectors;
	for (AActor* Actor : ValidActors)
	{
		OutVectors.Append(GetVecMatAttrsForValidActor(Actor, AttributeName, Index));
	}
	return OutVectors;
}

TArray<FLinearColor> GetVecMatAttrsForValidActor(AActor* ValidActor, const FString& AttributeName, int Index = -1)
{
	TArray<UMaterialInterface*> Materials = GetAllMaterialInterfacesFromValidActor(ValidActor);
	TArray<FLinearColor> OutVectors;
	FLinearColor TempVector;
	if (Index >= 0)  // if no index specified carry on as usual
	{
		UE_LOG(LogPromethean, Display, TEXT("Material Num %i"), Materials.Num());
		if (Materials.Num() <= Index)
		{
			UE_LOG(LogPromethean, Display, TEXT("Material index %i out of range in %s"), Index, *ValidActor->GetPathName());
			return OutVectors;
		}
		else  // if index is specified check if there is enough indexes and then try to set. !Warning assuming ParameterInfo matches visible order in editor
		{
			Materials = { Materials[Index] };
		}
	}
	for (UMaterialInterface* Material : Materials)
	{
		if (UMaterialInstanceConstant* MaterialInstance = Cast<UMaterialInstanceConstant>(Material))  // input is material interface
		{
			TArray<FMaterialParameterInfo> OutParameterInfo;
			TArray<FGuid> OutParameterIds;
			Material->GetAllVectorParameterInfo(OutParameterInfo, OutParameterIds);
			bool AttrFound = false;
			for (FMaterialParameterInfo SingleParameterInfo : OutParameterInfo)
			{
				if (SingleParameterInfo.Name.ToString() == AttributeName)
				{
					MaterialInstance->GetVectorParameterValue(SingleParameterInfo, TempVector);  // second arugment is out value
					OutVectors.Add(TempVector);
					AttrFound = true;
					break;  // stop parsing attributes if found a match, move on to next material
				}
			}
			if (!AttrFound)
				UE_LOG(LogPromethean, Display, TEXT("Attribute %s not found in %s"), *AttributeName, *MaterialInstance->GetPathName());
		}
		else
			UE_LOG(LogPromethean, Display, TEXT("Material %s not an instance"), *Material->GetPathName());
	}
	return OutVectors;
}

TArray<float> GetScalarMatAttrsForValidActors(const TArray<AActor*>& ValidActors, FString AttributeName)
{
	TArray<float> OutArray;
	for (AActor* Actor : ValidActors)
	{
		OutArray.Append(GetScalarMatAttrsForValidActor(Actor, AttributeName));
	}
	return OutArray;
}

TArray<float> GetScalarMatAttrsForValidActor(AActor* ValidActor, FString AttributeName)
{
	TArray<UMaterialInterface*> Materials = GetAllMaterialInterfacesFromValidActor(ValidActor);
	TArray<float> OutArray;
	float TempFloat;
	for (UMaterialInterface* Material : Materials)
	{
		if (UMaterialInstanceConstant* MaterialInstance = Cast<UMaterialInstanceConstant>(Material))  // input is material interface
		{
			TArray<FMaterialParameterInfo> OutParameterInfo;
			TArray<FGuid> OutParameterIds;
			Material->GetAllScalarParameterInfo(OutParameterInfo, OutParameterIds);
			bool AttrFound = false;
			for (FMaterialParameterInfo SingleParameterInfo : OutParameterInfo)
			{
				if (SingleParameterInfo.Name.ToString() == AttributeName)
				{
					MaterialInstance->GetScalarParameterValue(SingleParameterInfo, TempFloat);  // second arugment is out value
					OutArray.Add(TempFloat);
					AttrFound = true;
					break;  // stop parsing attributes if found a match, move on to next material
				}
			}
			if (!AttrFound)
				UE_LOG(LogPromethean, Display, TEXT("Attribute %s not found in %s"), *AttributeName, *MaterialInstance->GetPathName());
		}
		else
			UE_LOG(LogPromethean, Display, TEXT("Material %s not an instance"), *Material->GetPathName());
	}
	return OutArray;
}

// get material attribute names
TArray<FString> GetMaterialAttributeNamesFromValidActors(const TArray<FString>& ActorNames, FString AttributeType, UWorld* World)
{
	TArray<FString> MaterialAttributeNames;
	for (FString ActorName : ActorNames)
	{
		auto ValidActor = GetValidActorByName(ActorName, World);
		if (ValidActor)
		{
			MaterialAttributeNames.Append(GetMaterialAttributeNamesFromValidActor(ValidActor, AttributeType));
		}
	}
	return MaterialAttributeNames;
}

TArray<FString> getMaterialAttributeNamesFromSelectedStaticMeshActors(FString AttributeType)
{
	TArray<AActor*> ValidActors = GetSelectedValidActors();
	TArray<FString> MaterialAttributeNames;
	for (auto& Actor : ValidActors)
	{		
		if (Actor != nullptr)
		{
			MaterialAttributeNames.Append(GetMaterialAttributeNamesFromValidActor(Actor, AttributeType));
		}
	}
	return MaterialAttributeNames;
}

TArray<FString> GetMaterialAttributeNamesFromValidActor(AActor* Actor, FString AttributeType)
{
	TArray<UMaterialInterface*> Materials = GetAllMaterialInterfacesFromValidActor(Actor);
	TArray<FString> ParameterNames;
	for (UMaterialInterface* Material : Materials)
	{
		TArray<FMaterialParameterInfo> OutParameterInfo;
		TArray<FGuid> OutParameterIds;
		if (AttributeType == "vector")
			Material->GetAllVectorParameterInfo(OutParameterInfo, OutParameterIds);
		else if (AttributeType == "scalar")
			Material->GetAllScalarParameterInfo(OutParameterInfo, OutParameterIds);		
		else if (AttributeType == "texture")
			Material->GetAllTextureParameterInfo(OutParameterInfo, OutParameterIds);
		else if (AttributeType == "staticSwitch")
			Material->GetAllStaticSwitchParameterInfo(OutParameterInfo, OutParameterIds);
		else if (AttributeType == "componentMask")
			Material->GetAllStaticComponentMaskParameterInfo(OutParameterInfo, OutParameterIds);
		else
		{
			UE_LOG(LogPromethean, Display, TEXT("Request for an unknown material attribute type: %s"), *AttributeType);
		}

		for (FMaterialParameterInfo SingleParameterInfo : OutParameterInfo)
		{
			ParameterNames.Add(SingleParameterInfo.Name.ToString());			
		}
	}
	return ParameterNames;
}

// get material names

TArray<FString> GetMaterialPathsFromSelectedValidActors()
{
	TArray<AActor*> ValidActors = GetSelectedValidActors();
	TArray<FString> MaterialNames;
	for (auto& Actor : ValidActors)
	{
		MaterialNames.Append(GetMaterialPathsFromValidActor(Actor));
	}	
	return MaterialNames;
}

FString GetMaterialPathJSONDictFromActorsByName(const TArray<FString>& ActorNames, UWorld* World)
{
	TSharedPtr<FJsonObject> JsonDict = MakeShared<FJsonObject>();
	for (FString ActorName : ActorNames)
	{
		AActor* FoundActor = GetActorByName<AStaticMeshActor>(ActorName, World);
		if (FoundActor == nullptr)
		{
			// not found, try PrometheanMesh
			FoundActor = GetActorByName<APrometheanMeshActor>(ActorName, World);
		}
		if (FoundActor != nullptr)
		{
			TArray<FString> MaterialNames = GetMaterialPathsFromValidActor(FoundActor);
			TArray<TSharedPtr<FJsonValue>> JsonArray;
			UE_LOG(LogPromethean, Display, TEXT("Materials used by actor '%s':"), *ActorName);
			for (auto& MaterialName : MaterialNames)
			{
				JsonArray.Add(MakeShareable(new FJsonValueString(MaterialName)));
				UE_LOG(LogPromethean, Display, TEXT("%s"), *MaterialName);
			}
			JsonDict->SetArrayField(ActorName, JsonArray);
		}
	}

	FString OutputString;
	TSharedRef< TJsonWriter<> > Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(JsonDict.ToSharedRef(), Writer);
	return OutputString;
}

TArray<FString> getMaterialPathsFromStaticMeshActorByName(FString ActorName, UWorld* World)
{
	AStaticMeshActor* StaticMeshActor = GetActorByName<AStaticMeshActor>(ActorName, World);
	return GetMaterialPathsFromValidActor(StaticMeshActor);
}

TArray<FString> GetMaterialPathsFromValidActor(AActor* Actor)
{
	TArray<UMaterialInterface*> Materials = GetAllMaterialInterfacesFromValidActor(Actor);
	TArray<FString> MaterialPaths;
	for (UMaterialInterface* Material : Materials)
	{
		MaterialPaths.Add(Material->GetPathName());
	}
	return MaterialPaths;
}

// get material function dependencies

TArray<UMaterialFunctionInterface*> GetMaterialFunctionDependencies(const UMaterial& Material)
{
	TArray<UMaterialFunctionInterface*> OutArray;
	// TODO - fix 4.25 support
	/*
	for (auto& FunctionInfo : Material.MaterialFunctionInfos)
	{
		OutArray.Add(FunctionInfo.Function);
	}
	*/
	return OutArray;
}

template<class AssetType>
FString GetJSONImportSourceDataFromAsset(const FString& AssetPath)
{
	auto Asset = LoadObject<AssetType>(nullptr, *AssetPath, nullptr);
	FString SourcePaths;
	if (Asset != nullptr)
	{
		for (auto& SourceFile : Asset->AssetImportData->SourceData.SourceFiles)
		{
			if (SourcePaths.Len() > 0)
			{
				SourcePaths += ' ';
			}
			SourcePaths += SourceFile.RelativeFilename;
		}
	}
	return SourcePaths;
}

// generic

/*
	Takes JsonString and parse it, creating the material instance, setting all the attributes contained in the string.
	Json string example: {"/Game/ParentMaterial" : {"target" : "/Game/FinalResult/Instance", "attributes" : {"Texture" : "/Game/MyTexture", "Color" : [0.2, 0.7, 0.4], "Emissive" : "20.0", "BoolTest" : "true"}}}
*/
void CreateMaterialInstancesFromJsonString(const FString& JsonString)
{
	TSharedPtr<FJsonObject> JsonObject;
	if (!ParseJsonString(JsonString, JsonObject)) {
		return;
	}

	for (auto& JsonItem : JsonObject->Values)
	{
		auto ParentPath = JsonItem.Key;
		auto MaterialJsonObject = JsonItem.Value->AsObject();
		auto TargetPath = MaterialJsonObject->GetStringField("target");
		auto MaterialAttrs = MaterialJsonObject->GetObjectField("attributes");

		if (ParentPath.IsEmpty() || TargetPath.IsEmpty())
		{
			return;
		}

		auto ParentMaterial = LoadObject<UMaterial>(nullptr, *ParentPath);
		FName TargetName = *FPaths::GetBaseFilename(TargetPath);
		if (ParentMaterial != nullptr)
		{
			UMaterialInstanceConstant* NewMaterialInstance = NewObject<UMaterialInstanceConstant>(UMaterialInstanceConstant::StaticClass(), TargetName, RF_Public | RF_Standalone);
			NewMaterialInstance->Parent = ParentMaterial;

			TArray<FMaterialParameterInfo> MaterialInfo;
			TArray<FGuid> ParameterIDs;

			// Scalar
			NewMaterialInstance->GetAllScalarParameterInfo(MaterialInfo, ParameterIDs);
			for (FMaterialParameterInfo SingleParameterInfo : MaterialInfo)
			{
				for (auto& MAttr : MaterialAttrs->Values)
				{
					FName AttrName = *MAttr.Key;
					if (SingleParameterInfo.Name == AttrName)
					{
						// found a match, set value dependent on the parameter type.
						NewMaterialInstance->SetScalarParameterValueEditorOnly(SingleParameterInfo, (float)MAttr.Value->AsNumber());
						break;  // stop parsing attributes, move on to next dynamic parameter
					}
				}
			}

			MaterialInfo.Empty();
			ParameterIDs.Empty();

			// FVector
			NewMaterialInstance->GetAllVectorParameterInfo(MaterialInfo, ParameterIDs);
			for (FMaterialParameterInfo SingleParameterInfo : MaterialInfo)
			{
				for (auto& MAttr : MaterialAttrs->Values)
				{
					FName AttrName = *MAttr.Key;
					if (SingleParameterInfo.Name == AttrName)
					{
						// found a match, set value dependent on the parameter type.
						auto VArray = MAttr.Value->AsArray();
						NewMaterialInstance->SetVectorParameterValueEditorOnly(SingleParameterInfo, FLinearColor(VArray[0]->AsNumber(), VArray[1]->AsNumber(), VArray[2]->AsNumber()));
						break;  // stop parsing attributes, move on to next dynamic parameter
					}
				}
			}

			MaterialInfo.Empty();
			ParameterIDs.Empty();

			// Texture
			NewMaterialInstance->GetAllTextureParameterInfo(MaterialInfo, ParameterIDs);
			for (FMaterialParameterInfo SingleParameterInfo : MaterialInfo)
			{
				for (auto& MAttr : MaterialAttrs->Values)
				{
					FName AttrName = *MAttr.Key;
					if (SingleParameterInfo.Name == AttrName)
					{
						// found a match, set value dependent on the parameter type.
						auto TexturePath = MAttr.Value->AsString();
						UTexture* TextureParameter = LoadObject<UTexture>(nullptr, *TexturePath);
						if (TextureParameter != nullptr)
						{
							NewMaterialInstance->SetTextureParameterValueEditorOnly(SingleParameterInfo, TextureParameter);
						}
						break;  // stop parsing attributes, move on to next dynamic parameter
					}
				}
			}

			// Static Bools
			FStaticParameterSet StaticParameters;
			NewMaterialInstance->GetStaticParameterValues(StaticParameters);
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 2
			// TODO: this is very hacky! 
			for (FStaticSwitchParameter& StaticSwitch : StaticParameters.EditorOnly.StaticSwitchParameters_DEPRECATED)
#elif ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 1
			for (FStaticSwitchParameter& StaticSwitch : StaticParameters.EditorOnly.StaticSwitchParameters)
#else
			for (FStaticSwitchParameter& StaticSwitch : StaticParameters.StaticSwitchParameters)
#endif
			{
				for (auto& MAttr : MaterialAttrs->Values)
				{
					FName AttrName = *MAttr.Key;
					if (StaticSwitch.ParameterInfo.Name == AttrName)
					{
						// found a match, set value dependent on the parameter type.
						StaticSwitch.Value = MAttr.Value->AsBool();
						StaticSwitch.bOverride = true;
						break;  // stop parsing attributes, move on to next parameter
					}
				}
			}
			// Update static bools with changed parameters.
			NewMaterialInstance->UpdateStaticPermutation(StaticParameters);

			// Place new material instance into Content Browser, selecting it in Content Browser for user acknowledge
			TArray<UObject*> SelectedAssets;
			SelectedAssets.Add(AddObjectToContentBrowser(NewMaterialInstance, TargetPath));
			SetAssetBrowserSelection(SelectedAssets);
		}
	}
}

/*
	Parse the json string and sets the material inside specific index of the supplied Static Mesh asset from supplied path.
	Logging warning if provided material index is out of scope for the asset.
	Json string example: {"/Game/MyMesh/Chair" : [["/Game/M_Red", 2],[\"/Game/M_Blue", 0]]}
*/
void SetMeshAssetMaterialFromJsonString(const FString& JsonString)
{
	TSharedPtr<FJsonObject> JsonObject;
	if (!ParseJsonString(JsonString, JsonObject)) {
		return;
	}

	for (auto& JsonMeshObject : JsonObject->Values) {
		auto JsonMaterialObjects = JsonMeshObject.Value->AsArray();
		for (auto& JsonMaterialObject : JsonMaterialObjects)
		{
			auto Parameters = JsonMaterialObject->AsArray();
			if (Parameters.Num() >= 2)
			{
				FString MeshAssetPath = JsonMeshObject.Key;
				FString TargetMaterialPath = Parameters[0]->AsString();
				int32 MaterialIndex = (int32)Parameters[1]->AsNumber();

				auto MeshAsset = LoadObject<UStaticMesh>(nullptr, *MeshAssetPath);
				if (MeshAsset != nullptr)
				{

					if (MaterialIndex >= MeshAsset->GetStaticMaterials().Num())
					{
						UE_LOG(LogPromethean, Warning, TEXT("set_mesh_asset_material: provided material index (%d) is out of range for asset: %s"), MaterialIndex, *MeshAssetPath);
					}
					auto MaterialAsset = LoadObject<UMaterialInterface>(nullptr, *TargetMaterialPath);
					if (MaterialAsset != nullptr)
					{
						MeshAsset->SetMaterial(MaterialIndex, MaterialAsset);
						SaveToAsset(MeshAsset);
					}
				}
			}
		}
	}
}

/*
	Removes material override by assigning a default value to overriden material slot in the supplied actor.
*/
void RemoveMaterialOverride(const FString& ActorName, UWorld* World)
{
	auto Actor = GetActorByName<AActor>(ActorName, World);
	auto LearningSceneComponent = GetValidSceneComponent(Actor); // this function returns child static mesh components which are managed by standalone.
	auto PrimitiveMeshComponent = Cast<UPrimitiveComponent>(LearningSceneComponent);
	if (PrimitiveMeshComponent != nullptr)
	{
		auto DefaultObject = PrimitiveMeshComponent->GetClass()->GetDefaultObject<UPrimitiveComponent>();
		if (DefaultObject != nullptr)
		{
			for (int32 MaterialIndex = 0; MaterialIndex < PrimitiveMeshComponent->GetNumMaterials(); ++MaterialIndex)
			{
				PrimitiveMeshComponent->SetMaterial(MaterialIndex, DefaultObject->GetMaterial(MaterialIndex));
			}
		}
	}
}

TArray<UMaterialInterface*> GetAllMaterialInterfacesFromValidActor(AActor* Actor)
{
	TArray<UMaterialInterface*> OutMaterials;
	auto MeshComponent = GetValidMeshComponent(Actor);
	if (MeshComponent == nullptr)
	{
		return OutMaterials;
	}

	for(auto& Material : MeshComponent->GetMaterials())
	{
		OutMaterials.Add(Material);
	}
	return OutMaterials;
}


// --------------------------------------------------------------------
// --- Physics Simulation Functions
// -------------------------------------------------------------------


FString GetActorsPhysicsSimulationStateByName(const TArray<FString>& ActorNames)
{
	TSharedPtr<FJsonObject> RootObject = MakeShared<FJsonObject>();
	for (auto ActorName : ActorNames)
	{
		auto Actor = GetActorByName<AActor>(ActorName, GetEditorWorld());		
		if (Actor)
		{
			RootObject->SetBoolField(Actor->GetName().ToLower(), GetActorsPhysicalSimulationState(Actor));
		}
	}

	FString OutputString;
	TSharedRef< TJsonWriter<> > Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);

	return OutputString;

}

void SetActorsToBePhysicallySimulatedByName(const TArray<FString>& ActorNames, bool State)
{
	for (auto ActorName : ActorNames)
	{
		SetActorToBePhysicallySimulatedByName(ActorName, State);
	}
}

/*
	Find actors by name in the editor world and set them to be physically simulated
*/
void SetActorToBePhysicallySimulatedByName(const FString& ActorName, bool State)
{
	auto Actor = GetActorByName<AActor>(ActorName, GetEditorWorld());
	if (Actor)
	{
		SetActorToBePhysicallySimulated(Actor, State);
	}

}

/*
	Set given actors to be physically simulated
*/
void SetActorToBePhysicallySimulated(AActor* Actor, bool State)
{
	if (Actor)
	{
		UPrimitiveComponent* root = Cast<UPrimitiveComponent>(Actor->GetRootComponent());  // root component is USceneComponent* by default
		// check the Simulate Physics Checkbox
		if (root)
		{
			root->SetSimulatePhysics(State);
		}
		// Set Actor to be movable if actor is to be simulated
		if (State)
		{
			root->SetMobility(EComponentMobility::Movable);
		}
		else
		{
			root->SetMobility(EComponentMobility::Static);
		}

	}
}

bool GetActorsPhysicalSimulationState(AActor* Actor)
{
	if (Actor)
	{
		UPrimitiveComponent* root = Cast<UPrimitiveComponent>(Actor->GetRootComponent());  // root component is USceneComponent* by default
		// check the Simulate Physics Checkbox
		if (root)
		{
			return root->IsSimulatingPhysics();
		}
	}
	return false;
}


FString GetSimulatingActorsTransformJSONDictByNames(const TArray<FString>& ActorNames)
{
	TArray<AActor*> SimulatingActors = GetSimulatingActorsByName(ActorNames);
	FString OutJsonString = GetValidActorsTransformJSONDict(SimulatingActors);
	return OutJsonString;  // if OutJsonString is empty TCPSend will replace it with "None" to avoid sending no data
}
	

/*
	In case a simulation is running in the editor get actor references in the simulation world that gets instantiated on PIE
	return an array for pointers to actors with matching names. not found actors are not included. this is not intended to be 
	matched by order. rather by object name key returned by the likes of GetValidActorsTransformJSONDict()
*/
TArray<AActor*> GetSimulatingActorsByName(const TArray<FString>& ActorNames)
{
	TArray<AActor*> OutActors;
	for (auto ActorName : ActorNames)
	{
		AActor* Actor = GetSimulatingActorByName<AActor>(ActorName);
		if (Actor)
		{
			OutActors.Add(Actor);
		}
	}
	return OutActors;
}

/*
	In case a simulation is running in the editor get actor references in the simulation world that gets instantiated on PIE
	return nullptr if no simulation is currently in progress
*/
template<class ActorType>
ActorType* GetSimulatingActorByName(const FString& ActorName)
{
	if (GEditor->PlayWorld != NULL)
	{
		auto SimWorldActor = GetActorByName<AActor>(ActorName, GEditor->PlayWorld);
		return SimWorldActor;
	}
	return nullptr;
}


void EnablePlayInEditor()
{
	FEditorViewportClient* client = (FEditorViewportClient*)GEditor->GetActiveViewport()->GetClient();
	const FVector editorCameraPosition = client->GetViewLocation();
	const FRotator editorCameraRotation = client->GetViewRotation();

	FRequestPlaySessionParams Params;
	Params.StartLocation = editorCameraPosition;
	Params.StartRotation = editorCameraRotation;

	FLevelEditorModule& LevelEditorModule = FModuleManager::Get().GetModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));
	Params.DestinationSlateViewport = LevelEditorModule.GetFirstActiveViewport();
	Params.WorldType = EPlaySessionWorldType::SimulateInEditor;
	GEditor->RequestPlaySession(Params);
}

/*
	Kill play in editor session
	Wrapping in try/catch because it was causing sporadic crashes on 4.24
*/
void DisablePlayInEditor()
{
	if (GEditor->PlayWorld != NULL)
	{
		try
		{
			GEditor->RequestEndPlayMap();  // kill play in editor session
			UE_LOG(LogPromethean, Display, TEXT("Succesful Request End Play Map"));
		}
		catch (const std::exception& e)
		{
			UE_LOG(LogPromethean, Display, TEXT("Error Requesting End Play Map: "), *e.what());
		}
	}
}


// --------------------------------------------------------------------
// --- Scene Management Functions
// -------------------------------------------------------------------

FString addStaticMeshActors(UWorld* World, FString JsonString)
{	
	GEditor->SelectNone(false, false, false);  // deselect all
	// deserialize json str to a dictionary obj
	TSharedPtr<FJsonObject> InJsonObjects = MakeShared<FJsonObject>();
	TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(JsonString);		
	TArray<AActor*> CreatedActors;
	TArray<bool> DontSkipObjects; // because we rely on selected objects in order at the end to get the new DCC names
	FString json_data_to_send = "None"; // dummy text since empty strings are not sent
	if (FJsonSerializer::Deserialize(JsonReader, InJsonObjects))  // if got valid json data
	{	//UE_LOG(LogPromethean, Display, TEXT("String Deserialized"));		
		TArray<FString> ObjectNames;
		int num_objs = InJsonObjects->Values.GetKeys(ObjectNames);
		for (FString JsonObjectName : ObjectNames)  // iterate over dictionary key-value pairs andcreate variables
		{			
			bool IsGroup = InJsonObjects->GetObjectField(JsonObjectName)->GetBoolField("group");			
			FString Name = InJsonObjects->GetObjectField(JsonObjectName)->GetStringField("name");			
			FString MeshPath = InJsonObjects->GetObjectField(JsonObjectName)->GetStringField("asset_path");

			// --- LOCATION
			FVector Location;
			TArray<TSharedPtr<FJsonValue>> LocationArray = InJsonObjects->GetObjectField(JsonObjectName)->GetArrayField("location");
			if (LocationArray.Num() > 2)
			{
				Location = FVector(LocationArray[0]->AsNumber(), LocationArray[1]->AsNumber(), LocationArray[2]->AsNumber());
			}
			else
			{
				Location = FVector::ZeroVector;
			}

			// --- ROTATION
			FVector RotationVec;
			TArray<TSharedPtr<FJsonValue>> RotationArray = InJsonObjects->GetObjectField(JsonObjectName)->GetArrayField("rotation");
			if (RotationArray.Num() > 2)
			{
				RotationVec = FVector(RotationArray[0]->AsNumber(), RotationArray[1]->AsNumber(), RotationArray[2]->AsNumber());
			}
			else
			{
				RotationVec = FVector::ZeroVector;
			}

			// --- SCALE
			FVector Scale;
			TArray<TSharedPtr<FJsonValue>> ScaleArray = InJsonObjects->GetObjectField(JsonObjectName)->GetArrayField("scale");
			if (ScaleArray.Num() > 2)
			{
				Scale = FVector(ScaleArray[0]->AsNumber(), ScaleArray[1]->AsNumber(), ScaleArray[2]->AsNumber());
			}
			else
			{
				Scale = FVector::ZeroVector;
			}
			// -- RAYTRACE - check if we need to raytrace to update the location and rotation
			float RaytraceDistance = InJsonObjects->GetObjectField(JsonObjectName)->GetNumberField("raytrace_distance");
			if (RaytraceDistance)
			{
				FVector RayDirection = FVector(0.0, 0.0, RaytraceDistance); //  * 0.5;  // half it to offset up and down				
				FVector StartPoint = Location; // + RayDirection;
				FVector EndPoint = Location - RayDirection;
				FHitResult TraceResult = RayTrace(StartPoint, EndPoint, World, CreatedActors);
				if (!TraceResult.Location.IsZero())
				{
					Location = TraceResult.Location;
					Location.Z += FMath::FRandRange(0.0, 0.25); // fix Z-fighting for flat surfaces by offseting them ever so slightly
					
					// - if we had a location hit check if we need to update orientatio ntoo
					float RaytraceAlignment = InJsonObjects->GetObjectField(JsonObjectName)->GetNumberField("raytrace_alignment");					
					float RaytraceAlignmentMask = InJsonObjects->GetObjectField(JsonObjectName)->GetNumberField("raytrace_alignment_mask");					
					if (RaytraceAlignment || RaytraceAlignmentMask)
					{
						float UpDotProduct = FVector::DotProduct(FVector(0, 0, 1), TraceResult.Normal);						
						if (UpDotProduct < RaytraceAlignmentMask)
						{
							DontSkipObjects.Add(false);
							continue;  // don't create object
						}
						if (UpDotProduct > RaytraceAlignment)
						{							
							FQuat Quat = RotationFromNormal(TraceResult.Normal);
							RotationVec[0] = 0.0;  // zero out everything but rotation around up axis
							RotationVec[1] = 0.0;  // zero out everything but rotation around up axis						
							FQuat RootQuat = FQuat::MakeFromEuler(RotationVec);
							FQuat NewQuat = Quat * RootQuat;  // this has a bit of a drift and if repeated over time the rotation will settle into Quat					
							RotationVec = NewQuat.Euler();
						}
					}
				}
			}

			/* debug print out
			UE_LOG(LogPromethean, Display, TEXT("Old Dcc Name: %s"), *JsonObjectName);
			UE_LOG(LogPromethean, Display, TEXT("Is Group: %s"), IsGroup);
			UE_LOG(LogPromethean, Display, TEXT("Name: %s"), *Name);
			UE_LOG(LogPromethean, Display, TEXT("Asset Path: %s"), *MeshPath);
			UE_LOG(LogPromethean, Display, TEXT("Location: %s"), *Location.ToString());
			UE_LOG(LogPromethean, Display, TEXT("Rotation: %s"), *RotationVec.ToString());
			UE_LOG(LogPromethean, Display, TEXT("Scale: %s"), *Scale.ToString());
			*/
			// create new object
			AActor* CreatedActor;
			if (IsGroup)
				CreatedActor = addEmptyStaticMeshActor(World, Name, Location, RotationVec, Scale);
			else if (!MeshPath.IsEmpty())
				if (MeshPath.StartsWith("Blueprint'"))
				{ 
					CreatedActor = addBlueprintActor(World, MeshPath, Name, Location, RotationVec, Scale);
				}
				else
				{
					CreatedActor = addStaticMeshActor(World, MeshPath, Name, Location, RotationVec, Scale);
				}
				
			else  
			{				
				// WARNING! - hardcoded cube path and data!
				CreatedActor = addStaticMeshActor(World, "/Engine/BasicShapes/Cube.Cube", Name, Location, RotationVec, Scale);
				// StaticMesh'/Engine/BasicShapes/Cube.Cube'
			}
			
			// if all else fails 
			if (CreatedActor == nullptr)
			{
				CreatedActor = addStaticMeshActor(World, "/Engine/BasicShapes/Cube.Cube", "Missing_" + Name, Location, RotationVec, FVector(0.25, 0.25, 0.25));
			}

			// parent new object if parent was specified
			if (CreatedActor != nullptr)
			{
				FString ParentName = InJsonObjects->GetObjectField(JsonObjectName)->GetStringField("parent_dcc_name");
				if (ParentName.Len() > 0)
				{
					ParentObjectsByName(ParentName, CreatedActor->GetName(), World);
				}
			}
			CreatedActors.Add(CreatedActor);
			DontSkipObjects.Add(true);

		}
		// because we can skip object creation based on conditions have to recreate the name array in order to match selection
		TArray<FString> FilteredObjectNames;
		for (int i = 0; i < ObjectNames.Num(); i++)
		{
			if (DontSkipObjects[i])  // have to do this because we are selection order based
			{
				FilteredObjectNames.Add(ObjectNames[i]);
			}
		}
		// iterate over all selected objects and create a new json dict
		TArray<AActor*> SelectedActors = GetSelectedValidActors();
		TSharedPtr<FJsonObject> OutJsonObject = MakeShared<FJsonObject>();
		for (int i = 0; i < SelectedActors.Num(); i++)
		{
			OutJsonObject->SetStringField(FilteredObjectNames[i], SelectedActors[i]->GetName().ToLower());			
		}
		TSharedRef<TJsonWriter<>> json_writer = TJsonWriterFactory<>::Create(&json_data_to_send);
		FJsonSerializer::Serialize(OutJsonObject.ToSharedRef(), json_writer);		
	}
	return json_data_to_send;
}

TArray<AStaticMeshActor*> addStaticMeshActorOnSelection(FString MeshPath, UWorld* World)
{
	// place N meshes for every selected object, or 1 at origin if nothing is selected
	int32 dot_index;	
	MeshPath.FindLastChar('.', dot_index);  // name is to the right of . in path. +1 because will include . otherwise
	FString Name = MeshPath.RightChop(dot_index + 1);  // /Game/StarterContent/Props/SM_Chair.SM_Chair	
	FVector Rotation = FVector(0, 0, 0);
	FVector Scale = FVector(1, 1, 1);
	TArray<FVector> SelectionPositions = getSelectionPositions(World);
	GEditor->SelectNone(false, false, false);  // deselect everything 
	TArray<AStaticMeshActor*> NewActors;
	for (auto& Location : SelectionPositions)
	{		
		AStaticMeshActor* NewActor;
		NewActor = addStaticMeshActor(World, MeshPath, Name, Location, Rotation, Scale);
		if (NewActor != nullptr)
		{
			NewActors.Add(NewActor);
		}
	}
	// if no selection add at once origin
	if (SelectionPositions.Num() == 0)
	{
		AStaticMeshActor* NewActor;
		NewActor = addStaticMeshActor(World, MeshPath, Name, FVector(0,0,0), Rotation, Scale);		
		if (NewActor != nullptr)
		{
			NewActors.Add(NewActor);
		}
	}
	return NewActors;
}

TArray<AStaticMeshActor*> addStaticMeshActorsOnSelection(TArray<FString> MeshPaths, UWorld* World)
{
	// add all the actors
	TArray<AStaticMeshActor*> NewActors;
	for (auto MeshPath: MeshPaths)
	{
		TArray<AStaticMeshActor*> SingleMeshTypeActors;
		SingleMeshTypeActors = addStaticMeshActorOnSelection(MeshPath, World);
		if (SingleMeshTypeActors.Num())
		{			
			NewActors += SingleMeshTypeActors;
		}
	}
	// select all the new actors
	GEditor->SelectNone(false, false, false);  // deselect everything 
	for (auto NewActor : NewActors)
	{
		GEditor->SelectActor(NewActor, true, false);  // add to selection, first true is important
	}	
	return NewActors;
}

template<class ActorType>
ActorType* AddActorToWorld(UWorld* World, const FString& Name, const FVector& Location, const FVector& RotationVec, const FVector& Scale) {
	FRotator Rotation = FRotatorFromXYZVec(RotationVec);
	FActorSpawnParameters SpawnInfo;  //	SpawnInfo.Name = FName(*Name);  // didn't seem to work well. deletes the object with original name from scene but doesn't assign the name %/ 		
	auto obj = World->SpawnActor<ActorType>(ActorType::StaticClass(), Location, Rotation, SpawnInfo);

	obj->SetActorScale3D(Scale);
	if (Name != "")
	{
		auto unique_name = MakeUniqueObjectName(obj->GetOuter(), obj->GetClass(), *Name).ToString();  // obj->GetOuter() is important. was crashing with just obj
		obj->Rename(*unique_name);  // hangs indefinitely when trying to use additioanl parameters
		obj->SetActorLabel(obj->GetName());
	}
	GEditor->SelectActor(obj, true, false);  // add to selection, first true is important
	return obj;
}


AActor* addBlueprintActor(UWorld* World, FString MeshPath, FString Name, FVector Location, FVector RotationVec, FVector Scale)
{	
	/* https://www.reddit.com/r/unrealengine/comments/bjnwfp/comment/embwmk6/
	static const FString BPClassPath = TEXT("Blueprint'/Game/BP_SomeClass.BP_SomeClass_C'");
	const TSubclassOf<AActor> BPSubClass = Cast<UClass>(StaticLoadObject(UObject::StaticClass(), nullptr, *BPClassPath));

	if (BPSubClass != nullptr)
	{
		FActorSpawnParameters SpawnInfo;
		SpawnInfo.Owner = PlayerPawn; // <- just an example, don't remember ownership in detail
		SpawnInfo.Instigator = PlayerPawn;
		SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnInfo.ObjectFlags |= RF_Transient;
		GetWorld()->SpawnActor(BPSubClass, &SomeTransform, SpawnInfo);
	}

	UClass* MyItemBlueprintClass = StaticLoadClass(UObject::StaticClass(), NULL, TEXT("/Game/Weapons/axes/DoubleAxeActor.DoubleAxeActor_C"), NULL, LOAD_None, NULL);
	FActorSpawnParameters SpawnInfo;
	SpawnInfo.Owner = this;
	SpawnInfo.Instigator = Instigator;
	ApsItemActor* obj = spawnManager->currentWorld->SpawnActor(MyItemBlueprintClass, newlocation, GetActorRotation(), SpawnInfo);

	* */	
	const TSubclassOf<AActor> BPSubClass = Cast<UClass>(StaticLoadObject(UObject::StaticClass(), nullptr, *MeshPath));	
	// UClass* BPSubClass = StaticLoadClass(UObject::StaticClass(), NULL, *MeshPath, NULL, LOAD_None, NULL);
	UE_LOG(LogPromethean, Display, TEXT("Adding blueprint: %s"), *MeshPath);
	if (BPSubClass == nullptr)
	{
		UE_LOG(LogPromethean, Display, TEXT("Failed to load blueprint: %s"), *MeshPath);
		return nullptr;
	}
	UE_LOG(LogPromethean, Display, TEXT("1"));
	FRotator Rotation = FRotatorFromXYZVec(RotationVec);
	FActorSpawnParameters SpawnInfo;  //	SpawnInfo.Name = FName(*Name);  // didn't seem to work well. deletes the object with original name from scene but doesn't assign the name %/ 		
	SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	// const FVector ConstLocation = Location;
	UE_LOG(LogPromethean, Display, TEXT("2"));
	AActor* obj = World->SpawnActor(BPSubClass, &Location, &Rotation, SpawnInfo);  // why did we have to pass Location and Rotation by reference?
	
	UE_LOG(LogPromethean, Display, TEXT("3"));
	if (obj != nullptr)
	{
		obj->SetActorScale3D(Scale);
		if (Name != "")
		{
			UE_LOG(LogPromethean, Display, TEXT("4"));
			Name = MakeUniqueObjectName(obj->GetOuter(), obj->GetClass(), *Name).ToString();  // obj->GetOuter() is important. was crashing with just obj
			obj->Rename(*Name);  // hangs indefinitely when trying to use additioanl parameters
			obj->SetActorLabel(obj->GetName());
		}
		GEditor->SelectActor(obj, true, false);  // add to selection, first true is important
		UE_LOG(LogPromethean, Display, TEXT("5"));
	}
	return obj;
}


// Adds Static Mesh Actor to world, returns nullptr when asset not found;
AStaticMeshActor* addStaticMeshActor(UWorld* World, FString MeshPath, FString Name, FVector Location, FVector RotationVec, FVector Scale)
{
	FString FullMeshReference = "StaticMesh'" + MeshPath + "'";  // StaticMesh'/Game/StarterContent/Props/SM_Chair.SM_Chair'
	UStaticMesh* MeshAsset = Cast<UStaticMesh>(StaticLoadObject(UStaticMesh::StaticClass(), nullptr, *FullMeshReference));
	// UE_LOG(LogPromethean, Display, TEXT("Location Vector: %s"), *Location.ToString()); // sometimes location just flies away even with correct input
	if (MeshAsset != nullptr)
	{
		auto obj = addEmptyStaticMeshActor(World, Name, Location, RotationVec, Scale);
		if (obj != nullptr)
		{
			obj->GetStaticMeshComponent()->SetStaticMesh(MeshAsset);
		}
		return obj;
	}
	else
	{
		UE_LOG(LogPromethean, Display, TEXT("Warning! Static Mesh Not Found: %s"), *FullMeshReference);
		return nullptr;
	}
}

AStaticMeshActor* addEmptyStaticMeshActor(UWorld* World, FString Name, FVector Location, FVector RotationVec, FVector Scale)
{	// we don't want to be able to create an empty actor when're trying to have a proper one. so this is better off separate
	FRotator Rotation = FRotatorFromXYZVec(RotationVec);
	FActorSpawnParameters SpawnInfo;  //	SpawnInfo.Name = FName(*Name);  // didn't seem to work well. deletes the object with original name from scene but doesn't assign the name %/ 		
	SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AStaticMeshActor* obj = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), Location, Rotation, SpawnInfo);

	if (obj != nullptr)
	{
		obj->SetActorScale3D(Scale);
		if (Name != "")
		{
			Name = MakeUniqueObjectName(obj->GetOuter(), obj->GetClass(), *Name).ToString();  // obj->GetOuter() is important. was crashing with just obj
			obj->Rename(*Name);  // hangs indefinitely when trying to use additioanl parameters
			obj->SetActorLabel(obj->GetName());
		}
		GEditor->SelectActor(obj, true, false);  // add to selection, first true is important
	}
	return obj;
}

void SetStaticMeshAssetForValidActor(AActor* ValidActor, FString MeshPath)
{
	FString FullMeshReference = "StaticMesh'" + MeshPath + "'";  // StaticMesh'/Game/StarterContent/Props/SM_Chair.SM_Chair'
	UStaticMesh* MeshAsset = Cast<UStaticMesh>(StaticLoadObject(UStaticMesh::StaticClass(), NULL, *FullMeshReference));
	if (MeshAsset)
	{
		auto StaticMesh = GetValidStaticMeshComponent(ValidActor);
		StaticMesh->Modify();
		StaticMesh->SetStaticMesh(MeshAsset);		
	}
}

template<class ActorType>
ActorType* GetActorByName(const FString& ActorName, UWorld* World)
{
	for (TActorIterator<ActorType> ActorItr(World); ActorItr; ++ActorItr)
	{
		if (ActorItr->GetName().ToLower() == ActorName)
		{
			return Cast<ActorType>(*ActorItr);
		}
	}
	UE_LOG(LogPromethean, Display, TEXT("%s: '%s' not found in the scene"), *(ActorType::StaticClass()->GetFName().ToString()), *ActorName);
	return nullptr;
}

TArray<AActor*> GetRenderedActors()
{		
	float MostRecentTime = NegativeInfinity;
	TArray<AActor*> CurrentlyRenderedActors;
	
	//for (TObjectIterator<AActor> Itr; Itr; ++Itr)  //Iterate Over AActors - starting at 4.25 this started to return wrong array when chaning levels
	for (TActorIterator<AActor> Itr(GetEditorWorld()); Itr; ++Itr)
	{
		if (Itr->GetLastRenderTime() > MostRecentTime)  // latest rendered is what's on screen
		{
			MostRecentTime = Itr->GetLastRenderTime(); // time seems to be in seconds starting from editor start
			CurrentlyRenderedActors.Empty();			
		}
		if (Itr->GetLastRenderTime() == MostRecentTime)
		{
			CurrentlyRenderedActors.Add(*Itr);
		}
	}
	return CurrentlyRenderedActors;	
}

bool ActorHasStaticMeshComponent(AActor* Actor)
{
	TArray<UStaticMeshComponent*> ActorStaticMeshComponents;
	Actor->GetComponents<UStaticMeshComponent>(ActorStaticMeshComponents);
	return ActorStaticMeshComponents.Num() > 0;
}

void FocusOnActorsByName(const TArray<FString>& ObjectNames, UWorld* World)
{
	TArray<AActor*> Actors;
	for (auto& ObjectName : ObjectNames)
	{
		Actors.Add(Cast<AActor>(GetActorByName<AStaticMeshActor>(ObjectName.TrimStartAndEnd(), World)));
	}
	GEditor->MoveViewportCamerasToActor(Actors, false);  // second argument is AciveViewPortOnly
}

void SelectSceneActorsByName(const TArray<FString>& ObjectNames)
{
	// if multiple objects have the same name will select the first one
	// if multiple objects have the same and you send the same name twice it will still only select the first one
	// if you send a name of object that doesn't exist - you are fine, nothing will crash
	GEditor->SelectNone(false, false, false);  // deselect everything 
	for (auto& ObjectName : ObjectNames)
	{
		UE_LOG(LogPromethean, Display, TEXT("Select: %s"), *ObjectName);
		GEditor->SelectNamedActor(*ObjectName);
	}
}

void SetActorsHiddenByName(const TArray<FString>& ObjectNames, UWorld* World)
{
	for (auto& ObjectName : ObjectNames)	
	{		
		AActor* Actor = GetActorByName<AActor>(ObjectName.TrimStartAndEnd(), World);
		if (Actor)
		{
			Actor->Modify(); // register undo
			Actor->SetIsTemporarilyHiddenInEditor(true);
		}
	}
}

void SetActorsVisibleByName(const TArray<FString>& ObjectNames, UWorld* World)
{
	for (auto& ObjectName : ObjectNames)
	{
		AActor* Actor = GetActorByName<AActor>(ObjectName.TrimStartAndEnd(), World);
		if (Actor)
		{
			Actor->Modify(); // register undo
			Actor->SetIsTemporarilyHiddenInEditor(false);
		}
	}
}

// deprecated?
void SetStaticMeshActorsHiddenByName(const TArray<FString>& ObjectNames, UWorld* World)
{
	for (auto& ObjectName : ObjectNames)
	{
		AStaticMeshActor* Actor = GetActorByName<AStaticMeshActor>(ObjectName.TrimStartAndEnd(), World);		
		if (Actor)
		{
			Actor->SetIsTemporarilyHiddenInEditor(true);
		}
	}
}

// deprecated?
void SetStaticMeshActorsVisibleByName(const TArray<FString>& ObjectNames, UWorld* World)
{
	for (auto& ObjectName : ObjectNames)
	{
		AStaticMeshActor* Actor = GetActorByName<AStaticMeshActor>(ObjectName.TrimStartAndEnd(), World);
		if (Actor)
		{
			Actor->SetIsTemporarilyHiddenInEditor(false);
		}
	}
}

TArray<FString> GetParentsForObjectsByName(const TArray<FString>& ObjectNames, UWorld* World)
{	// take an array of objects names and return the names of their parents in the same order
	TArray<FString> ParentNames;
	for (auto& ObjectName : ObjectNames)
	{
		FString ParentName = GetParentForObjectByName(ObjectName, World);
		ParentNames.Add(ParentName);
	}
	return ParentNames;
}

FString GetParentForObjectByName(FString ObjectName, UWorld* World)
{
	auto Object = GetActorByName<AActor>(ObjectName.TrimStartAndEnd(), World);	
	FString NoParentName = TEXT("No_Parent");  // Warning! Has to be consistent with other DCCs
	if (Object)
	{
		//auto Parent = Object->GetParentActor();
		auto Parent = Object->GetAttachParentActor();
		if (Parent)
		{
			FString ParentName = Parent->GetName().ToLower();
			return ParentName;
		}
		else
		{
			return NoParentName;
		}
	}
	return NoParentName;
}

TArray<AActor*> GetAllObjectsIntersectingGivenObjects(TArray<AActor*> Objects)
{
	TArray<AActor*> OutActors;
	TArray<AActor*> TempActors;
	for (auto& Object : Objects)
	{
		TempActors.Empty();
		Object->GetOverlappingActors(TempActors);  // get all actors
		// filter valid actors
		for (auto& TempActor: TempActors)
		{
			auto Valid = ValidateActor(TempActor);
			if (Valid != nullptr)
			{
				OutActors.Add(Valid);
			}
		}
	}
	return OutActors;
}

TArray<AActor*> GetAllDescendentsForObjectsRecursive(TArray<AActor*> Objects, UWorld* World)
{
	TArray<AActor*> OutActors;
	for (auto& Object: Objects)
	{
		OutActors += GetAllDescendentsForObjectRecursive(Object, World);
	}
	return OutActors;
}

TArray<AActor*> GetAllDescendentsForObjectRecursive(AActor* Object, UWorld* World)
{
	TArray<AActor*> OutActors;
	OutActors.Empty();
	if (Object != nullptr)
	{		
		TArray<AActor*> Children;  // inout argument
		Object->GetAttachedActors(Children, true);
		OutActors += Children;
		for (auto& Child : Children)
		{
			OutActors += GetAllDescendentsForObjectRecursive(Child, World);
		}
	}
	return OutActors;
}

void ParentAllObjectsByName(FString ParentName, const TArray<FString>& ChildNames, UWorld* World)
{  // take an array of children names and parent them all to the parent name
	for (auto& ChildName : ChildNames)
	{
		ParentObjectsByName(ParentName, ChildName, World);
	}
}

void ParentObjectsByName(FString ParentName, FString ChildName, UWorld* World)
{
	// parent one object to the other
	// wrong names are fine, however
	// TODO: WARNING! crahses if spaces at start/end
	auto Parent = GetActorByName<AActor>(ParentName.TrimStartAndEnd(), World);
	auto Child = GetActorByName<AActor>(ChildName.TrimStartAndEnd(), World);
	if (Parent)
	{
		USceneComponent* RootComponent = Parent->GetRootComponent();  // root component is USceneComponent* by default
		if (RootComponent && RootComponent->Mobility != EComponentMobility::Static)
		{
			// Only set the mobility to static if it's not already static
			RootComponent->SetMobility(EComponentMobility::Static);
		}
	}

	if (GEditor->CanParentActors(Parent, Child))
	{
		GEditor->ParentActors(Parent, Child, NAME_None);  // socket name is None
	}
}


void UnparentObjectsByName(const TArray<FString>& Names, UWorld* World)
{  // take an array of children names and parent them all to the parent name
	for (auto& Name : Names)
	{
		UnparentObjectByName(Name, World);
	}
}

void UnparentObjectByName(FString Name, UWorld* World)
{
	auto Object = GetActorByName<AActor>(Name.TrimStartAndEnd(), World);
	if (Object)
		Object->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);  //  bool bMaintainWorldPosition
}


void FixObjectNames(UWorld* World)
{
	TArray<AActor*> AllValidActors = GetWorldValidActors(World);
	for (auto& ValidActor : AllValidActors)
	{		
		ValidActor->SetActorLabel(ValidActor->GetName());
	}	
}

void RenameActor(FString SourceName, FString TargetName, UWorld* World)
{
	// TODO: get this to return the resulting DCC names. For now just one sided
	auto ActorToRename = GetActorByName<AActor>(SourceName.TrimStartAndEnd(), World);	
	if (ActorToRename != nullptr)
	{
		// Check if actor with same target name already exists since renaming to existing name will crash UE4
		auto ExistingTargetActor = GetActorByName<AActor>(TargetName.TrimStartAndEnd(), World);
		if (ExistingTargetActor != nullptr)
		{
			int i = -1;
			FString IString;
			FString NewUniqueName = TargetName.TrimStartAndEnd();
			while (ExistingTargetActor != nullptr)
			{
				i += 1;
				IString = FString::FromInt(i);				
				ExistingTargetActor = GetActorByName<AActor>(NewUniqueName + TEXT("_") + IString, World);
			}
			TargetName = NewUniqueName + TEXT("_") + IString;
		}
		// Rename
		FString CurrentName = ActorToRename->GetName();
		if (CurrentName != TargetName)  // unreal crahses trying to rename to the same name
		{

			ActorToRename->Rename(*TargetName);
			ActorToRename->SetActorLabel(*TargetName);
		}
	}
}

void ToggleKillNamesOnSelection()
{	// names with _kill_ are igrnored by Promethean
	FString IgnoreWord = "_kill_";
	FString Blank = "";
	TArray<AActor*> SelectedValidActors = GetSelectedValidActors();
	for (auto& ValidActor : SelectedValidActors)
	{
		FString CurrentName = ValidActor->GetName();
		FString NewName = CurrentName;
		if (CurrentName.Contains(IgnoreWord))
		{			
			NewName = CurrentName.Replace(*IgnoreWord, *Blank);
		}
		else
		{
			NewName = CurrentName.Append(*IgnoreWord);
		}
		ValidActor->Rename(*NewName);
		ValidActor->SetActorLabel(ValidActor->GetName());
	}
}

void OpenLevel(FString LevelPath)
{	
	FEditorFileUtils::SaveDirtyPackages(false, true, false, false);
	FEditorFileUtils::LoadMap(LevelPath, false, true);  // "/Game/Maps/mapname.mapname"
}

void LoadLevel(FString LevelPath, UWorld* World)
{
	// "/Game/Maps/mapname.mapname" or "/Game/Maps/mapname" - both ok	
	TArray<FString> LevelPaths;
	LevelPaths.Add(LevelPath);
	EditorLevelUtils::AddLevelsToWorld(World, LevelPaths, ULevelStreamingDynamic::StaticClass());
	World->UpdateAllSkyCaptures();  // updates the global reflection probe to reflect new levels
	// GEditor->BuildReflectionCaptures();  // optionally update all reflection captures as well?
}

void UnloadLevel(FString LevelPath, UWorld* World)
{
	// - fix path in case full reference is passed - "/Game/Maps/mapname.mapname" or "/Game/Maps/mapname" - both ok	
	int DotIndex = LevelPath.Find(".");
	if (DotIndex > 0) 
	{
		LevelPath = LevelPath.Left(DotIndex);
	}
	// - iterate through all levels to find a path fit
	LevelPath = "Level " + LevelPath + "."; // "Level /Game/Promethean/test_levels/debug/U_shaped_test_00.U_shaped_test_00:PersistentLevel"
	auto CurrentLevels = World->GetLevels();
	for (auto& CurrentLevel : CurrentLevels) 
	{		
		if (CurrentLevel->GetFullName().StartsWith(LevelPath))
		{
			EditorLevelUtils::RemoveLevelFromWorld(CurrentLevel);
			return;
		}		
	}
	UE_LOG(LogPromethean, Display, TEXT("Couldn't unload %s"), *LevelPath);
}

void SetLevelVisibility(FString LevelPath, bool IsVisible, UWorld* World)
{
	// - fix path in case full reference is passed - "/Game/Maps/mapname.mapname" or "/Game/Maps/mapname" - both ok	
	int DotIndex = LevelPath.Find(".");
	if (DotIndex > 0)
	{
		LevelPath = LevelPath.Left(DotIndex);
	}
	// - iterate through all levels to find a path fit
	LevelPath = "Level " + LevelPath + "."; // "Level /Game/Promethean/test_levels/debug/U_shaped_test_00.U_shaped_test_00:PersistentLevel"
	auto CurrentLevels = World->GetLevels();
	for (auto& CurrentLevel : CurrentLevels)
	{
		if (CurrentLevel->GetFullName().StartsWith(LevelPath))
		{
			EditorLevelUtils::SetLevelVisibility(CurrentLevel, IsVisible, true);  // last parameter is bForceLayersVisible
			return;
		}
	}
	UE_LOG(LogPromethean, Display, TEXT("Couldn't set level visibility: %s"), *LevelPath);
}


void SetLevelCurrent(FString LevelPath, UWorld* World)
{
	// - fix path in case full reference is passed - "/Game/Maps/mapname.mapname" or "/Game/Maps/mapname" - both ok	
	int DotIndex = LevelPath.Find(".");
	if (DotIndex > 0)
	{
		LevelPath = LevelPath.Left(DotIndex);
	}
	// - iterate through all levels to find a path fit
	LevelPath = "Level " + LevelPath + "."; // "Level /Game/Promethean/test_levels/debug/U_shaped_test_00.U_shaped_test_00:PersistentLevel"
	auto CurrentLevels = World->GetLevels();
	for (auto& CurrentLevel : CurrentLevels)
	{
		if (CurrentLevel->GetFullName().StartsWith(LevelPath))
		{
			EditorLevelUtils::MakeLevelCurrent(CurrentLevel, true);  // ULevel* InLevel, bool bEvenIfLocked				
			return;
		}
	}
	UE_LOG(LogPromethean, Display, TEXT("Couldn't set level current: %s"), *LevelPath);
	return;
}


FString GetCurrentLevelPath(UWorld* World)
{
	return World->PersistentLevel->GetFullName();
}

// --------------------------------------------------------------------
// --- Asset Library Functions
// --------------------------------------------------------------------

FString getSceneName(UWorld* World)
{
	return World->GetMapName();
	//return GEditor->UserOpenedFile;
}

FString GetAssetBrowserSelection()
{
	TArray<FAssetData> Selections;
	GEditor->GetContentBrowserSelections(Selections);
	FString outString;
	for (auto & Selection : Selections)
		outString += Selection.GetFullName().Replace(TEXT(" "), TEXT("'")) + "'\n";  // adding ' ' around path to match copy refrence path from right click in editor and consistency with other functions
	return outString;
}

void SetAssetBrowserSelection(const TArray<UObject*>& SelectedObjects)
{
	FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	ContentBrowserModule.Get().SyncBrowserToAssets(SelectedObjects);
}

FString getJSONAssetDataFromAssetBrowserSelection()
{
	TSharedPtr<FJsonObject> RootObject = MakeShared<FJsonObject>();
	TArray<FAssetData> Selections;
	GEditor->GetContentBrowserSelections(Selections);
	FString temp_key;
	for (auto & SelectionAssetData : Selections)
	{
		temp_key = SelectionAssetData.GetFullName().Replace(TEXT(" "), TEXT("'")) + "'";  // adding ' ' around path to match copy refrence path from right click in editor and consistency with other functions		
		// UE_LOG(LogPromethean, Display, TEXT("Asset Browser Selection Item: %s"), *temp_key);
		if (temp_key.StartsWith("StaticMesh"))			
			RootObject->SetObjectField(temp_key, getStaticMeshAssetData(SelectionAssetData));
		else if (temp_key.StartsWith("MaterialInstanceConstant"))   // needs to go before material
			RootObject->SetObjectField(temp_key, getMaterialInstanceAssetData(SelectionAssetData));
		else if (temp_key.StartsWith("Material"))
			RootObject->SetObjectField(temp_key, getMaterialAssetData(SelectionAssetData));		
		else if (temp_key.StartsWith("Texture2D"))
			RootObject->SetObjectField(temp_key, getTextureAssetData(SelectionAssetData));
		else if (temp_key.StartsWith("World"))
			RootObject->SetObjectField(temp_key, getLevelAssetData(SelectionAssetData));
	}	
	FString OutputString;
	TSharedRef< TJsonWriter<> > Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);
	return OutputString;
}

UObject* GetAssetUObject(FString AssetReferencePath)
{
	UClass * Class = UStaticMesh::StaticClass();
	if (AssetReferencePath.StartsWith("StaticMesh"))
		Class = UStaticMesh::StaticClass();
	if (AssetReferencePath.StartsWith("Material"))
		Class = UMaterial::StaticClass();
	if (AssetReferencePath.StartsWith("MaterialInstanceConstant"))
		Class = UMaterialInstanceConstant::StaticClass();
	if (AssetReferencePath.StartsWith("Texture2D"))
		Class = UTexture2D::StaticClass();
	if (AssetReferencePath.StartsWith("Blueprint"))
		Class = UBlueprint::StaticClass();
	if (AssetReferencePath.StartsWith("World"))
		Class = UWorld::StaticClass();

	UObject* Asset = StaticLoadObject(Class, NULL, *AssetReferencePath);
	if (Asset)
	{
		return Asset;
	}
	return nullptr;
}


FString getAllExistingAssetByType(FString OutputPath, FString AssetType)
{
	/// <summary>
	/// get a list of asset strings including their unreal asset type:
	/// StaticMesh'/Game/Furniture/Beds/Lago_Bed/Lago_Bed_Structure.Lago_Bed_Structure'
	/// check whether they exist
	/// </summary>
	/// <param name="AssetReferencePaths"></param>
	/// <returns>json dict of 0s and 1s for whether these are actual existing assets</returns>
	
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	TArray<FAssetData> AssetData;

	// const UClass* Class = UStaticMesh::StaticClass();
	UClass* Class = UStaticMesh::StaticClass();
	if (AssetType.StartsWith("StaticMesh"))
		Class = UStaticMesh::StaticClass();
	if (AssetType.StartsWith("Material"))
		Class = UMaterial::StaticClass();
	if (AssetType.StartsWith("MaterialInstanceConstant"))
		Class = UMaterialInstanceConstant::StaticClass();
	if (AssetType.StartsWith("Texture2D"))
		Class = UTexture2D::StaticClass();
	if (AssetType.StartsWith("Blueprint"))
		Class = UBlueprint::StaticClass();


#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 1
	AssetRegistryModule.Get().GetAssetsByClass(Class->GetClassPathName(), AssetData);
#else
	AssetRegistryModule.Get().GetAssetsByClass(Class->GetFName(), AssetData);
#endif

	TSharedPtr<FJsonObject> RootObject = MakeShared<FJsonObject>();	
	TArray<TSharedPtr<FJsonValue>> AssetPaths;		

	for (auto& Asset : AssetData)
	{		
		// AssetPaths.Add(Asset.PackagePath.ToString() + "/" + Asset.PackageName.ToString());
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 1
		TSharedRef<FJsonValueString> MeshPivotX = MakeShared<FJsonValueString>(Asset.GetSoftObjectPath().ToString());
#else
		TSharedRef<FJsonValueString> MeshPivotX = MakeShared<FJsonValueString>(Asset.ObjectPath.ToString());
#endif
		AssetPaths.Add(MeshPivotX);
	}
	
	RootObject->SetArrayField("Assets", AssetPaths);	

	FString OutputString;
	TSharedRef< TJsonWriter<> > Writer = TJsonWriterFactory<>::Create(&OutputString);	
	FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);
	
	SaveToFile(OutputPath, OutputString);  // no file format is currently reserved for vertexes
	return OutputString;
}

DECLARE_STATS_GROUP(TEXT("Promethean_AI"), STATGROUP_PrometheanGeneric, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("Promethean - getJSONAssetDataFromAssetPaths"), STAT_GetJSONAssetDataFromAssetPaths, STATGROUP_PrometheanGeneric);
FString getJSONAssetDataFromAssetPaths(TArray<FString> AssetReferencePaths)
{
	SCOPE_CYCLE_COUNTER(STAT_GetJSONAssetDataFromAssetPaths);
	TSharedPtr<FJsonObject> RootObject = MakeShared<FJsonObject>();
	TArray<FAssetData> Assets;
	const IAssetRegistry* AssetRegistry = IAssetRegistry::Get();
	
	for (const FString& AssetReferencePath : AssetReferencePaths)
	{
		FAssetData AssetData;
		// First try to get AssetData without loading object
		if (AssetRegistry)
		{
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 1
			AssetData = AssetRegistry->GetAssetByObjectPath(FSoftObjectPath(AssetReferencePath));
#else
			AssetData = AssetRegistry->GetAssetByObjectPath(*AssetReferencePath);
#endif
		}

		// If unsuccessful, load it baby
		if (!AssetData.IsValid())
		{
			if (UObject* Asset = GetAssetUObject(AssetReferencePath))
			{
				AssetData = FAssetData(Asset);
			}
			else
			{
				UE_LOG(LogPromethean, Display, TEXT("No asset found for %s"), *AssetReferencePath);
				RootObject->SetObjectField(AssetReferencePath, getUnknownAssetData());
			}
		}

		if (AssetData.IsValid())
		{
			Assets.Add(AssetData);
		}
	}
		
	FString temp_key;
	for (const FAssetData& SelectionAssetData : Assets)
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 1
	{
		temp_key = SelectionAssetData.GetFullName().Replace(TEXT(" "), TEXT("'")) + "'";  // adding ' ' around path to match copy refrence path from right click in editor and consistency with other functions		
		if (SelectionAssetData.AssetClassPath.GetAssetName().IsEqual("StaticMesh"))
			RootObject->SetObjectField(temp_key, getStaticMeshAssetData(SelectionAssetData));
		else if (SelectionAssetData.AssetClassPath.GetAssetName().IsEqual("MaterialInstanceConstant"))   // needs to go before material
			RootObject->SetObjectField(temp_key, getMaterialInstanceAssetData(SelectionAssetData));
		else if (SelectionAssetData.AssetClassPath.GetAssetName().IsEqual("Material"))
			RootObject->SetObjectField(temp_key, getMaterialAssetData(SelectionAssetData));
		else if (SelectionAssetData.AssetClassPath.GetAssetName().IsEqual("Texture2D"))
			RootObject->SetObjectField(temp_key, getTextureAssetData(SelectionAssetData));
		else if (SelectionAssetData.AssetClassPath.GetAssetName().IsEqual("World"))
			RootObject->SetObjectField(temp_key, getLevelAssetData(SelectionAssetData));
		else if (SelectionAssetData.AssetClassPath.GetAssetName().IsEqual("Blueprint"))  // Blueprint'/Game/Ammo_and_bullets_Pack/Blueprints/BP_12GaMidBox.BP_12GaMidBox'
			RootObject->SetObjectField(temp_key, getBlueprintAssetData(SelectionAssetData));
		else
		{
			UE_LOG(LogPromethean, Display, TEXT("Unsupported data type"));
			RootObject->SetObjectField(SelectionAssetData.AssetClassPath.GetAssetName().ToString(), getUnknownAssetData());
		}
	}
#else
	{
		temp_key = SelectionAssetData.GetFullName().Replace(TEXT(" "), TEXT("'")) + "'";  // adding ' ' around path to match copy refrence path from right click in editor and consistency with other functions		
		// UE_LOG(LogPromethean, Display, TEXT("Asset Browser Selection Item: %s"), *temp_key);
		if (temp_key.StartsWith("StaticMesh"))
			RootObject->SetObjectField(temp_key, getStaticMeshAssetData(SelectionAssetData));
		else if (temp_key.StartsWith("MaterialInstanceConstant"))   // needs to go before material
			RootObject->SetObjectField(temp_key, getMaterialInstanceAssetData(SelectionAssetData));
		else if (temp_key.StartsWith("Material"))
			RootObject->SetObjectField(temp_key, getMaterialAssetData(SelectionAssetData));
		else if (temp_key.StartsWith("Texture2D"))
			RootObject->SetObjectField(temp_key, getTextureAssetData(SelectionAssetData));
		else if (temp_key.StartsWith("World"))
			RootObject->SetObjectField(temp_key, getLevelAssetData(SelectionAssetData));
		else if (temp_key.StartsWith("Blueprint"))  // Blueprint'/Game/Ammo_and_bullets_Pack/Blueprints/BP_12GaMidBox.BP_12GaMidBox'
			RootObject->SetObjectField(temp_key, getBlueprintAssetData(SelectionAssetData));
		else
		{
			UE_LOG(LogPromethean, Display, TEXT("Unsupported data type"));
			RootObject->SetObjectField(temp_key, getUnknownAssetData());
		}
	}
#endif
	FString OutputString;
	TSharedRef< TJsonWriter<> > Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);
	return OutputString;
}

void EditAsset(FString AssetReferencePath)
{
	UClass * Class = UStaticMesh::StaticClass();
	if (AssetReferencePath.StartsWith("StaticMesh"))
		Class = UStaticMesh::StaticClass();
	if (AssetReferencePath.StartsWith("Material"))
		Class = UMaterial::StaticClass();
	if (AssetReferencePath.StartsWith("MaterialInstanceConstant"))
		Class = UMaterialInstanceConstant::StaticClass();
	if (AssetReferencePath.StartsWith("Texture2D"))
		Class = UTexture2D::StaticClass();

	UObject* Asset = StaticLoadObject(Class, NULL, *AssetReferencePath);
	if (Asset)
	{
		GEditor->EditObject(Asset);
	}
}

void FindAsset(FString AssetReferencePath)
{
	UObject* Asset = GetAssetUObject(AssetReferencePath);
	if (Asset)
	{
		TArray <UObject*> Assets;
		Assets.Add(Asset);
		GEditor->SyncBrowserToObjects(Assets);
	}
}

void FindAssets(TArray<FString> AssetReferencePaths)
{
	TArray <UObject*> Assets;
	for (auto & AssetReferencePath : AssetReferencePaths)
	{
		UObject* Asset = GetAssetUObject(AssetReferencePath);
		if (Asset)
		{
			Assets.Add(Asset);
		}
	}
	GEditor->SyncBrowserToObjects(Assets);
}

void LoadAssets(TArray<FString> AssetReferencePaths)
{
	// to perform some operations (like drag and drop) assets need to be preloaded
	for (auto & AssetReferencePath : AssetReferencePaths)
	{ 
		GetAssetUObject(AssetReferencePath);
	}
	
}

void ImportAssetToCurrentPath()
{
	TArray<FAssetData> Selections;
	GEditor->GetContentBrowserSelections(Selections);
	if (Selections.Num() > 0)
	{
		FName CurrentPath = Selections[0].PackagePath;  // import at first selected object
		UE_LOG(LogPromethean, Display, TEXT("\nImport Asset at path: %s"), *CurrentPath.ToString());
		ImportAsset(CurrentPath.ToString());
	}
}

void ImportAsset(FString AssetImportPath)
{
	FAssetToolsModule & AssetToolsModule = FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>("AssetTools");
	AssetToolsModule.Get().ImportAssetsWithDialog(AssetImportPath);
}

void DeleteSelectedAssets()
{
	TArray< FAssetData > AssetViewSelectedAssets;  // = AssetView.Pin()->GetSelectedAssets();
	GEditor->GetContentBrowserSelections(AssetViewSelectedAssets);
	if (AssetViewSelectedAssets.Num() > 0)
	{
		TArray<FAssetData> AssetsToDelete;

		for (auto AssetIt = AssetViewSelectedAssets.CreateConstIterator(); AssetIt; ++AssetIt)
		{
			const FAssetData& AssetData = *AssetIt;
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 1
			if (AssetData.AssetClassPath.GetAssetName().IsEqual(UObjectRedirector::StaticClass()->GetFName()))
#else
			if (AssetData.AssetClass == UObjectRedirector::StaticClass()->GetFName())
#endif
			{
				// Don't operate on Redirectors
				continue;
			}

			AssetsToDelete.Add(AssetData);
		}

		if (AssetsToDelete.Num() > 0)
		{
			ObjectTools::DeleteAssets(AssetsToDelete);
		}
	}
}

/*
	Loads object to target location in Content folder from source path and leave it dirty.
	Any factory derived from UFactory can be used.
	Returns imported object or nullptr when failed.
*/
static UObject* ImportObjectUsingFactory(UFactory& Factory, const FString& AbsoluteSourcePath, const FString& AssetTarget)
{
	// Do nothing if target name contains no destination file name
	FString AssetName = FPaths::GetBaseFilename(AssetTarget);
	if (AssetName.IsEmpty())
	{
		return nullptr;
	}

#if ENGINE_MAJOR_VERSION >= 5 || ENGINE_MAJOR_VERSION >= 4 && ENGINE_MINOR_VERSION >= 27
	UPackage* Package = CreatePackage(*AssetTarget); //Create package if not exist
#else
	UPackage* Package = CreatePackage(nullptr, *AssetTarget); //Create package if not exist
#endif

	bool Canceled = false;
	// To not prompt the user every time asset is overwritten, set the automated asset import data to replace existing.
	UAutomatedAssetImportData* AutomatedAssetImportData = NewObject<UAutomatedAssetImportData>();
	AutomatedAssetImportData->bReplaceExisting = true;
	Factory.SetAutomatedAssetImportData(AutomatedAssetImportData);
	// Import Object. InName has to be the same as package name or filename when saving (I'm not 100% sure).
	auto AssetObject = Factory.ImportObject(Factory.ResolveSupportedClass(), Package, FName(*AssetName), RF_Public | RF_Standalone, AbsoluteSourcePath, nullptr, Canceled);
	if (Canceled == true)
	{
		UE_LOG(LogPromethean, Display, TEXT("Import canceled."));
		return nullptr;
	}
	if (AssetObject == nullptr)
	{
		return nullptr;
	}

	// Generate thumbnail before saving. ThumbnailTools need UObject so we need to find it in the package.
	ThumbnailTools::GenerateThumbnailForObjectToSaveToDisk(AssetObject);

	// Register newly created asset to registry, to make it visible in Content Browser.
	Package->FullyLoad(); // Fully load so the asset may be saved in the future.
	FAssetRegistryModule::AssetCreated(AssetObject);
	Package->SetDirtyFlag(true);
	AssetObject->PostEditChange();
	AssetObject->AddToRoot();
	return AssetObject;

	// TODO (Maciej): Remove the snippet below. I leave it if we would like to save assets immediately after import.
	// Save package with loaded UObject and generated thumbnail to Content folder.
	//return UPackage::SavePackage(Package, nullptr, EObjectFlags::RF_Public | EObjectFlags::RF_Standalone, *(FPaths::ProjectContentDir() + AssetTarget + FPackageName::GetAssetPackageExtension()),
	//								GError, nullptr, false, true, SAVE_NoError);
}



/*
	Takes JsonString and parse it, loading FBX files from all paths contained in the string.
	Returns actual UE4 UObjects created during load process.
	Json string example: {"D:/Chair.FBX" : "/Game/MyMesh/Chair", "D:/Table.FBX" : "/Game/MyMesh/Table"}
*/
TArray<UObject*> ImportFBXAssetsFromJsonString(const FString& JsonString)
{
	TArray<UObject*> LoadedFBX;
	TSharedPtr<FJsonObject> JsonObject;
	if (ParseJsonString(JsonString, JsonObject)) {
		for (auto& JsonItem : JsonObject->Values) {
			FString FBXFileSourcePath = JsonItem.Key;
			FString FBXFileTargetPath = JsonItem.Value->AsString();
			auto Loaded = LoadFBXFile(FBXFileSourcePath, FBXFileTargetPath);
			if (Loaded != nullptr) {
				LoadedFBX.Add(Loaded);
				SaveToAsset(Loaded);  // actually save to a file from memory
			}
		}
	}
	return LoadedFBX;
}


TArray<UObject*> ImportTextureAssetsFromJsonString(const FString& JsonString)
{
	TArray<UObject*> LoadedTextures;
	TSharedPtr<FJsonObject> JsonObject;
	if (ParseJsonString(JsonString, JsonObject)) {
		for (auto& JsonItem : JsonObject->Values) {
			FString FBXFileSourcePath = JsonItem.Key;
			FString FBXFileTargetPath = JsonItem.Value->AsString();
			auto Loaded = LoadTextureFile(FBXFileSourcePath, FBXFileTargetPath);
			if (Loaded != nullptr) {
				LoadedTextures.Add(Loaded);
			}
		}
	}
	return LoadedTextures;
}

/*
	Loads FBX from file, saving it to Content folder.
	AbsoluteSourcePath example: C:/Users/example/models/filename.FBX
	AssetTarget example: MyModels/Subfolder/NewAssetName
*/
UObject* LoadFBXFile(const FString& AbsoluteSourcePath, const FString& AssetTarget) {
	UE_LOG(LogPromethean, Display, TEXT("Promethean Plugin is loading FBX file: %s to %s"), *AbsoluteSourcePath, *AssetTarget);
	// Create FBX factory, which handles FBX importing.
	UFbxFactory* Factory = NewObject<UFbxFactory>(UFbxFactory::StaticClass(), FName("Factory"), RF_NoFlags);

	// Set FBX import settings.
	Factory->bEditorImport = true;
	Factory->ImportUI->StaticMeshImportData->bCombineMeshes = true;
	Factory->ImportUI->bImportMaterials = false;
	Factory->ImportUI->bImportTextures = false;

	return ImportObjectUsingFactory(*Factory, AbsoluteSourcePath, AssetTarget);
}

/*
	Loads Texture from file (any supported extension), saving it to Content folder.
	AbsoluteSourcePath example: C:/Users/example/models/filename.FBX
	AssetTarget example: MyTextures/Subfolder/NewAssetName
*/
UObject* LoadTextureFile(const FString& AbsoluteSourcePath, const FString& AssetTarget)
{
	UE_LOG(LogPromethean, Display, TEXT("Promethean Plugin is loading Texture file: %s to %s"), *AbsoluteSourcePath, *AssetTarget);

	UTextureFactory* Factory;

	// Create Texture factory, which handles all types of files.
	Factory = NewObject<UTextureFactory>(UTextureFactory::StaticClass(), FName("Factory"), RF_NoFlags);

	// Set Texture import settings.
	Factory->bEditorImport = true;
	Factory->bCreateMaterial = false;
	
	// Load and save the object
	return ImportObjectUsingFactory(*Factory, AbsoluteSourcePath, AssetTarget);
}

/*
	Adds object to content browser by duplicating the objects. Returns newly created object.
*/
template<class ObjectType>
UObject* AddObjectToContentBrowser(ObjectType* AssetObject, const FString& TargetPath)
{
	if (AssetObject == nullptr) {
		return nullptr;
	}

#if ENGINE_MAJOR_VERSION >= 5
	UPackage* Package = CreatePackage(*TargetPath); //Create package if not exist
#else
	UPackage* Package = CreatePackage(nullptr, *TargetPath); //Create package if not exist
#endif

	// TODO (Maciej): If we could find a way to update outer package without duplicating that would be nice.
	auto PackageObject = DuplicateObject<ObjectType>(AssetObject, Package, *FPaths::GetBaseFilename(TargetPath));

	// Generate thumbnail before saving.
	ThumbnailTools::GenerateThumbnailForObjectToSaveToDisk(PackageObject);

	// Register newly created asset to registry, to make it visible in Content Browser.
	Package->FullyLoad(); // Fully load so the asset may be saved in the future.
	FAssetRegistryModule::AssetCreated(PackageObject);
	Package->SetDirtyFlag(true);
	PackageObject->PostEditChange();
	PackageObject->AddToRoot();
	return PackageObject;
}

// --------------------------------------------------------------------
// --- Ray Tracing
// --------------------------------------------------------------------

FString RayTraceFromActorsByNameJSON(const TArray<FString>& ActorNames, FVector RayDirection, UWorld* World)
{
	TSharedPtr<FJsonObject> RootObject = MakeShared<FJsonObject>();
	for (auto& ActorName : ActorNames)
	{
		FVector Vector = RayTraceFromActorByName(ActorName, RayDirection, World);
		TArray<TSharedPtr<FJsonValue>> JSONArray;		
		TSharedRef<FJsonValueNumber> X = MakeShareable(new FJsonValueNumber(Vector.X));
		TSharedRef<FJsonValueNumber> Y = MakeShareable(new FJsonValueNumber(Vector.Y));
		TSharedRef<FJsonValueNumber> Z = MakeShareable(new FJsonValueNumber(Vector.Z));
		JSONArray.Add(X);
		JSONArray.Add(Y);
		JSONArray.Add(Z);
		RootObject->SetArrayField(ActorName, JSONArray);
	}
	FString OutputString;
	TSharedRef< TJsonWriter<> > Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);
	return OutputString;
}

FVector RayTraceFromActorByName(FString ActorName, FVector RayDirection, UWorld* World)
{
	AStaticMeshActor* Actor = GetActorByName<AStaticMeshActor>(ActorName, World);
	return RayTraceFromActor(Actor, RayDirection, World);
}

FVector RayTraceFromActor(AActor* Actor, FVector RayDirection, UWorld* World)
{
	TArray<AActor*> IgnoreActors = { Actor }; // don't record collisions with yourself	
	FVector StartPoint = Actor->GetComponentsBoundingBox().GetCenter();
	StartPoint.Z = StartPoint.Z - Actor->GetComponentsBoundingBox().GetSize().Z / 2.0;  // get bottom center location
	FVector EndPoint = StartPoint + RayDirection;
	FHitResult TraceResult = RayTrace(StartPoint, EndPoint, World, IgnoreActors);
	return TraceResult.Location;
}

// global list of hidden actors to make sure raytracing doesn't affect them, refresh on new TCP message
TArray<AActor*> HiddenActors;

FHitResult RayTrace(FVector StartPoint, FVector EndPoint, UWorld* World, TArray<AActor*> IgnoreActors)
{
	bool bHit = false;
	bool bTraceComplex = true;
	FHitResult HitResult;
	FCollisionQueryParams TraceParams(NAME_None, FCollisionQueryParams::GetUnknownStatId(), bTraceComplex, nullptr);
	TraceParams.AddIgnoredActors(IgnoreActors);
	TraceParams.AddIgnoredActors(HiddenActors);
	UE_LOG(LogPromethean, Warning, TEXT("Number of Hidden Actors: %d"), HiddenActors.Num());
	FCollisionObjectQueryParams CollisionParams(FCollisionObjectQueryParams::AllStaticObjects);
	// bHit = World->LineTraceSingleByObjectType(HitResult, StartPoint, EndPoint, CollisionParams, TraceParams);  // wasn't working well with 5.2 and up
	bHit = World->SweepSingleByChannel(HitResult, StartPoint, EndPoint, FQuat::Identity, ECC_WorldStatic, FCollisionShape::MakeSphere(1.f), TraceParams);
	// UE_LOG(LogPromethean, Display, TEXT("Actor hit by raycast %s"), *HitResult.Actor->GetPathName());
	return HitResult;
}

FString RayTraceFromPointsJSON(const TArray<FVector>& Points, FVector RayDirection, UWorld* World, TArray<AActor*> IgnoreActors)
{
	// return the same point of no hit found?
	TSharedPtr<FJsonObject> RootObject = MakeShared<FJsonObject>();
	int i = 0;
	for (auto& StartPoint : Points)
	{
		FVector EndPoint = StartPoint + RayDirection;
		FHitResult TraceResult = RayTrace(StartPoint, EndPoint, World, IgnoreActors);

		FVector Vector = TraceResult.Location;
		TArray<TSharedPtr<FJsonValue>> JSONArray;
		TSharedRef<FJsonValueNumber> X = MakeShareable(new FJsonValueNumber(Vector.X));
		TSharedRef<FJsonValueNumber> Y = MakeShareable(new FJsonValueNumber(Vector.Y));
		TSharedRef<FJsonValueNumber> Z = MakeShareable(new FJsonValueNumber(Vector.Z));
		JSONArray.Add(X);
		JSONArray.Add(Y);
		JSONArray.Add(Z);
		
		RootObject->SetArrayField(FString::FromInt(i), JSONArray);
		i += 1;
	}
	FString OutputString;
	TSharedRef< TJsonWriter<> > Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);
	return OutputString;
}


FVector RayTraceFromCamera(UWorld* World)
{
	// return the position in front of the camera
	TArray<AActor*> IgnoreActors;
	FEditorViewportClient* client = (FEditorViewportClient*)GEditor->GetActiveViewport()->GetClient();
	FVector editorCameraPosition = client->GetViewLocation();
	FVector editorCameraDirection = client->GetViewRotation().Vector();
	FVector endPoint = editorCameraPosition + editorCameraDirection * 1500;
	FHitResult TraceResult = RayTrace(editorCameraPosition, endPoint, World, IgnoreActors);
	return TraceResult.Location;
}

// MULTI RAY TRACE

FString RayTraceMultiFromActorsByNameJSON(const TArray<FString>& ActorNames, FVector RayDirection, UWorld* World)
{
	TArray<AActor*> Actors;
	for (auto& ActorName : ActorNames)	
		Actors.Add(GetActorByName<AStaticMeshActor>(ActorName, World));  // get all actors to use as ignore actors to avoid collision between them

	TSharedPtr<FJsonObject> RootObject = MakeShared<FJsonObject>();
	for (auto& Actor : Actors)
	{
		FVector Vector = RayTraceMultiFromActor(Actor, RayDirection, World, Actors);
		TArray<TSharedPtr<FJsonValue>> JSONArray;
		TSharedRef<FJsonValueNumber> X = MakeShareable(new FJsonValueNumber(Vector.X));
		TSharedRef<FJsonValueNumber> Y = MakeShareable(new FJsonValueNumber(Vector.Y));
		TSharedRef<FJsonValueNumber> Z = MakeShareable(new FJsonValueNumber(Vector.Z));
		JSONArray.Add(X);
		JSONArray.Add(Y);
		JSONArray.Add(Z);
		RootObject->SetArrayField(Actor->GetName().ToLower(), JSONArray);
	}
	FString OutputString;
	TSharedRef< TJsonWriter<> > Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);
	return OutputString;
}

FVector RayTraceMultiFromActorByName(FString ActorName, FVector RayDirection, UWorld* World)
{
	AStaticMeshActor* Actor = GetActorByName<AStaticMeshActor>(ActorName, World);
	return RayTraceMultiFromActor(Actor, RayDirection, World);
}

FVector RayTraceMultiFromActor(AActor* Actor, FVector RayDirection, UWorld* World, TArray<AActor*> IgnoreActors)
{
	IgnoreActors.AddUnique(Actor); // only adds if actor is not present already. don't record collisions with yourself
	FVector HalfDirectionRay = RayDirection * 0.5;
	FVector ActorPivot = Actor->GetComponentsBoundingBox().GetCenter();
	ActorPivot.Z = ActorPivot.Z - Actor->GetComponentsBoundingBox().GetSize().Z / 2.0;  // get bottom center location
	FVector StartPoint = ActorPivot - HalfDirectionRay;  // multi ray is by default ray direction based (no backfaces), so instead of shooting 2 rays one up and one down...
	FVector EndPoint = ActorPivot + HalfDirectionRay;  //... we shoot one from higher up and find closest to original 
	TArray<FHitResult> TraceResults = RayTraceMulti(StartPoint, EndPoint, World, IgnoreActors);
	if (TraceResults.Num() > 0)
	{
		float CurrentBestDisntace = std::numeric_limits<float>::infinity();
		FVector CurrentBestLocation;
		for (auto& TraceResult : TraceResults)
		{
			float CurrentDistance = FVector::Distance(TraceResult.Location, ActorPivot);
			if (CurrentDistance < CurrentBestDisntace)
			{
				CurrentBestLocation = TraceResult.Location;
				CurrentBestDisntace = CurrentDistance;
			}
		}
		return CurrentBestLocation;
	}
	return FVector(0,0,0);
}

TArray<FHitResult> RayTraceMulti(FVector StartPoint, FVector EndPoint, UWorld* World, TArray<AActor*> IgnoreActors)
{
	bool bHit = false;
	bool bTraceComplex = true;
	TArray<FHitResult> HitResults;
	FCollisionQueryParams TraceParams(NAME_None, FCollisionQueryParams::GetUnknownStatId(), bTraceComplex, nullptr);
	TraceParams.AddIgnoredActors(IgnoreActors);
	FCollisionObjectQueryParams CollisionParams(FCollisionObjectQueryParams::AllStaticObjects);
	bHit = World->LineTraceMultiByObjectType(HitResults, StartPoint, EndPoint, CollisionParams, TraceParams);
	/*  debug logging
	UE_LOG(LogPromethean, Display, TEXT("Hits by raycast %i"), HitResults.Num());	
	for (auto& HitResult : HitResults)
	{
		UE_LOG(LogPromethean, Display, TEXT("Hit location %s"), *HitResult.Location.ToString());
		UE_LOG(LogPromethean, Display, TEXT("Hit normal %s"), *HitResult.Normal.ToString());		
	}
	*/
	return HitResults;
}


// --------------------------------------------------------------------
// --- Mesh Functions
// --------------------------------------------------------------------
TArray<FVector> GetMeshVertexPositions(AStaticMeshActor* StaticMeshActor)
{
	TArray<FVector> OutArray;
	// if (!IsValidLowLevel()) return;
	UStaticMeshComponent* StaticMeshComponent = StaticMeshActor->GetStaticMeshComponent();	
	if (!StaticMeshComponent) return OutArray;
	if (!StaticMeshComponent->GetStaticMesh()) return OutArray;
	if (!StaticMeshComponent->GetStaticMesh()->GetRenderData()) return OutArray;
	if (StaticMeshComponent->GetStaticMesh()->GetRenderData()->LODResources.Num() > 0)
	{
		FPositionVertexBuffer* VertexBuffer = &StaticMeshComponent->GetStaticMesh()->GetRenderData()->LODResources[0].VertexBuffers.PositionVertexBuffer;
		if (VertexBuffer)
		{
			const int32 VertexCount = VertexBuffer->GetNumVertices();
			for (int32 Index = 0; Index < VertexCount; Index++)
			{
				//This is in the Static Mesh Actor Class, so it is location and tranform of the SMActor
				const FVector WorldSpaceVertexLocation = StaticMeshActor->GetActorLocation() + StaticMeshActor->GetTransform().TransformVector(FVector(VertexBuffer->VertexPosition(Index)));
				OutArray.Add(WorldSpaceVertexLocation);
			}
		}
	}
	return OutArray;
}


TArray<FVector> GetMeshTrianglePositionsFromActor(AActor* MeshActor)
{
	TArray<FVector> Vertexes;
	// supports both static meshes and generated meshes
	if (MeshActor == nullptr)
	{
		return Vertexes;  // return nullptr
	}	
	auto GeneratedMeshActor = Cast<APrometheanMeshActor>(MeshActor);
	if (GeneratedMeshActor != nullptr)
	{
		Vertexes = GetMeshTrianglePositionsFromGeneratedMesh(GeneratedMeshActor);
	}
	else {
		auto ValidMeshComponent = GetValidStaticMeshComponent(MeshActor);
		Vertexes = GetMeshTrianglePositionsFromStaticMesh(ValidMeshComponent);
	}
	return Vertexes;
}


TArray<FVector> GetMeshTrianglePositionsFromStaticMesh(UStaticMeshComponent* StaticMeshComponent)
{	
	// return an array of vertices IN WORLD SPACE from SCENE OBJECT where every three verts define a triangle in order of their ids
	TArray<FVector> OutArray;

	if (StaticMeshComponent == nullptr) {
		return OutArray;
	}

	if (!StaticMeshComponent) return OutArray;
	if (!StaticMeshComponent->GetStaticMesh()) return OutArray;
	if (!StaticMeshComponent->GetStaticMesh()->GetRenderData()) return OutArray;
	if (!(StaticMeshComponent->GetStaticMesh()->GetRenderData()->LODResources.Num() > 0)) return OutArray;
	
	const FStaticMeshLODResources& LODModel = StaticMeshComponent->GetStaticMesh()->GetRenderData()->LODResources[0];
	FIndexArrayView Indices = LODModel.IndexBuffer.GetArrayView();
	const FPositionVertexBuffer& PositionVertexBuffer = LODModel.VertexBuffers.PositionVertexBuffer;
	int32 SectionFirstTriIndex = 0;
	// TODO: FIX for 4.25!
	//for (TArray<FStaticMeshSection>::TConstIterator SectionIt(LODModel.Sections); SectionIt; ++SectionIt)	  // - 4.24 and below
	for (auto& SectionIt : LODModel.Sections)
	{
		const FStaticMeshSection& Section = SectionIt;  // *SectionIt;  // - 4.24 and below
		int32 NextSectionTriIndex = SectionFirstTriIndex + Section.NumTriangles;
		for (uint32 TriIndex = 0; TriIndex < Section.NumTriangles; TriIndex++) // if (InTriangleIndex >= SectionFirstTriIndex && InTriangleIndex < NextSectionTriIndex)		
		{
			// Get vertex index for Tri index
			int32 IndexBufferIdx = TriIndex * 3 + Section.FirstIndex;

			// Look up the triangle vertex indices
			int32 Index0 = Indices[IndexBufferIdx];
			int32 Index1 = Indices[IndexBufferIdx + 1];
			int32 Index2 = Indices[IndexBufferIdx + 2];

			// Lookup the triangle world positions and colors.
			FVector WorldVert0 = StaticMeshComponent->GetComponentTransform().TransformPosition(FVector(PositionVertexBuffer.VertexPosition(Index0)));
			FVector WorldVert1 = StaticMeshComponent->GetComponentTransform().TransformPosition(FVector(PositionVertexBuffer.VertexPosition(Index1)));
			FVector WorldVert2 = StaticMeshComponent->GetComponentTransform().TransformPosition(FVector(PositionVertexBuffer.VertexPosition(Index2)));
			// Write the data to output array
			OutArray.Add(WorldVert0);
			OutArray.Add(WorldVert1);
			OutArray.Add(WorldVert2);
		}
		SectionFirstTriIndex += Section.NumTriangles;
	}
	return OutArray;
}


TArray<FVector> GetMeshTrianglePositionsFromGeneratedMesh(APrometheanMeshActor* PrometheanMeshActor)
{
	TArray<FVector> OutArray;

	if (PrometheanMeshActor == nullptr) {
		return OutArray;
	}

	for (auto TriangleID : PrometheanMeshActor->GeneratedMeshParams.TriangleIDs)
	{
		OutArray.Add(PrometheanMeshActor->GeneratedMesh->GetComponentTransform().TransformPosition(PrometheanMeshActor->GeneratedMeshParams.VertLocations[TriangleID]));
	}

	return OutArray;
}


TArray<FVector> GetMeshTrianglePositionsFromLibraryStaticMesh(UStaticMesh* StaticMesh)
{
	// return an array of vertices IN WORLD SPACE from SCENE OBJECT where every three verts define a triangle in order of their ids
	TArray<FVector> OutArray;

	if (StaticMesh == nullptr) {
		return OutArray;
	}

	if (!StaticMesh->GetRenderData()) return OutArray;
	if (!(StaticMesh->GetRenderData()->LODResources.Num() > 0)) return OutArray;

	const FStaticMeshLODResources& LODModel = StaticMesh->GetRenderData()->LODResources[0];
	FIndexArrayView Indices = LODModel.IndexBuffer.GetArrayView();
	const FPositionVertexBuffer& PositionVertexBuffer = LODModel.VertexBuffers.PositionVertexBuffer;
	int32 SectionFirstTriIndex = 0;
	// TODO: FIX for 4.25!
	//for (TArray<FStaticMeshSection>::TConstIterator SectionIt(LODModel.Sections); SectionIt; ++SectionIt)	  // - 4.24 and below
	for (auto& SectionIt : LODModel.Sections)
	{
		const FStaticMeshSection& Section = SectionIt;  // *SectionIt;  // - 4.24 and below
		int32 NextSectionTriIndex = SectionFirstTriIndex + Section.NumTriangles;
		for (uint32 TriIndex = 0; TriIndex < Section.NumTriangles; TriIndex++) // if (InTriangleIndex >= SectionFirstTriIndex && InTriangleIndex < NextSectionTriIndex)		
		{
			// Get vertex index for Tri index
			int32 IndexBufferIdx = TriIndex * 3 + Section.FirstIndex;

			// Look up the triangle vertex indices
			int32 Index0 = Indices[IndexBufferIdx];
			int32 Index1 = Indices[IndexBufferIdx + 1];
			int32 Index2 = Indices[IndexBufferIdx + 2];

			// Lookup the triangle world positions and colors.
			FVector WorldVert0 = FVector(PositionVertexBuffer.VertexPosition(Index0));
			FVector WorldVert1 = FVector(PositionVertexBuffer.VertexPosition(Index1));
			FVector WorldVert2 = FVector(PositionVertexBuffer.VertexPosition(Index2));
			// Write the data to output array
			OutArray.Add(WorldVert0);
			OutArray.Add(WorldVert1);
			OutArray.Add(WorldVert2);
		}
		SectionFirstTriIndex += Section.NumTriangles;
	}
	return OutArray;
}

// --------------------------------------------------------------------
// --- Misc Functions
// --------------------------------------------------------------------

TArray<FVector> getSelectionPositions(UWorld* World)
{
	TArray<FVector> OutArray;
	USelection* SelectedActors = GEditor->GetSelectedActors();
	for (FSelectionIterator Iter(*SelectedActors); Iter; ++Iter)
	{
		AActor* Actor = Cast<AActor>(*Iter);  // add only on AActor location, other types might not have a location
		if (Actor)
		{
			FVector Location = Actor->GetActorLocation();
			OutArray.Add(Location);
		}
	}
	return OutArray;
}

TArray<TSharedPtr<FJsonValue>> FVectorToJsonArray(FVector Vector)
{
	TArray<TSharedPtr<FJsonValue>> JSONNumArray;	
	JSONNumArray.Add(MakeShareable(new FJsonValueNumber(Vector.X)));
	JSONNumArray.Add(MakeShareable(new FJsonValueNumber(Vector.Y)));
	JSONNumArray.Add(MakeShareable(new FJsonValueNumber(Vector.Z)));
	return JSONNumArray;
}

TArray<TSharedPtr<FJsonValue>> FStringArrayToJsonArray(TArray<FString> StringArray)
{
	TArray<TSharedPtr<FJsonValue>> JSONStringArray;
	for (FString String : StringArray)
	{
		JSONStringArray.Add(MakeShareable(new FJsonValueString(String)));
	}
	return JSONStringArray;
}


FVector RoundVectorComponents(const FVector& Vector)
{	
	// round up to 2 points after the decimal to significantly save on tcp json traffic
	float multiplier = 100;  // FMath::Pow(10, DecimalPlaces);

	FVector RoundedVector;
	RoundedVector.X = FMath::RoundToInt(Vector.X * multiplier) / multiplier;
	RoundedVector.Y = FMath::RoundToInt(Vector.Y * multiplier) / multiplier;
	RoundedVector.Z = FMath::RoundToInt(Vector.Z * multiplier) / multiplier;

	return RoundedVector;
}

TArray<TSharedPtr<FJsonValue>> FVectorArrayToJsonArray(const TArray<FVector>& VectorArray)
{
	// add an array of vector
	TArray<TSharedPtr<FJsonValue>> JSONDictArray;  // for output

	TSharedPtr<FJsonObject> JSONDict = MakeShared<FJsonObject>();
	TSharedRef<FJsonValueObject> JSONDictValue = MakeShared<FJsonValueObject>(JSONDict);
	int32 i = 0;
	for (FVector Vector : VectorArray)
	{
		i += 1;
		// Round the FVector components to desired decimal precision
		FVector RoundedVector = RoundVectorComponents(Vector);

		TArray<TSharedPtr<FJsonValue>> MeshScaleArray = FVectorToJsonArray(RoundedVector);
		// UE_LOG(LogPromethean, Display, TEXT("Vector Position: %s"), *Vector.ToCompactString());  // gets material slots on the static mesh						
		// TODO just serialize a regular list
		JSONDict->SetArrayField(FString::FromInt(i), MeshScaleArray);  // TODO: pass vertex id?		
	}
	JSONDictArray.Add(JSONDictValue);	
	return JSONDictArray;
}

FString FVectorArrayToJsonString(const TArray<FVector>& VectorArray, FString Label)
{	
	TArray<TSharedPtr<FJsonValue>> JSONDictArray = FVectorArrayToJsonArray(VectorArray);  // for output		
	// output json data to string
	TSharedPtr<FJsonObject> RootObject = MakeShared<FJsonObject>();	
	RootObject->SetArrayField(*Label, JSONDictArray);
	FString OutputString;
	TSharedRef< TJsonWriter<> > Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);
	return OutputString;
}

TSharedPtr<FJsonObject> FStringArrayToJsonIndexDict(const TArray<FString>& StringArray)
{
	TArray<FString> Used;

	// add an array of vector
	TSharedPtr<FJsonObject> JSONDict = MakeShared<FJsonObject>();
	int32 index;
	for (FString String : StringArray)
	{
		if (Used.Contains(String))
		{
			continue;
		}
		else
		{
			Used.Add(String);
		}

		index = 0;  // reset index
		TArray<TSharedPtr<FJsonValue>> IndexArray;
		for (FString CountString : StringArray)		
		{

			if (String == CountString)
			{
				IndexArray.Add(MakeShareable(new FJsonValueNumber(index)));
			}
			index += 1;
		}
		JSONDict->SetArrayField(String, IndexArray);
	}
	return JSONDict;
}

void CaptureScreenshot(FString OutputPath)
{
	UE_LOG(LogPromethean, Display, TEXT("Capture screenshot at %s"), *OutputPath);
	FScreenshotRequest::RequestScreenshot(OutputPath, false, false);
	GEditor->RedrawAllViewports(false);  // invalidate hit proxies - false
	// GEditor->TakeHighResScreenShots();
}

void SaveToFile(FString OutputPath, FString TextToSave)
{	
	int Index;
	OutputPath.FindLastChar('/', Index);		
	FString OutputFolder = OutputPath.Left(Index) + "/";  // need to check if the folder exists

	UE_LOG(LogPromethean, Display, TEXT("Saving file: %s"), *OutputFolder);
	bool AllowOverwriting = true;

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	// CreateDirectoryTree returns true if the destination directory existed prior to call
	// or has been created during the call.
	if (PlatformFile.CreateDirectoryTree(*OutputFolder))
	{
		// Allow overwriting or file doesn't already exist
		if (AllowOverwriting || !FPaths::FileExists(*OutputPath))
		{
			FFileHelper::SaveStringToFile(TextToSave, *OutputPath);
			UE_LOG(LogPromethean, Display, TEXT("File saved to: %s"), *OutputPath);
		}
	}
}

FTransform StringArrayToTransform(const TArray<FString>& StringArray)
{
	if (StringArray.Num() > 8)
	{
		FVector Translation = FVector(FCString::Atof(*StringArray[0]), FCString::Atof(*StringArray[1]), FCString::Atof(*StringArray[2]));
		FRotator Rotation = FRotatorFromXYZVec(FVector(FCString::Atof(*StringArray[3]), FCString::Atof(*StringArray[4]), FCString::Atof(*StringArray[5])));
		FVector Scale = FVector(FCString::Atof(*StringArray[6]), FCString::Atof(*StringArray[7]), FCString::Atof(*StringArray[8]));
		return FTransform(Rotation, Translation, Scale);
	}
	FTransform DummyTransform = FTransform();
	DummyTransform.SetLocation(FVector(NegativeInfinity, NegativeInfinity, NegativeInfinity));  // this will fail NaN test in functions that use this
	return DummyTransform;
}

FLinearColor StringToLinearColor(FString String)
{
	TArray<FString> StringArray;
	String.ParseIntoArray(StringArray, TEXT(","), true);  // text values are separated by comma

	FLinearColor Vector;
	if (StringArray.Num() > 3)
	{
		float r = FCString::Atof(*StringArray[0]);
		float g = FCString::Atof(*StringArray[1]);
		float b = FCString::Atof(*StringArray[2]);
		float a = FCString::Atof(*StringArray[3]);		
		return FLinearColor(r, g, b, a);
	}
	return Vector;
}

FString LinearColorArrayToString(const TArray<FLinearColor>& ColorArray)
{
	FString OutString = "";
	for (auto& LinearColor : ColorArray)
	{		
		OutString += FString::SanitizeFloat(LinearColor.R) + ",";
		OutString += FString::SanitizeFloat(LinearColor.G) + ",";
		OutString += FString::SanitizeFloat(LinearColor.B) + ",";
		OutString += FString::SanitizeFloat(LinearColor.A) + " ";
	}
	OutString.RemoveFromEnd(" ");
	return OutString.ToLower();
}

FVector StringToVector(FString String)
{
	TArray<FString> StringArray;
	String.ParseIntoArray(StringArray, TEXT(","), true);  // text values are separated by comma
	return StringArrayToVector(StringArray);
}

FVector StringArrayToVector(const TArray<FString>& StringArray)
{	
	if (StringArray.Num() > 2)
	{
		return FVector(FCString::Atof(*StringArray[0]), FCString::Atof(*StringArray[1]), FCString::Atof(*StringArray[2]));
	}	
	return NegativeInfinityVector;
}

FString StaticMeshActorArrayToNamesString(const TArray<AStaticMeshActor*>& Actors)
{
	FString NameString = "";
	for (auto& Actor : Actors)
	{		
		NameString = NameString + Actor->GetName() + ",";
	}
	NameString.RemoveFromEnd(",");
	return NameString.ToLower();
}

FString ActorArrayToAssetPathsString(const TArray<AActor*>& Actors)
{
	return StringArrayToString(ActorArrayToAssetPathsArray(Actors));  // .ToLower()  - don't lower_case because that can affect hash consistency for DB lookup
}


TArray<FString> ActorArrayToAssetPathsArray(const TArray<AActor*>& Actors)
{
	TArray<FString> PathArray;
	for (auto& Actor : Actors)
	{
	    FString AssetPath = TEXT("none"); // Initialize with default value
        if (Actor != nullptr)
        {
        // check if Blueprint
            UClass* ActorClass = Actor->GetClass();
            if (ActorClass != nullptr)
            {
                UBlueprintGeneratedClass* BPGC = Cast<UBlueprintGeneratedClass>(ActorClass);
                if (BPGC != nullptr && BPGC->ClassGeneratedBy != nullptr)
                {
                    UObject* Asset = BPGC->ClassGeneratedBy;
                    AssetPath = Asset->GetPathName(); // Update the path if conditions are met
                }
            }
        }
        // check if static mesh
        if (AssetPath == TEXT("none"))
        {
            UStaticMeshComponent* StaticMeshComponent = Cast<UStaticMeshComponent>(GetValidSceneComponent(Actor));
            if (StaticMeshComponent != nullptr)
            {
                UStaticMesh* StaticMesh = StaticMeshComponent->GetStaticMesh();
				AssetPath = StaticMesh->GetPathName();  // .ToLower()  - don't lower_case because that can affect hash consistency for DB lookup
            }
        }
        // Add the asset path to the array
        PathArray.Add(AssetPath);
	}
	return PathArray;
}


FString StaticMeshActorArrayToAssetPathsString(const TArray<AStaticMeshActor*>& Actors)
{
	FString NameString = "";
	for (auto& Actor : Actors)
	{
		TArray<UStaticMeshComponent*> ActorStaticMeshComponents;
		Actor->GetComponents<UStaticMeshComponent>(ActorStaticMeshComponents);
		for (UStaticMeshComponent* StaticMeshComponent : ActorStaticMeshComponents)
		{
			UStaticMesh* StaticMesh = StaticMeshComponent->GetStaticMesh();
			NameString = NameString + StaticMesh->GetPathName() + ",";  // .ToLower()  - don't lower_case because that can affect hash consistency for DB lookup
		}		
	}
	NameString.RemoveFromEnd(",");
	return NameString;  // .ToLower()  - don't lower_case because that can affect hash consistency for DB lookup
}

FString ActorArrayToNamesString(const TArray<AActor*>& Actors)
{
	return StringArrayToString(ActorArrayToNamesArray(Actors));
}


TArray<FString> ActorArrayToNamesArray(const TArray<AActor*>& Actors)
{
	TArray<FString> NameArray;
	for (auto& Actor : Actors)
	{
		NameArray.Add(Actor->GetName().ToLower());  // ATTENTION: making lowercase here
	}
	return NameArray;
}


TArray<FVector> ActorArrayToLocationsArray(TArray<AActor*> Actors)
{
	TArray<FVector> OutArray;
	for (auto& Actor : Actors)
	{
		OutArray.Add(Actor->GetActorLocation());
	}
	return OutArray;
}


TArray<FVector> ActorArrayToPivotArray(TArray<AActor*> Actors)
{
	TArray<FVector> OutArray;
	for (auto& Actor : Actors)
	{
		FVector MeshPivotVec = Actor->GetComponentsBoundingBox().GetCenter();
		MeshPivotVec.Z = MeshPivotVec.Z - Actor->GetComponentsBoundingBox().GetSize().Z / 2.0;
		OutArray.Add(MeshPivotVec);
	}
	return OutArray;
}



FString StringArrayToString(const TArray<FString>& Strings)
{	// comma-separated 
	FString NameString = "";
	for (auto& String : Strings)	
		NameString += String + ",";
	NameString.RemoveFromEnd(",");
	return NameString;
}

FString FloatArrayToString(const TArray<float>& Floats)
{
	FString OutString = "";
	for (auto& Float : Floats)
	{
		OutString += FString::SanitizeFloat(Float) + ",";
	}
	OutString.RemoveFromEnd(",");
	return OutString.ToLower();
}

FString FVectorToString(FVector Vector)
{
	return FString::SanitizeFloat(Vector.X) + "," + FString::SanitizeFloat(Vector.Y) + "," + FString::SanitizeFloat(Vector.Z);
}

FRotator FRotatorFromXYZVec(FVector RotationVec)
{
	return FRotator(RotationVec[1], RotationVec[2], RotationVec[0]);
}

/*
	Returns true if supplies result with correctly deserialized json object, parsed from parameter string.
*/
bool ParseJsonString(const FString& String, TSharedPtr<FJsonObject>& Result)
{
	TSharedRef<FJsonStringReader> JsonReader = FJsonStringReader::Create(String);
	if (!FJsonSerializer::Deserialize((TSharedRef<TJsonReader<TCHAR>>)JsonReader, Result))
	{
		UE_LOG(LogPromethean, Display, TEXT("Promethean TCP command json parsing error: %s"), *JsonReader->GetErrorMessage());
		return false;
	}
	return true;
}

USceneComponent* GetBiggestVolumeBoundComponent(const TArray<UStaticMeshComponent*>& Components)
{
	if (Components.Num() <= 0)
	{
		return nullptr;
	}

	USceneComponent* TheBiggest = Components[0];
	for (int32 i = 1; i < Components.Num(); ++i)
	{
		if (Components[i] != nullptr && Components[i]->Bounds.BoxExtent.Size() > TheBiggest->Bounds.BoxExtent.Size())
		{
			TheBiggest = Components[i];
		}
	}
	return TheBiggest;
}




