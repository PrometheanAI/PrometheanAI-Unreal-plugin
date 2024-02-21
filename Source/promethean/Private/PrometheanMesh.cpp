#include "PrometheanMesh.h"
#include "ProceduralMeshComponent.h"
#include "PrometheanGeneric.h"
#include "Promethean.h"
#include "Editor.h"

static bool IsTriangleFaceInverted(const FVector& Vertex1, const FVector& Vertex2, const FVector& Vertex3, const FVector& Normal);
static FVector CalcPerpendicularNormal(const FVector& Vertex1, const FVector& Vertex2, const FVector& Vertex3);

/*
	Standalone app sends one normal per face via TCP. One normal has to be copied into all 3 vertices of a triangle, to make one normal per vertex.
	Moreover, vertices has to be cloned, to create hard edges. In the end, all three buffers should have equal size.
*/
static void TransformNormalsToUE4(const TArray<TSharedPtr<FJsonValue>>& TriangleIDs,
									const TArray<TSharedPtr<FJsonValue>>& Verts,
									const TArray<TSharedPtr<FJsonValue>>& Normals,
									FPrometheanMeshParameters& Result)
{
	for (int32 TriangleCount = 0; TriangleCount < TriangleIDs.Num(); ++TriangleCount)
	{
		// For each normal (for each face),
		auto TriangleVArray = TriangleIDs[TriangleCount]->AsArray(); // Get 3 indexes of a single triangle in form of an array,
		for (auto ID : TriangleVArray) {
			// For each triangle index,
			auto VertVArray = Verts[ID->AsNumber()]->AsArray(); // Get single vertex location vector in form of an array,
			FVector CurrentVertexLoc = FVector{ (float)VertVArray[0]->AsNumber(), (float)VertVArray[1]->AsNumber(), (float)VertVArray[2]->AsNumber() };
			Result.VertLocations.Add(CurrentVertexLoc); // Create new vertex,
			Result.TriangleIDs.Add(Result.VertLocations.Num() - 1); // Create new triangle index,

			if (Normals.Num() > 0)
			{
				auto NormalVArray = Normals[TriangleCount]->AsArray(); // Get single normal vector in form of an array,
				FVector CurrentNormal = FVector{ (float)NormalVArray[0]->AsNumber(), (float)NormalVArray[1]->AsNumber(), (float)NormalVArray[2]->AsNumber() };
				Result.Normals.Add(CurrentNormal); // For each vertex in the triangle, assign same normal.
			}
		}

		// If normals are not provided, generate default normals for every verts. It is outside the triangle loop, as Result.VertLocations needs to contain the last triangle vertex data.
		if (Normals.Num() <= 0)
		{
			// TODO: If triangle winding provided in TCP call is different than in UE4, change the vertices order provided to CalcPerpendicularNormal function.
			FVector DefaultNormal = CalcPerpendicularNormal(Result.VertLocations.Last(2), Result.VertLocations.Last(1), Result.VertLocations.Last(0));
			DefaultNormal.Normalize();
			// For each vertex in the triangle, assign same normal.
			Result.Normals.Add(DefaultNormal);
			Result.Normals.Add(DefaultNormal);
			Result.Normals.Add(DefaultNormal);
		}
	}
}

/*
	TransformFacesToUE4 takes mesh data and convert the triangle indices into clockwise or counter-clockwise order, to make it visible from supplied normal vectors perspective.
	In UE4, clockwise faces are displayed and counter-clockwise faces are culled.
*/
static void TransformFacesToUE4(TArray<int32>& TriangleIDs, const TArray<FVector>& Verts, const TArray<FVector>& Normals)
{
	int32 FaceIndex = 0;
	for (int32 i = 0; i + 2 < TriangleIDs.Num(); i += 3) {
		if (IsTriangleFaceInverted(Verts[TriangleIDs[i]], Verts[TriangleIDs[i + 1]], Verts[TriangleIDs[i + 2]], Normals[TriangleIDs[i]])) {
			TriangleIDs.SwapMemory(i, i + 2);
		}
	}
}

