// Fill out your copyright notice in the Description page of Project Settings.

#include "PrometheanLearning.h"  // matching header has to go first
#include "PrometheanGeneric.h"
#include "PrometheanMesh.h"
#include "promethean.h" // has stuff necessary for custom logging

#include "UObject/Object.h"
#include "EngineUtils.h"
#include "Dom/JsonObject.h"
#include "Engine/StaticMesh.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"


// --------------------------------------------------------------------
// --- LEARNING FUNCTIONS
// --------------------------------------------------------------------

void PrometheanLearn(UWorld* World, FString OutputPath)
{
	TArray<AActor*> AllValidActors = GetWorldValidActors(World);
	FString SceneName = World->GetName();
	GenerateLearnData(AllValidActors, SceneName, OutputPath);
}

void PrometheanLearnSelection(FString SceneName, FString OutputPath)
{
	TArray<AActor*> SelectedValidActors = GetSelectedValidActors();
	GenerateLearnData(SelectedValidActors, SceneName, OutputPath);
}

void PrometheanLearnView(FString SceneName, FString OutputPath)
{
	TArray<AActor*> RenderedValidActors = GetRenderedValidActors();
	GenerateLearnData(RenderedValidActors, SceneName, OutputPath);
}

void GenerateLearnData(TArray<AActor*> ValidActors, FString SceneName, FString OutputPath)
{
	UE_LOG(LogPromethean, Display, TEXT("Generating Learn Data..."));	
	FString TextToSave = GenerateLearnFileString(ValidActors, SceneName);
	SaveToFile(OutputPath, TextToSave);
}

FString GenerateLearnFileString(TArray<AActor*> LearnActors, FString SceneName)
{	

	TSharedPtr<FJsonObject> RootObject = MakeShared<FJsonObject>();
	TArray<TSharedPtr<FJsonValue>> RawDataArray;
	UWorld* EditorWorld = GetEditorWorld(); //  TODO: can pass this on from the Command Switch later to get a tiny speedup
	// - get actor data
	for (AActor* CurrentLearnActor : LearnActors)
	{
		USceneComponent* SceneComponent = GetValidSceneComponent(CurrentLearnActor);
		if (SceneComponent == nullptr)
		{
			// skip if actor has no valid static mesh
			continue;
		}

		TSharedPtr<FJsonObject> MeshObject = MakeShared<FJsonObject>();
		TSharedRef<FJsonValueObject> MeshValue = MakeShared<FJsonValueObject>(MeshObject);

		auto StaticMeshComponent = Cast<UStaticMeshComponent>(SceneComponent);
		auto ProceduralMeshComponent = Cast<UProceduralMeshComponent>(SceneComponent);
		// if CurrentLearnActor is StaticMeshComponent, get asset path;
		if (StaticMeshComponent != nullptr) {
			UStaticMesh* StaticMesh = StaticMeshComponent->GetStaticMesh();
			
			// --- early outs
			// --- skip empty static meshes which are most likely there to signify semantic objects
			if (StaticMesh->GetPathName() == FString("None"))
			{
				// continue;
				MeshObject->SetBoolField(TEXT("is_group"), true);
			}

			// --- art asset path
			// - Ignore VR actors that get added to the scene for VR editing
			if (StaticMesh->GetPathName().StartsWith("/Engine/VREditor/"))
				continue;
			MeshObject->SetStringField(TEXT("art_asset_path"), StaticMesh->GetPathName());
		}
		else if(ProceduralMeshComponent != nullptr) {
			// else, CurrentLearnActor is probably a GeneratedMesh. Additional if statement secure the code if someone adds more types in GetValidLearningSceneComponent();
			MeshObject->SetStringField(TEXT("art_asset_path"), "generated_mesh");
		}

		// --- raw_name: There is no maya path name in UE. 
		// All meshes inside an fbx are considered a single mesh
		MeshObject->SetStringField(TEXT("raw_name"), CurrentLearnActor->GetName().ToLower());  // lowercase name

		// --- parent name
		FString ParentName = GetParentForObjectByName(CurrentLearnActor->GetName(), EditorWorld);
		MeshObject->SetStringField(TEXT("parent_name"), ParentName.ToLower());  // lowercase name

		// --- rotation
		FRotator ActorRotation = CurrentLearnActor->GetActorRotation();  // reused later to reset rotation back after getting local space bounds
		TArray<TSharedPtr<FJsonValue>> MeshRotationArray = GetRotationJSONData(ActorRotation);
		MeshObject->SetArrayField(TEXT("rotation"), MeshRotationArray);

		// --- scale
		FVector Scale = CurrentLearnActor->GetActorScale3D();
		TArray<TSharedPtr<FJsonValue>> MeshScaleArray = GetVectorJSONData(Scale); // reused for transform string
		MeshObject->SetArrayField(TEXT("scale"), MeshScaleArray);

		// --- pivot
		// Get bottom-center of local bbox. Local Bounding Box is the bbox of that object with identity transform matrix
		FBox LocalBBox = CurrentLearnActor->CalculateComponentsBoundingBoxInLocalSpace(true, false);  // (bool bNonColliding, bool bIncludeFromChildActors)
		FVector MeshPivotVec = LocalBBox.GetCenter();
		MeshPivotVec.Z = MeshPivotVec.Z - LocalBBox.GetExtent().Z;
		// Get WS position of the pivot
		MeshPivotVec = ActorRotation.RotateVector(MeshPivotVec) * Scale + CurrentLearnActor->GetActorLocation();
		MeshObject->SetArrayField(TEXT("pivot"), GetVectorJSONData(MeshPivotVec));

		// --- size
		FVector MeshSize = LocalBBox.GetSize() * Scale;  // - this would be world space, if wasn't for rotation
		// UE_LOG(LogPromethean, Display, TEXT("Local bbox: %s"), *LocalBBox.ToString());
		// UE_LOG(LogPromethean, Display, TEXT("Local bbox size: %s"), *LocalBBox.GetSize().ToString());
		// UE_LOG(LogPromethean, Display, TEXT("Scale: %s"), *Scale.ToString());
		MeshObject->SetArrayField(TEXT("size"), GetVectorJSONData(MeshSize));

		// --- transform
		TArray<TSharedPtr<FJsonValue>> MeshTransformArray = GetVectorJSONData(CurrentLearnActor->GetActorLocation());
		MeshTransformArray.Append(MeshRotationArray);
		MeshTransformArray.Append(MeshScaleArray);
		MeshObject->SetArrayField(TEXT("transform"), MeshTransformArray);


		// -- Add mesh
		RawDataArray.Add(MeshValue);

	}
	RootObject->SetStringField(TEXT("scene_id"), SceneName);
	RootObject->SetArrayField(TEXT("raw_data"), RawDataArray);

	FString OutputString;
	TSharedRef< TJsonWriter<> > Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);

	return OutputString;
}



