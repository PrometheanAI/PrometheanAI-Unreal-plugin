#pragma once

#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "PrometheanMesh.generated.h"

USTRUCT(BlueprintType)
struct FPrometheanMeshParameters {
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category="Promethean")
		TArray<FVector> VertLocations;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Promethean")
		TArray<int32> TriangleIDs; // Index buffer indicating which vertices make up each triangle, must be a length of Vertices array length multiplied by 3.

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Promethean")
		TArray<FVector> Normals; // must be same length as Vertices array.

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Promethean")
		TArray<FProcMeshTangent> Tangents; // must be same length as Vertices array.

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Promethean")
		TArray<FVector2D> UVs; // must be same length as Vertices array.

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Promethean")
		TArray<FLinearColor> VertexColors; // must be same length as Vertices array.

	FPrometheanMeshParameters() = default;
};


UCLASS()
class PROMETHEAN_API APrometheanMeshActor : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	APrometheanMeshActor();

public:
	UFUNCTION()
		static FString GenerateMeshes(const FString& JsonInputString);

public:
	UFUNCTION(BlueprintCallable, Category = "Promethean|Mesh")
		void RegeneratePrometheanMesh();

	UFUNCTION(BlueprintCallable, Category = "Promethean|Mesh")
		bool TryToSetMaterial(const FString& MaterialPath, const int MaterialIndex);

	/*  // TODO 4.24 replaced FMeshDescription for FStaticMeshAttributes - need to fix!
	UFUNCTION(BlueprintCallable, Category = "Promethean|Mesh")
		void ConvertGeneratedMeshToStaticMesh();
	*/

	UPROPERTY(BlueprintReadOnly, Category = "Promethean|Mesh")
		FPrometheanMeshParameters GeneratedMeshParams;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="Promethean|Mesh")
		class UProceduralMeshComponent* GeneratedMesh;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// This is called when actor is spawned (at runtime or when you drop it into the world in editor)
	virtual void PostActorCreated() override;

	// This is called when actor is already in level and map is opened
	virtual void PostLoad() override;

};