static void GenerateDefaultUVs(const TArray<FVector>& VertLocations, const TArray<FVector>& Normals, TArray<FVector2D>& ResultUVs)
{
	for (int32 VertexCounter = 0; VertexCounter < VertLocations.Num(); ++VertexCounter)
	{
		 float YZ = FVector::DotProduct(Normals[VertexCounter], FVector{ 1, 0, 0 });
		 float XZ = FVector::DotProduct(Normals[VertexCounter], FVector{ 0, 1, 0 });
		 float XY = FVector::DotProduct(Normals[VertexCounter], FVector{ 0, 0, 1 });
		 float Highest = FMath::Max3(FMath::Abs(YZ), FMath::Abs(XZ), FMath::Abs(XY));
		 if (Highest == FMath::Abs(YZ))
		 {
			 ResultUVs.Add(FVector2D{ -float(VertLocations[VertexCounter].Y) * FMath::Sign(YZ), -float(VertLocations[VertexCounter].Z) } * 0.01 );
		 }
		 else if (Highest == FMath::Abs(XZ))
		 {
			 ResultUVs.Add(FVector2D{ float(VertLocations[VertexCounter].X) * FMath::Sign(XZ), -float(VertLocations[VertexCounter].Z) } * 0.01);
		 }
		 else
		 {
			 ResultUVs.Add(FVector2D{ float(VertLocations[VertexCounter].X) * FMath::Sign(XY), float(VertLocations[VertexCounter].Y) } * 0.01 );
		 }
	}
}

/* Format is as follows - higher level key is the orginical unique name of the asset in standalone memory, the value is the dictionary of object data to generate
	{"ceiling_4847230804619326912":{"name":"ceiling","tri_ids":[[0,1,2],[0,2,3]],"verts":[[-25.38,274.54,-9.38],[-368.04,274.54,-9.38],[-368.04,274.54,-166.49],[-25.38,274.54,-166.49]],"normals":[[0.0,-1.0,0.0],[0.0,-1.0,0.0]]},"floor_3593213985134695789":{"name":"__floor","tri_ids":[[0,1,2],[0,2
	,3]],"verts":[[-25.38,0.0,-9.38],[-368.04,0.0,-9.38],[-368.04,0.0,-166.49],[-25.38,0.0,-166.49]],"normals":[[0.0,1.0,0.0],[0.0,1.0,0.0]]},"wall_3044943331162771623":{"name":"__wall","tri_ids":[[0,1,2],[0,3,2],[1,4,5],[1,2,5],[4,6,7],[4,5,7],[6,8,7],[8,9,7],[9,10,3],[9,7,3],[0,11,3],[11,10,3],[8,9,12],[9,13,12],[9,10,13],[10,14,13],[11,10,15],[10,14,15]],"ver
	ts":[[-368.04,0.0,-9.38],[-368.04,0.0,-166.49],[-368.04,274.66,-166.49],[-368.04,27...
	*/