TArray<TSharedPtr<FJsonValue>> GetVectorJSONData(FVector UEVector)
{
	// -- Centralized function that controls the coordinate system for both learn and update transform commands
	TArray<TSharedPtr<FJsonValue>> UEVectorArray;
	TSharedRef<FJsonValueNumber> VectorX = MakeShareable(new FJsonValueNumber(UEVector.X));
	TSharedRef<FJsonValueNumber> VectorY = MakeShareable(new FJsonValueNumber(UEVector.Y));
	TSharedRef<FJsonValueNumber> VectorZ = MakeShareable(new FJsonValueNumber(UEVector.Z));
	UEVectorArray.Add(VectorX);
	UEVectorArray.Add(VectorY);
	UEVectorArray.Add(VectorZ);
	return UEVectorArray;
}


TArray<TSharedPtr<FJsonValue>> GetRotationJSONData(FRotator ActorRotation)
{
	// -- Centralized function that controls the coordinate system for both learn and update transform commands
	TArray<TSharedPtr<FJsonValue>> RototationArray;
	TSharedRef<FJsonValueNumber> RotationX = MakeShareable(new FJsonValueNumber(ActorRotation.Roll));
	TSharedRef<FJsonValueNumber> RotationY = MakeShareable(new FJsonValueNumber(ActorRotation.Pitch));
	TSharedRef<FJsonValueNumber> RotationZ = MakeShareable(new FJsonValueNumber(ActorRotation.Yaw));
	RototationArray.Add(RotationX);
	RototationArray.Add(RotationY);
	RototationArray.Add(RotationZ);
	return RototationArray;
}