FString APrometheanMeshActor::GenerateMeshes(const FString& JsonInputString) {
	FString OutString = TEXT("");
	TSharedPtr<FJsonObject> OutJsonObject = MakeShared<FJsonObject>();
	TSharedRef<FJsonValueObject> OutJsonObjectValue = MakeShared<FJsonValueObject>(OutJsonObject);
	TSharedPtr<FJsonObject> JsonParsed;
	TSharedRef<FJsonStringReader> JsonReader = FJsonStringReader::Create(JsonInputString);
	if (FJsonSerializer::Deserialize((TSharedRef<TJsonReader<TCHAR>>)JsonReader, JsonParsed))
	{
		// Don't run this function without editor (on loading screen)
		if (GEditor == nullptr)
		{
			return OutString;
		}
		if (GEditor->GetEditorWorldContext().World() == nullptr)
		{
			return OutString;
		}

		// For every mesh in json string...
		for (auto& ObjectTMap : JsonParsed->Values)
		{
			auto OriginalDCCName = ObjectTMap.Key;
			auto ObjectValue = ObjectTMap.Value;
			auto ObjectJsonDict = ObjectValue->AsObject();
			UE_LOG(LogPromethean, Display, TEXT("Add Objects from geometry original DCC name: %s"), *OriginalDCCName);
			
			// if no name found, use default one
			auto Name = ObjectJsonDict->GetStringField("Name");
			if (Name.IsEmpty())
			{
				Name = "PrometheanMeshActor";
			}

			// Get actual values from json.
			auto Obj = AddActorToWorld<APrometheanMeshActor>(GEditor->GetEditorWorldContext().World(), Name, FVector(0.0f, 0.0f, 0.0f));
			auto TriangleIDs = ObjectJsonDict->GetArrayField("tri_ids");
			auto Verts = ObjectJsonDict->GetArrayField("verts");
			auto Normals = ObjectJsonDict->GetArrayField("normals");
			auto UVs = ObjectJsonDict->GetArrayField("uvs");
			auto Tangents = ObjectJsonDict->GetArrayField("tangents");
			auto VColors = ObjectJsonDict->GetArrayField("vcolors");
			auto Material = ObjectJsonDict->GetStringField("material");

			// Throw error if triangle indices or vertex locations are missing in the json string
			if (TriangleIDs.Num() <= 0)
			{
				UE_LOG(LogPromethean, Error, TEXT("PrometheanMeshActor>> GenerateMesh: json string is correct but does not contain triangle IDs!"));
				return OutString;
			}
			if (Verts.Num() <= 0)
			{
				UE_LOG(LogPromethean, Error, TEXT("PrometheanMeshActor>> GenerateMesh: json string is correct but does not contain vertices' data!"));
				return OutString;
			}

			// Data from standalone app is different than ue4 render system: read description of following functions.
			TransformNormalsToUE4(TriangleIDs, Verts, Normals, Obj->GeneratedMeshParams); // Transform face normals into vertex normals, support hard edges.
			TransformFacesToUE4(Obj->GeneratedMeshParams.TriangleIDs, Obj->GeneratedMeshParams.VertLocations, Obj->GeneratedMeshParams.Normals); // Adjust faces to aim normals' directions.

			// Supply rest of the data into UE4 containers
			for (auto& tangent : Tangents)
			{
				auto tangent_varray = tangent->AsArray();
				Obj->GeneratedMeshParams.Tangents.Add(FProcMeshTangent{ (float)tangent_varray[0]->AsNumber(), (float)tangent_varray[1]->AsNumber(), (float)tangent_varray[2]->AsNumber() });
			}

			// UVs
			if (UVs.Num() > 0)
			{
				// if UVs are supplied, add them to generated mesh params.
				for (auto& uv : UVs)
				{
					auto uv_varray = uv->AsArray();
					Obj->GeneratedMeshParams.UVs.Add(FVector2D{ (float)uv_varray[0]->AsNumber(), (float)uv_varray[1]->AsNumber() });
				}
			}
			else
			{
				// else, generate default ones.
				GenerateDefaultUVs(Obj->GeneratedMeshParams.VertLocations, Obj->GeneratedMeshParams.Normals, Obj->GeneratedMeshParams.UVs);
			}

			for (auto& vcolor : VColors)
			{
				auto vcolor_array = vcolor->AsArray();
				Obj->GeneratedMeshParams.VertexColors.Add(FLinearColor{ (float)vcolor_array[0]->AsNumber(), (float)vcolor_array[1]->AsNumber(), (float)vcolor_array[2]->AsNumber(), (float)vcolor_array[3]->AsNumber() });
			}

			// Generate the mesh in UE4.
			Obj->RegeneratePrometheanMesh();

			// Set material if found.
			if (!Material.IsEmpty() && !Obj->TryToSetMaterial(Material, 0))
			{
				UE_LOG(LogPromethean, Error, TEXT("PrometheanMeshActor>> GenerateMesh: material not found!"));
			}

			// Setting pivot to bottom center of generated mesh.
			auto Center = Obj->GeneratedMesh->Bounds.Origin;
			Obj->SetPivotOffset(FVector(Center.X, Center.Y, Obj->GeneratedMesh->Bounds.Origin.Z - Obj->GeneratedMesh->Bounds.GetBox().GetExtent().Z));

			OutJsonObject->SetStringField(OriginalDCCName, Obj->GetName().ToLower());
		}
	}
	else {
		UE_LOG(LogPromethean, Error, TEXT("PrometheanMeshActor>> GenerateMesh: json string is not correct! ~> %s"), *JsonReader->GetErrorMessage());
	}

	// Send response to standalone app.
	TSharedRef< TJsonWriter<> > Writer = TJsonWriterFactory<>::Create(&OutString);
	FJsonSerializer::Serialize(OutJsonObject.ToSharedRef(), Writer);
	return OutString;
}

/*
	Function uses the vertices in deterministic and order dependent cross product, to find whether face is directed differently than supplied normal vector.
	Inverted faces for UE4 are counter-clockwise faces. Assuming 2D triangle laying flat, feeding this function with vertices in clockwise order and up normal, gives false.
*/
static bool IsTriangleFaceInverted(const FVector& Vertex1, const FVector& Vertex2, const FVector& Vertex3, const FVector& Normal) {
	return FVector::DotProduct(Normal, CalcPerpendicularNormal(Vertex1, Vertex2, Vertex3)) < 0.0f;
}

/*
	It should generate the correct normal in UE4's triangle winding manner: clockwise winding is visible, and counter-clockwise is not visible.
	If this function works the opposite, please, change the vertices order in the function call. Do not change this function alone.
*/
static FVector CalcPerpendicularNormal(const FVector& Vertex1, const FVector& Vertex2, const FVector& Vertex3) {
	return FVector::CrossProduct(Vertex3 - Vertex1, Vertex2 - Vertex1);
}

// Sets default values
APrometheanMeshActor::APrometheanMeshActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	GeneratedMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("GeneratedMesh"));
	RootComponent = GeneratedMesh;
	RootComponent->SetMobility(EComponentMobility::Static); // Make sure it is static
	GeneratedMesh->bUseAsyncCooking = true; // New in UE 4.17, multi-threaded PhysX cooking.

	UMaterialInterface* DefaultMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("Material'/Engine/EngineMaterials/WorldGridMaterial.WorldGridMaterial'"));
	GeneratedMesh->SetMaterial(0, DefaultMaterial);
}

/*
	Sets the material using supplied path to asset. Returns true if succeeded.
*/
bool APrometheanMeshActor::TryToSetMaterial(const FString& MaterialPath, const int MaterialIndex) {
	return SetMaterialForMeshComponent(GeneratedMesh, MaterialPath, MaterialIndex);
}

/*
	Takes currently generated mesh and transfer its data to newly created static mesh asset.
*/
/*  // TODO 4.24 replaced FMeshDescription for FStaticMeshAttributes - need to fix!
void APrometheanMeshActor::ConvertGeneratedMeshToStaticMesh()
{
	if (GetWorld() == nullptr)
	{
		// returns if actor is not spawned.
		return;
	}	

	FString ActorName = GetName();
	FString LevelName = GetWorld()->GetMapName();
	FString AssetName = LevelName + FString(TEXT("_")) + ActorName;
	FString PathName = FString(TEXT("/Game/PrometheanAI/"));
	FString PackageName = PathName + AssetName;

	FMeshDescription MeshDescription;

	UStaticMesh::RegisterMeshAttributes(MeshDescription);
	FStaticMeshDescriptionAttributeGetter AttributeGetter(&MeshDescription);
	TPolygonGroupAttributesRef<FName> PolygonGroupNames = AttributeGetter.GetPolygonGroupImportedMaterialSlotNames();
	TVertexAttributesRef<FVector> VertexPositions = AttributeGetter.GetPositions();
	TVertexInstanceAttributesRef<FVector> Tangents = AttributeGetter.GetTangents();
	TVertexInstanceAttributesRef<float> BinormalSigns = AttributeGetter.GetBinormalSigns();
	TVertexInstanceAttributesRef<FVector> Normals = AttributeGetter.GetNormals();
	TVertexInstanceAttributesRef<FVector4> Colors = AttributeGetter.GetColors();
	TVertexInstanceAttributesRef<FVector2D> UVs = AttributeGetter.GetUVs();
	TEdgeAttributesRef<bool> EdgeHardnesses = AttributeGetter.GetEdgeHardnesses();
	TEdgeAttributesRef<float> EdgeCreaseSharpnesses = AttributeGetter.GetEdgeCreaseSharpnesses();

	// Materials to apply to new mesh
	const int32 NumSections = GeneratedMesh->GetNumSections();
	int32 VertexCount = 0;
	int32 VertexInstanceCount = 0;
	int32 PolygonCount = 0;
	TMap<UMaterialInterface*, FPolygonGroupID> UniqueMaterials;
	UniqueMaterials.Reserve(NumSections);
	TArray<FPolygonGroupID> MaterialRemap;
	MaterialRemap.Reserve(NumSections);
	//Get all the info we need to create the MeshDescription
	for (int32 SectionIdx = 0; SectionIdx < NumSections; SectionIdx++)
	{
		FProcMeshSection* ProcSection = GeneratedMesh->GetProcMeshSection(SectionIdx);
		VertexCount += ProcSection->ProcVertexBuffer.Num();
		VertexInstanceCount += ProcSection->ProcIndexBuffer.Num();
		PolygonCount += ProcSection->ProcIndexBuffer.Num() / 3;

		UMaterialInterface* Material = GeneratedMesh->GetMaterial(SectionIdx);
		if (Material == nullptr)
		{
			UMaterialInterface* DefaultMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("Material'/Engine/EngineMaterials/WorldGridMaterial.WorldGridMaterial'"));
			Material = DefaultMaterial;
		}

		if (!UniqueMaterials.Contains(Material))
		{
			FPolygonGroupID NewPolygonGroup = MeshDescription.CreatePolygonGroup();
			UniqueMaterials.Add(Material, NewPolygonGroup);
			PolygonGroupNames[NewPolygonGroup] = Material->GetFName();
		}
		FPolygonGroupID* PolygonGroupID = UniqueMaterials.Find(Material);
		if (PolygonGroupID == nullptr)
		{
			return;
		}
		MaterialRemap.Add(*PolygonGroupID);
	}
	MeshDescription.ReserveNewVertices(VertexCount);
	MeshDescription.ReserveNewVertexInstances(VertexInstanceCount);
	MeshDescription.ReserveNewPolygons(PolygonCount);
	MeshDescription.ReserveNewEdges(PolygonCount * 2);
	UVs.SetNumIndices(4);
	//Add Vertex and VertexInstance and polygon for each section
	for (int32 SectionIdx = 0; SectionIdx < NumSections; SectionIdx++)
	{
		FProcMeshSection* ProcSection = GeneratedMesh->GetProcMeshSection(SectionIdx);
		FPolygonGroupID PolygonGroupID = MaterialRemap[SectionIdx];
		//Create the vertex
		int32 NumVertex = ProcSection->ProcVertexBuffer.Num();
		TMap<int32, FVertexID> VertexIndexToVertexID;
		VertexIndexToVertexID.Reserve(NumVertex);
		for (int32 VertexIndex = 0; VertexIndex < NumVertex; ++VertexIndex)
		{
			FProcMeshVertex& Vert = ProcSection->ProcVertexBuffer[VertexIndex];
			const FVertexID VertexID = MeshDescription.CreateVertex();
			VertexPositions[VertexID] = Vert.Position;
			VertexIndexToVertexID.Add(VertexIndex, VertexID);
		}
		//Create the VertexInstance
		int32 NumIndices = ProcSection->ProcIndexBuffer.Num();
		int32 NumTri = NumIndices / 3;
		TMap<int32, FVertexInstanceID> IndiceIndexToVertexInstanceID;
		IndiceIndexToVertexInstanceID.Reserve(NumVertex);
		for (int32 IndiceIndex = 0; IndiceIndex < NumIndices; IndiceIndex++)
		{
			const int32 VertexIndex = ProcSection->ProcIndexBuffer[IndiceIndex];
			const FVertexID VertexID = VertexIndexToVertexID[VertexIndex];
			const FVertexInstanceID VertexInstanceID = MeshDescription.CreateVertexInstance(VertexID);
			IndiceIndexToVertexInstanceID.Add(IndiceIndex, VertexInstanceID);

			FProcMeshVertex& ProcVertex = ProcSection->ProcVertexBuffer[VertexIndex];

			Tangents[VertexInstanceID] = ProcVertex.Tangent.TangentX;
			Normals[VertexInstanceID] = ProcVertex.Normal;
			BinormalSigns[VertexInstanceID] = ProcVertex.Tangent.bFlipTangentY ? -1.f : 1.f;

			Colors[VertexInstanceID] = FLinearColor(ProcVertex.Color);

			UVs.Set(VertexInstanceID, 0, ProcVertex.UV0);
			UVs.Set(VertexInstanceID, 1, ProcVertex.UV1);
			UVs.Set(VertexInstanceID, 2, ProcVertex.UV2);
			UVs.Set(VertexInstanceID, 3, ProcVertex.UV3);
		}

		//Create the polygons for this section
		for (int32 TriIdx = 0; TriIdx < NumTri; TriIdx++)
		{
			FVertexID VertexIndexes[3];
			TArray<FVertexInstanceID> VertexInstanceIDs;
			VertexInstanceIDs.SetNum(3);

			for (int32 CornerIndex = 0; CornerIndex < 3; ++CornerIndex)
			{
				const int32 IndiceIndex = (TriIdx * 3) + CornerIndex;
				const int32 VertexIndex = ProcSection->ProcIndexBuffer[IndiceIndex];
				VertexIndexes[CornerIndex] = VertexIndexToVertexID[VertexIndex];
				VertexInstanceIDs[CornerIndex] = IndiceIndexToVertexInstanceID[IndiceIndex];
			}

			// Insert a polygon into the mesh
			const FPolygonID NewPolygonID = MeshDescription.CreatePolygon(PolygonGroupID, VertexInstanceIDs);
			//Triangulate the polygon
			FMeshPolygon& Polygon = MeshDescription.GetPolygon(NewPolygonID);
			MeshDescription.ComputePolygonTriangulation(NewPolygonID, Polygon.Triangles);
		}
	}

	// If we got some valid data.
	if (MeshDescription.Polygons().Num() > 0)
	{
		// Create StaticMesh object
		UStaticMesh* StaticMesh = NewObject<UStaticMesh>(GetTransientPackage(), *AssetName, RF_Public | RF_Standalone);
		StaticMesh->InitResources();

		StaticMesh->LightingGuid = FGuid::NewGuid();

		// Add source to new StaticMesh
		FStaticMeshSourceModel& SrcModel = StaticMesh->AddSourceModel();
		SrcModel.BuildSettings.bRecomputeNormals = false;
		SrcModel.BuildSettings.bRecomputeTangents = false;
		SrcModel.BuildSettings.bRemoveDegenerates = false;
		SrcModel.BuildSettings.bUseHighPrecisionTangentBasis = false;
		SrcModel.BuildSettings.bUseFullPrecisionUVs = false;
		SrcModel.BuildSettings.bGenerateLightmapUVs = true;
		SrcModel.BuildSettings.SrcLightmapIndex = 0;
		SrcModel.BuildSettings.DstLightmapIndex = 1;
		FMeshDescription* OriginalMeshDescription = StaticMesh->GetMeshDescription(0);
		if (OriginalMeshDescription == nullptr)
		{
			OriginalMeshDescription = StaticMesh->CreateMeshDescription(0);
		}
		*OriginalMeshDescription = MeshDescription;
		StaticMesh->CommitMeshDescription(0);

		// Copy materials to new mesh
		for (auto Kvp : UniqueMaterials)
		{
			UMaterialInterface* Material = Kvp.Key;
			StaticMesh->StaticMaterials.Add(FStaticMaterial(Material, Material->GetFName(), Material->GetFName()));
		}

		//Set the Imported version before calling the build
		StaticMesh->ImportVersion = EImportStaticMeshVersion::LastVersion;

		// Build mesh from source
		StaticMesh->Build(false);
		
		AddObjectToContentBrowser(StaticMesh, PackageName);

		// Replace generated mesh
		FVector Location = FVector(0.0f, 0.0f, 0.0f);
		FVector Rotation = FVector(0.0f, 0.0f, 0.0f);
		FVector Scale = FVector(1.0f, 1.0f, 1.0f);
		RemoveActorByName(ActorName, GEditor->GetEditorWorldContext().World());  // remove original
		PackageName = PackageName + "." + AssetName;  // add .AssetName at the end
		UE_LOG(LogPromethean, Error, TEXT("Adding actor with path: %s"), *PackageName);		
		FPlatformProcess::Sleep(3.0f);  // need to wait for the save thread to finish		
		addStaticMeshActor(GEditor->GetEditorWorldContext().World(), PackageName, ActorName, Location, Rotation, Scale);  // add new one
		// TODO make this work with timers instead of a sleep
		//
		//FTimerDelegate TimerDel;
		//FTimerHandle TimerHandle;
		////Binding the function with specific variables
		//TimerDel.BindLambda(this, FName("addStaticMeshActor"), GEditor->GetEditorWorldContext().World(), PackageName, ActorName, Location, Rotation, Scale);
		////Calling addStaticMeshActor after 4 seconds without looping
		//GetWorldTimerManager().SetTimer(TimerHandle, TimerDel, 4.f, false);
		//
	}
	
}
*/

/*
	Creates section of procedural mesh, using GeneratedMeshParams from APrometheanMeshActor instance. Can be used to regenerate the mesh but might be rather slow.
*/
void APrometheanMeshActor::RegeneratePrometheanMesh() {
	GeneratedMesh->CreateMeshSection_LinearColor(
		0, // Index of the section to create or replace.
		GeneratedMeshParams.VertLocations, // Vertex buffer of all vertex positions to use for this mesh section.
		GeneratedMeshParams.TriangleIDs, // Index buffer indicating which vertices make up each triangle. Length must be a multiple of 3. Order sensitive (face culling)!
		GeneratedMeshParams.Normals, // Optional array of normal vectors for each vertex. If supplied, must be same length as Vertices array.
		GeneratedMeshParams.UVs, // Optional array of texture co-ordinates for each vertex. If supplied, must be same length as Vertices array.
		GeneratedMeshParams.VertexColors, // Optional array of colors for each vertex. If supplied, must be same length as Vertices array.
		GeneratedMeshParams.Tangents, // Optional array of tangent vector for each vertex. If supplied, must be same length as Vertices array.
		true); // Indicates whether collision should be created for this section. This adds significant cost.
	GeneratedMesh->ContainsPhysicsTriMeshData(true); // Enable collision data

}


// Called when the game starts or when spawned
void APrometheanMeshActor::BeginPlay()
{
	Super::BeginPlay();

}

// Called every frame
void APrometheanMeshActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void APrometheanMeshActor::PostActorCreated()
{
	Super::PostActorCreated();
	//RegeneratePrometheanMesh(); // In case we would have to regenerate the mesh at this point. Sadly, it also means regenerating an empty mesh when actor is being added by TCP.
}

// This is called when actor is already in level and map is opened
void APrometheanMeshActor::PostLoad()
{
	Super::PostLoad();
	//RegeneratePrometheanMesh(); // In case we would have to regenerate the mesh at this point. Sadly, it also means regenerating an empty mesh when actor is being added by TCP.
}




