// Fill out your copyright notice in the Description page of Project Settings.

#include "PrometheanLibraryData.h"  // matching header has to go first
#include "PrometheanGeneric.h"
#include "promethean.h" // has stuff necessary for custom logging


#include "UObject/Object.h"
#include "Engine/Texture2D.h"
#include "EngineUtils.h"
#include "Engine/StaticMesh.h"
#include "Engine/ObjectLibrary.h"

#include "Materials/Material.h"
#include "Materials/MaterialInstanceConstant.h"
#if ENGINE_MAJOR_VERSION >= 4  && ENGINE_MINOR_VERSION > 19  // MaterialStatsCommon missing on 4.19 so compiling
	#include "MaterialStatsCommon.h"
#endif
#if ENGINE_MAJOR_VERSION >= 4  && ENGINE_MINOR_VERSION >= 21
	#include "Misc/FileHelper.h" // - to output images on 4.21 and higher
#endif
#include "HighResScreenshot.h"  // - to output asset thumbnails on 4.20 and below

#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 1
#include "ImageCore.h"
#endif
#include "ImageUtils.h"
#include "IImageWrapperModule.h"
#include "ObjectTools.h"  // - has ThumbnailTools manager!
#if ENGINE_MAJOR_VERSION >= 5
#include "TextureCompiler.h"
#endif
#include "UnrealEdGlobals.h"
#include "AutoReimport/AssetSourceFilenameCache.h"  // - getting source asset without having to load it
#include "Dom/JsonObject.h"
#include "Editor/UnrealEdEngine.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "Misc/FileHelper.h"
#include "Misc/ScopedSlowTask.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Slate/SlateTextures.h"
#include "ThumbnailRendering/ThumbnailManager.h"
//#include "MaterialStats.h" // can't import, private but has declarations for material stats info that we need

// --------------------------------------------------------------------
// --- ASSET LIBRARY FUNCTIONS
// --------------------------------------------------------------------

DECLARE_STATS_GROUP(TEXT("Promethean_AI"), STATGROUP_PrometheanLibraryData, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("Promethean - SaveThumbnail"), STAT_SaveThumbnail, STATGROUP_PrometheanLibraryData);

/** Generates a thumbnail for the specified object and caches it */
FObjectThumbnail* GenerateHighResolutionThumbnail( UObject* InObject )
{
	return NULL;
	if constexpr (!bGenerateHighResThumbnails)
	{
		return nullptr;
	}
	
	// Does the object support thumbnails?
	FThumbnailRenderingInfo* RenderInfo = GUnrealEd ? GUnrealEd->GetThumbnailManager()->GetRenderingInfo( InObject ) : nullptr;
	if( RenderInfo != NULL && RenderInfo->Renderer != NULL )
	{
		// Set the size of cached thumbnails
		const int32 ImageWidth = RenderedThumbnailSize;
		const int32 ImageHeight = RenderedThumbnailSize;

		// For cached thumbnails we want to make sure that textures are fully streamed in so that the thumbnail we're saving won't have artifacts
		// However, this can add 30s - 100s to editor load
		//@todo - come up with a cleaner solution for this, preferably not blocking on texture streaming at all but updating when textures are fully streamed in
		ThumbnailTools::EThumbnailTextureFlushMode::Type TextureFlushMode = ThumbnailTools::EThumbnailTextureFlushMode::NeverFlush;

#if ENGINE_MAJOR_VERSION >= 5
		if ( UTexture* Texture = Cast<UTexture>(InObject) )
		{
			FTextureCompilingManager::Get().FinishCompilation({Texture});
			Texture->WaitForStreaming();
		}
#endif

#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 1
		// When generating a material thumbnail to save in a package, make sure we finish compilation on the material first
		if ( UMaterial* InMaterial = Cast<UMaterial>(InObject) )
		{
			FScopedSlowTask SlowTask(0, NSLOCTEXT( "ObjectTools", "FinishingCompilationStatus", "Finishing Shader Compilation..." ) );
			SlowTask.MakeDialog();

			// Block until the shader maps that we will save have finished being compiled
			FMaterialResource* CurrentResource = InMaterial->GetMaterialResource(GMaxRHIFeatureLevel);
			if (CurrentResource)
			{
				if (!CurrentResource->IsGameThreadShaderMapComplete())
				{
					CurrentResource->SubmitCompileJobs_GameThread(EShaderCompileJobPriority::High);
				}
				CurrentResource->FinishCompilation();
			}
		}
#endif

		// Generate the thumbnail
		FObjectThumbnail NewThumbnail;
		ThumbnailTools::RenderThumbnail(
			InObject, ImageWidth, ImageHeight, TextureFlushMode, NULL,
			&NewThumbnail );		// Out

		UPackage* MyOutermostPackage = InObject->GetOutermost();
		return ThumbnailTools::CacheThumbnail( InObject->GetFullName(), &NewThumbnail, MyOutermostPackage );
	}

	return NULL;
}

// start of this is copied from https://forums.unrealengine.com/development-discussion/c-gameplay-programming/76797-how-to-save-utexture2d-to-png-file
// but repurposed to work with object thumbnail
bool SaveThumbnail(FObjectThumbnail* ObjectThumbnail, const FString& ImagePath, const bool bIsCompressed)
{
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 1
	if (!ObjectThumbnail || ObjectThumbnail->IsEmpty())
	{
		return false;
	}

	SCOPE_CYCLE_COUNTER(STAT_SaveThumbnail);
    
	const int32 w = ObjectThumbnail->GetImageWidth();
	const int32 h = ObjectThumbnail->GetImageHeight();

	if (bIsCompressed)
	{
		ObjectThumbnail->DecompressImageData();
	}
	const TArray<uint8>& ImageData = ObjectThumbnail->AccessImageData();		

	FImageView ImageView( (void*)ImageData.GetData(), h, w, ERawImageFormat::BGRA8);
	bool bSaved = FImageUtils::SaveImageByExtension(*ImagePath, ImageView);

	UE_LOG(LogPromethean, Display, TEXT("ThumbnailSize: %d %d"), w, h);
	UE_LOG(LogPromethean, Display, TEXT("SaveThumbnail: %s %d"), *ImagePath, bSaved == true ? 1 : 0);
	return bSaved;
#else
	TArray<FColor> OutBMP;
	int w = ObjectThumbnail->GetImageWidth();
	int h = ObjectThumbnail->GetImageHeight();

	if (bIsCompressed)
	{
		ObjectThumbnail->DecompressImageData();
	}
	const TArray<uint8>& ImageData = ObjectThumbnail->AccessImageData();
	if (ImageData.Num() < (w * h * 4)) // In some extreme cases, ImageData may be empty. No idea why.
	{
		return false;
	}

	OutBMP.InsertZeroed(0, w*h);

	for (int i = 0; i < (w*h); ++i)
	{
		int j = i * 4;		
		//FColor* Color = new FColor(ImageData[j+2], ImageData[j + 1], ImageData[j], ImageData[j + 3]);  // RED and BLUE are switched
		FColor* Color = new FColor(ImageData[j+2], ImageData[j + 1], ImageData[j], 255);  // RED and BLUE are switched
		OutBMP[i] = *Color;		
	}
	
	FIntPoint DestSize(w, h);	

	TArray<FString> FileList;	
	bool bSaved = FFileHelper::CreateBitmap(*ImagePath, w, h, OutBMP.GetData());

	UE_LOG(LogPromethean, Display, TEXT("ThumbnailSize: %d %d"), w, h);
	UE_LOG(LogPromethean, Display, TEXT("SaveThumbnail: %s %d"), *ImagePath, bSaved == true ? 1 : 0);
	return bSaved;
#endif
}

void GetAssetLibraryData(const FString& DataTypeName)
{
	UE_LOG(LogPromethean, Display, TEXT("Getting asset library data..."));
	
	UClass* cls = TSubclassOf<class UObject>();
	
	UObjectLibrary* ObjectLibrary = UObjectLibrary::CreateLibrary(cls, false, GIsEditor);
	
	if (ObjectLibrary != nullptr)
	{
		ObjectLibrary->AddToRoot();

		FString NewPath = TEXT("/Game");  // "/Game/StarterContent/Architecture"
		int32 NumOfAssetDatas = ObjectLibrary->LoadAssetDataFromPath(NewPath);

		TArray<FAssetData> AssetDatas;
		ObjectLibrary->GetAssetDataList(AssetDatas);

		UE_LOG(LogPromethean, Display, TEXT("About to go through all asset datas... Datas Num: %i"), AssetDatas.Num());
		
		TArray<TSharedPtr<FJsonValue>> JSONDictArray;  // for output
		for (int32 i = 0; i < AssetDatas.Num(); ++i)  // can't print AssetDatas.Num() - crashes!
		{
			FAssetData& AssetData = AssetDatas[i];
			if (AssetData.IsValid())
			{
				// currently get only all static meshes
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 1
				if (AssetData.AssetClassPath.GetAssetName().IsEqual(*DataTypeName))
#else
				if (AssetData.AssetClass.IsEqual(*DataTypeName))
#endif
				{									
					TSharedPtr<FJsonObject> JSONDict = MakeShared<FJsonObject>();
					if (DataTypeName == "MaterialInstanceConstant")
					{
						JSONDict = getMaterialInstanceAssetData(AssetData);
					}
					else if (DataTypeName == "Material")
					{
						JSONDict = getMaterialAssetData(AssetData);
					}
					else if (DataTypeName == "StaticMesh")
					{
						JSONDict = getStaticMeshAssetData(AssetData);
					}
					else if (DataTypeName == "Texture2D")
					{
						JSONDict = getTextureAssetData(AssetData);
					}
					else if (DataTypeName == "World")
					{
						// JSONDict = getLevelAssetData(AssetData);
					}
					TSharedRef<FJsonValueObject> JSONDictValue = MakeShared<FJsonValueObject>(JSONDict);
					UE_LOG(LogPromethean, Display, TEXT("================================================"));
					// -- Add To Array
					JSONDictArray.Add(JSONDictValue);
				}				
			}
		}
		// output json data to string
		TSharedPtr<FJsonObject> RootObject = MakeShared<FJsonObject>();
		RootObject->SetArrayField(TEXT("asset_data"), JSONDictArray);
		FString OutputString;
		TSharedRef< TJsonWriter<> > Writer = TJsonWriterFactory<>::Create(&OutputString);
		FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);
		SaveToFile(OutFolderPath + "/ue4_asset_data_" + DataTypeName + ".json", OutputString);
	}
}

DECLARE_CYCLE_STAT(TEXT("Promethean - getStaticMeshAssetData"), STAT_GetStaticMeshAssetData, STATGROUP_PrometheanLibraryData);
DECLARE_CYCLE_STAT(TEXT("Promethean - ExtractAssetImportInfo"), STAT_ExtractAssetImportInfo, STATGROUP_PrometheanLibraryData);
DECLARE_CYCLE_STAT(TEXT("Promethean - LoadObjectStuff"), STAT_LoadObjectStuff, STATGROUP_PrometheanLibraryData);
TSharedPtr<FJsonObject> getStaticMeshAssetData(const FAssetData& AssetData, bool bPerformHeavyOperations)
{
	SCOPE_CYCLE_COUNTER(STAT_GetStaticMeshAssetData);
	TSharedPtr<FJsonObject> JSONDict = MakeShared<FJsonObject>();	
	TMap<FName,FString> AllTags;
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 1
	if (AssetData.AssetClassPath.GetAssetName().IsEqual("StaticMesh"))
#else
	if (AssetData.AssetClass.IsEqual("StaticMesh"))
#endif
	{
		// ---- Unique ID - Name
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 1
    	const FString AssetPath = AssetData.GetSoftObjectPath().ToString();
#else
    	const FString AssetPath = AssetData.ObjectPath.ToString();
#endif
    	JSONDict->SetStringField(TEXT("path"), AssetPath);
    	
    	FString FileName = AssetPath;
    	FileName = FileName.Replace(TEXT("/"), TEXT("--"));
    	FString OutputPath = OutFolderPath + "/thumbnails/" + FileName;  // define path globally because it's reused for triangle too

    	if (bPerformHeavyOperations)
    	{
    		SCOPE_CYCLE_COUNTER(STAT_LoadObjectStuff);
    		// ---- WARNING! Have to load the actual StaticMesh here. Need it to get material paths and pivot offset
    		UStaticMesh* StaticMesh = Cast<UStaticMesh>(AssetData.GetAsset());
    		// ---- High Res Thumbs
    		GenerateHighResolutionThumbnail(StaticMesh);		
    		// ---- Material Paths
    		TArray<TSharedPtr<FJsonValue>> MaterialPathArray;
    		for (FStaticMaterial StaticMaterial : StaticMesh->GetStaticMaterials())
    		{
    			//UE_LOG(LogPromethean, Display, TEXT("Material Used: %s"), *StaticMaterial.MaterialInterface->GetPathName());  // gets material slots on the static mesh
    			MaterialPathArray.Add(MakeShareable(new FJsonValueString(StaticMaterial.MaterialInterface->GetPathName())));
    		}
    		JSONDict->SetArrayField(TEXT("material_paths"), MaterialPathArray);
    		// ---- Size (Instead of approx size, update bounding box when object is loaded
    		//UE_LOG(LogPromethean, Display, TEXT("Size: %s"), *StaticMesh->GetBoundingBox().GetSize().ToString());
    		FVector Size = StaticMesh->GetBoundingBox().GetSize();
    		TArray<TSharedPtr<FJsonValue>> MeshScaleArray;
    		MeshScaleArray.Add(MakeShareable(new FJsonValueNumber(Size.X)));
    		MeshScaleArray.Add(MakeShareable(new FJsonValueNumber(Size.Y)));
    		MeshScaleArray.Add(MakeShareable(new FJsonValueNumber(Size.Z)));
    		JSONDict->SetArrayField(TEXT("bounding_box"), MeshScaleArray);		
    		// ---- Pivot Offset
    		FVector PivotOffset = StaticMesh->GetBoundingBox().GetCenter();		
    		PivotOffset[2] = StaticMesh->GetBoundingBox().Min[2];  // min height
    		TArray<TSharedPtr<FJsonValue>> MeshPivotOffsetArray;
    		MeshPivotOffsetArray.Add(MakeShareable(new FJsonValueNumber(-PivotOffset.X)));
    		MeshPivotOffsetArray.Add(MakeShareable(new FJsonValueNumber(-PivotOffset.Y)));
    		MeshPivotOffsetArray.Add(MakeShareable(new FJsonValueNumber(-PivotOffset.Z)));
    		JSONDict->SetArrayField(TEXT("pivot_offset"), MeshPivotOffsetArray);
    		
    		// ---- Raw Triangles
    		TArray<FVector> Vertexes = GetMeshTrianglePositionsFromLibraryStaticMesh(StaticMesh);
    		if (Vertexes.Num() != 0)
    		{
    			FString VertexString = FVectorArrayToJsonString(Vertexes, TEXT("verts"));
    			SaveToFile(OutputPath, VertexString);  // no file format is currently reserved for vertexes
    			JSONDict->SetStringField(TEXT("verts"), OutputPath);
    		}
    	}
    		
		for (const auto& TagAndValuePair : AssetData.TagsAndValues)
		{
			// UE_LOG(LogPromethean, Display, TEXT("Tag NameValue Pair: %s : %s"), *TagAndValuePair.Key.ToString(), *TagAndValuePair.Value);			
			// ---- Material Count
			if (TagAndValuePair.Key.IsEqual("Materials"))
			{
				//UE_LOG(LogPromethean, Display, TEXT("Material Count: %i"), FCString::Atoi(*TagAndValuePair.Value.GetValue()));  // gets material slots on the static mesh
				JSONDict->SetNumberField(TEXT("material_count"), FCString::Atoi(*TagAndValuePair.Value.GetValue()));
			}
			// ---- Face Count
			else if (TagAndValuePair.Key.IsEqual("Triangles"))
			{
				//UE_LOG(LogPromethean, Display, TEXT("Faces: %i"), FCString::Atoi(*TagAndValuePair.Value.GetValue()));
				JSONDict->SetNumberField(TEXT("face_count"), FCString::Atoi(*TagAndValuePair.Value.GetValue()));
			}
			// ---- Vertex Count
			else if (TagAndValuePair.Key.IsEqual("Vertices"))
			{
				//UE_LOG(LogPromethean, Display, TEXT("Vertices: %i"), FCString::Atoi(*TagAndValuePair.Value.GetValue()));
				JSONDict->SetNumberField(TEXT("vertex_count"), FCString::Atoi(*TagAndValuePair.Value.GetValue()));
			}
			// ---- UV Channels
			else if (TagAndValuePair.Key.IsEqual("UVChannels"))
			{
				//UE_LOG(LogPromethean, Display, TEXT("UV Channels: %i"), FCString::Atoi(*TagAndValuePair.Value.GetValue()));
				JSONDict->SetNumberField(TEXT("uv_sets"), FCString::Atoi(*TagAndValuePair.Value.GetValue()));
			}			
			// ---- Size
			// Get precise size from the static mesh when we load it
			else if (TagAndValuePair.Key.IsEqual("ApproxSize"))
			{				
				TArray<FString> DimensionsList;
				FString TagValue = TagAndValuePair.Value.GetValue();
				TagValue.ParseIntoArray(DimensionsList, TEXT("x"), true);  // value is "8x16x4"
				if (DimensionsList.Num() == 3)  // make sure we split correctly into 3 items. unreal crashes if index is out of range for whatever unlikely reason
				{
					UE_LOG(LogPromethean, Display, TEXT("Size: %s"), *TagValue);
					TArray<TSharedPtr<FJsonValue>> MeshScaleArray;
					MeshScaleArray.Add(MakeShareable(new FJsonValueNumber(FCString::Atoi(*DimensionsList[0]))));
					MeshScaleArray.Add(MakeShareable(new FJsonValueNumber(FCString::Atoi(*DimensionsList[1]))));  // swizzle
					MeshScaleArray.Add(MakeShareable(new FJsonValueNumber(FCString::Atoi(*DimensionsList[2]))));  // swizzle
					JSONDict->SetArrayField(TEXT("bounding_box"), MeshScaleArray);
				}
			}
			// ---- LODs
			else if (TagAndValuePair.Key.IsEqual("LODs"))
			{				
				//UE_LOG(LogPromethean, Display, TEXT("Lod Count: %i"), FCString::Atoi(*TagAndValuePair.Value.GetValue()));
				JSONDict->SetNumberField(TEXT("lod_count"), FCString::Atoi(*TagAndValuePair.Value.GetValue()));
			}
			
		}		
		//UE_LOG(LogPromethean, Display, TEXT("Asset Path: %s"), *AssetPath);
		// ---- Name
		JSONDict->SetStringField(TEXT("name"), AssetData.AssetName.ToString());
		// ---- Type
		JSONDict->SetStringField(TEXT("type"), "mesh");
		// ---- Vertex Color Channels   // TODO?
		JSONDict->SetNumberField(TEXT("vertex_color_channels"), 1);
		{
			SCOPE_CYCLE_COUNTER(STAT_ExtractAssetImportInfo);
			// ---- Source Files
			TOptional<FAssetImportInfo> ImportInfo = FAssetSourceFilenameCache::ExtractAssetImportInfo(AssetData);
			if (ImportInfo.IsSet())
			{
				FString SourcePaths;
				for (const auto& File : ImportInfo->SourceFiles)
				{
					SourcePaths += " " + File.RelativeFilename;
				}
				// JSONDict->SetStringField(TEXT("source_path"), SourcePaths);  // TODO: non english characters prevent json from being written :( 
				//UE_LOG(LogPromethean, Display, TEXT("Source Assets: %s"), *SourcePaths);
			}
		}
		
		// ---- Thumbnail Image -----
		TArray<FString> FileList;
		OutputPath.ParseIntoArray(FileList, TEXT("."), true);  // need to make sure we don't repeat the name second time after the '.' i.e. --Props--SM_Chair.SM_Chair.bmp
		const FString ImagePath = FString::Printf(TEXT("%s.bmp"), *FileList[0]);  // need to add bmp extension to force CreateBitmap not ue4 add numbers at the end
		JSONDict->SetStringField(TEXT("thumbnail"), ImagePath);
		
		FString ObjectFullName = AssetData.GetFullName();
		TArray<FName> ObjectFullNames = { *ObjectFullName };
		FThumbnailMap LoadedThumbnails;  // out tmap of <name:thumbnail>
		if (ThumbnailTools::ConditionallyLoadThumbnailsForObjects(ObjectFullNames, LoadedThumbnails))
		{			
			FObjectThumbnail* ObjectThumbnail = LoadedThumbnails.Find(*ObjectFullName);
		 			
			if (ObjectThumbnail && !ObjectThumbnail->IsEmpty())
			{
				//UE_LOG(LogPromethean, Display, TEXT("Thumbnail Found. Size: %i"), ObjectThumbnail->GetCompressedDataSize());			
				SaveThumbnail(ObjectThumbnail, ImagePath);  // will add extension at the end
			}
			else
			{
				UE_LOG(LogPromethean, Warning, TEXT("ObjectThumbnail not valid for %s!"), *ObjectFullName);
			}
		}
	}
	return JSONDict;
}

TSharedPtr<FJsonObject> getMaterialAssetData(const FAssetData& AssetData, const bool bPerformHeavyOperations)
{
	TSharedPtr<FJsonObject> JSONDict = MakeShared<FJsonObject>();
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 1
	if (AssetData.AssetClassPath.GetAssetName().IsEqual("Material"))
#else
	if (AssetData.AssetClass.IsEqual("Material"))
#endif
	{		
		// ---- Unique ID - Name
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 1
		const FString AssetPath = AssetData.GetSoftObjectPath().ToString();
#else
		const FString AssetPath = AssetData.ObjectPath.ToString();
#endif
		JSONDict->SetStringField(TEXT("path"), AssetPath);
		UE_LOG(LogPromethean, Display, TEXT("Asset Path: %s"), *AssetPath);
		// ---- Name
		JSONDict->SetStringField(TEXT("name"), AssetData.AssetName.ToString());
		// ---- Type
		JSONDict->SetStringField(TEXT("type"), "material");
		// ---- Is Instance
		JSONDict->SetNumberField(TEXT("is_instance"), 0);
		// ---- Instruction Counts
		JSONDict->SetNumberField(TEXT("instructions_static"), 0);  // set default value
		JSONDict->SetNumberField(TEXT("instructions_dynamic"), 0);  // set default value
		JSONDict->SetNumberField(TEXT("instructions_vertex"), 0);  // set default value

		FString FileName = AssetPath;
		FileName = FileName.Replace(TEXT("/"), TEXT("--"));
		FString OutputPath = OutFolderPath + "/thumbnails/" + FileName;
		
		if (bPerformHeavyOperations)
		{
			// turn AssetData to UMaterial
			UMaterial* MaterialInstance = Cast<UMaterial>(AssetData.GetAsset());
			// High-res Thumbnail
			GenerateHighResolutionThumbnail(MaterialInstance);  // if couldn't load - generate
			// ---- Parent Material
			JSONDict->SetStringField(TEXT("parent_path"), MaterialInstance->GetBaseMaterial()->GetPathName());
			UE_LOG(LogPromethean, Display, TEXT("Parent Name: %s"), *MaterialInstance->GetBaseMaterial()->GetPathName());

			// ---- Number of textures
			TArray<UTexture*> MaterialTextures;
			//MaterialInstance->GetUsedTextures(MaterialTextures, EMaterialQualityLevel::Num, true, GMaxRHIFeatureLevel, true);
			MaterialInstance->GetUsedTextures(MaterialTextures, EMaterialQualityLevel::Num, true, ERHIFeatureLevel::SM5, true);
			JSONDict->SetNumberField(TEXT("texture_count"), MaterialTextures.Num());
			UE_LOG(LogPromethean, Display, TEXT("Textures: %i"), MaterialTextures.Num());
			// ---- Texture Paths
			TArray<TSharedPtr<FJsonValue>> TexturePathArray;
			for (UTexture* Texture : MaterialTextures)
			{
				UE_LOG(LogPromethean, Display, TEXT("Texture Used: %s"), *Texture->GetPathName());  // gets material slots on the static mesh
				TexturePathArray.Add(MakeShareable(new FJsonValueString(Texture->GetPathName())));
			}
			JSONDict->SetArrayField(TEXT("texture_paths"), TexturePathArray);
			// ---- Two Sided
			JSONDict->SetNumberField(TEXT("is_two_sided"), MaterialInstance->TwoSided);
			UE_LOG(LogPromethean, Display, TEXT("Two Sided: %i"), MaterialInstance->TwoSided);
			// ---- Masked
			JSONDict->SetNumberField(TEXT("is_masked"), MaterialInstance->IsMasked());
			UE_LOG(LogPromethean, Display, TEXT("Masked: %i"), MaterialInstance->IsMasked());
			
			// ---- Material Functions Used
			auto MaterialFunctions = GetMaterialFunctionDependencies(*MaterialInstance);
			TArray<TSharedPtr<FJsonValue>> MaterialFunctionPaths;
			for (auto& Function : MaterialFunctions)
			{
				UE_LOG(LogPromethean, Display, TEXT("Material Function Used: %s"), *Function->GetPathName());
				MaterialFunctionPaths.Add(MakeShareable(new FJsonValueString(Function->GetPathName())));
			}
			JSONDict->SetArrayField(TEXT("material_functions"), MaterialFunctionPaths);

#if ENGINE_MAJOR_VERSION == 4  && ENGINE_MINOR_VERSION > 19  // MaterialStatsCommon missing on 4.19 so compiling out for now
			FShaderStatsInfo OutInfo;  // WARNING: defined the structure locally since global definition is private. might change
			FMaterialResource* Resource = MaterialInstance->GetMaterialResource(GMaxRHIFeatureLevel);
			FMaterialStatsUtils::ExtractMatertialStatsInfo(OutInfo, Resource);
			for (auto& Elem : OutInfo.ShaderInstructionCount)
			{
				int32 intValue = FCString::Atoi(*Elem.Value.StrDescription);
				switch (Elem.Key)  // key is a shader type enum
				{
				case ERepresentativeShader::StationarySurface:
					JSONDict->SetNumberField(TEXT("instructions_static"), intValue);
					UE_LOG(LogPromethean, Display, TEXT("INSTRUCTION COUNT Static %s"), *Elem.Value.StrDescription);
					break;
				case ERepresentativeShader::DynamicallyLitObject:
					JSONDict->SetNumberField(TEXT("instructions_dynamic"), intValue);
					UE_LOG(LogPromethean, Display, TEXT("INSTRUCTION COUNT Dynamic %s"), *Elem.Value.StrDescription);
					break;
				case ERepresentativeShader::FirstVertexShader:
					JSONDict->SetNumberField(TEXT("instructions_vertex"), intValue);
					UE_LOG(LogPromethean, Display, TEXT("INSTRUCTION COUNT Vertex Shader %s"), *Elem.Value.StrDescription);
					break;
				};  // UE_LOG(LogPromethean, Display, TEXT("SAMPLER COUNT : %s"), *OutInfo.SamplersCount.StrDescription);  // alternate way to get textures					
			}
#endif
		}

		// ---- Thumbnail Image -----
		TArray<FString> FileList;
		OutputPath.ParseIntoArray(FileList, TEXT("."), true);  // need to make sure we don't repeat the name second time after the '.' i.e. --Props--SM_Chair.SM_Chair.bmp
		const FString ImagePath = FString::Printf(TEXT("%s.bmp"), *FileList[0]);  // need to add bmp extension to force CreateBitmap not ue4 add numbers at the end
		JSONDict->SetStringField(TEXT("thumbnail"), ImagePath);
		
		FString ObjectFullName = AssetData.GetFullName();
		TArray<FName> ObjectFullNames = { *ObjectFullName };
		FThumbnailMap LoadedThumbnails;  // out tmap of <name:thumbnail>
		if (ThumbnailTools::ConditionallyLoadThumbnailsForObjects(ObjectFullNames, LoadedThumbnails))
		{			
			FObjectThumbnail* ObjectThumbnail = LoadedThumbnails.Find(*ObjectFullName);
		 			
			if (ObjectThumbnail && !ObjectThumbnail->IsEmpty())
			{
				//UE_LOG(LogPromethean, Display, TEXT("Thumbnail Found. Size: %i"), ObjectThumbnail->GetCompressedDataSize());			
				SaveThumbnail(ObjectThumbnail, ImagePath);  // will add extension at the end
			}
			else
			{
				UE_LOG(LogPromethean, Warning, TEXT("ObjectThumbnail not valid for %s!"), *ObjectFullName);
			}
		}
	}
	return JSONDict;
}

TSharedPtr<FJsonObject> getMaterialInstanceAssetData(const FAssetData& AssetData, const bool bPerformHeavyOperations)
{
	TSharedPtr<FJsonObject> JSONDict = MakeShared<FJsonObject>();
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 1
	if (AssetData.AssetClassPath.GetAssetName().IsEqual("MaterialInstanceConstant"))
#else
	if (AssetData.AssetClass.IsEqual("MaterialInstanceConstant"))
#endif
	{
		// ---- Unique ID - Name
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 1
		const FString AssetPath = AssetData.GetSoftObjectPath().ToString();
#else
		const FString AssetPath = AssetData.ObjectPath.ToString();
#endif
		JSONDict->SetStringField(TEXT("path"), AssetPath);
		UE_LOG(LogPromethean, Display, TEXT("Asset Path: %s"), *AssetPath);
		// ---- Name		
		JSONDict->SetStringField(TEXT("name"), AssetData.AssetName.ToString());
		// ---- Type
		JSONDict->SetStringField(TEXT("type"), "material");
		// ---- Is Instance
		JSONDict->SetNumberField(TEXT("is_instance"), 1);
		// ---- Instruction Counts
		JSONDict->SetNumberField(TEXT("instructions_static"), 0);  // set default value
		JSONDict->SetNumberField(TEXT("instructions_dynamic"), 0);  // set default value
		JSONDict->SetNumberField(TEXT("instructions_vertex"), 0);  // set default value

		FString FileName = AssetPath;
		FileName = FileName.Replace(TEXT("/"), TEXT("--"));
		FString OutputPath = OutFolderPath + "/thumbnails/" + FileName;

		if (bPerformHeavyOperations)
		{
			// turn AssetData to UMaterialInstanceConstant
			UMaterialInstanceConstant* MaterialInstance = Cast<UMaterialInstanceConstant>(AssetData.GetAsset());
			GenerateHighResolutionThumbnail(MaterialInstance);
			// ---- Parent Material				
			FString ParentPath = "";
			const UMaterial* ParentMaterial = MaterialInstance->GetBaseMaterial();
			if (IsValid(ParentMaterial))  // need to check if material exists first - otherwise hard crash
				ParentPath = ParentMaterial->GetPathName();
			else
				UE_LOG(LogPromethean, Display, TEXT("Warning! Can't find a valid parent for this MaterialInstance - it might be corrupted"));
			JSONDict->SetStringField(TEXT("parent_path"), ParentPath);
			UE_LOG(LogPromethean, Display, TEXT("Parent Name: %s"), *ParentPath);

			// ---- Number of textures					
			TArray<UTexture*> MaterialTextures;
			MaterialInstance->GetUsedTextures(MaterialTextures, EMaterialQualityLevel::Num, true, GMaxRHIFeatureLevel, true);
			JSONDict->SetNumberField(TEXT("texture_count"), MaterialTextures.Num());
			UE_LOG(LogPromethean, Display, TEXT("Textures: %i"), MaterialTextures.Num());
			// ---- Texture Paths
			TArray<TSharedPtr<FJsonValue>> TexturePathArray;
			FString TexturePath = "";
			for (UTexture* Texture : MaterialTextures)
			{
				TexturePath = Texture->GetFullName();
				UE_LOG(LogPromethean, Display, TEXT("Texture Used: %s"), *TexturePath);  // gets material slots on the static mesh
				TexturePathArray.Add(MakeShareable(new FJsonValueString(TexturePath)));
			}
			JSONDict->SetArrayField(TEXT("texture_paths"), TexturePathArray);
			// ---- Two Sided
			JSONDict->SetNumberField(TEXT("is_two_sided"), MaterialInstance->TwoSided);
			UE_LOG(LogPromethean, Display, TEXT("Two Sided: %i"), MaterialInstance->TwoSided);
			// ---- Masked
			JSONDict->SetNumberField(TEXT("is_masked"), MaterialInstance->IsMasked());
			UE_LOG(LogPromethean, Display, TEXT("Masked: %i"), MaterialInstance->IsMasked());

#if ENGINE_MAJOR_VERSION >= 4  && ENGINE_MINOR_VERSION > 19  // MaterialStatsCommon missing on 4.19 so compiling out for now
			FShaderStatsInfo OutInfo;  // WARNING: defined the structure locally since global definition is private. might change
			FMaterialResource* Resource = MaterialInstance->GetMaterialResource(GMaxRHIFeatureLevel);  // 
			if (Resource == nullptr)
			{
				UE_LOG(LogPromethean, Display, TEXT("!Warning! Material resource is not valid. Asset could be corrupted."));
			}
			else
			{
				FMaterialStatsUtils::ExtractMatertialStatsInfo(OutInfo, Resource);
				for (auto& Elem : OutInfo.ShaderInstructionCount)  // Elem = TMap<ERepresentativeShader, FContent>
					{
					UE_LOG(LogPromethean, Display, TEXT("Getting Shader Instruction Count..."));  // without this 4.23 seems to inexplicably crash
					
					int32 intValue = FCString::Atoi(*Elem.Value.StrDescription);
					
					switch (Elem.Key)  // key is a shader type enum
					{
					case ERepresentativeShader::StationarySurface:
						JSONDict->SetNumberField(TEXT("instructions_static"), intValue);
						UE_LOG(LogPromethean, Display, TEXT("INSTRUCTION COUNT Static %s"), *Elem.Value.StrDescription);
						break;
					case ERepresentativeShader::DynamicallyLitObject:
						JSONDict->SetNumberField(TEXT("instructions_dynamic"), intValue);
						UE_LOG(LogPromethean, Display, TEXT("INSTRUCTION COUNT Dynamic %s"), *Elem.Value.StrDescription);
						break;
					case ERepresentativeShader::FirstVertexShader:
						JSONDict->SetNumberField(TEXT("instructions_vertex"), intValue);
						UE_LOG(LogPromethean, Display, TEXT("INSTRUCTION COUNT Vertex Shader %s"), *Elem.Value.StrDescription);
						break;
					};  // UE_LOG(LogPromethean, Display, TEXT("SAMPLER COUNT : %s"), *OutInfo.SamplersCount.StrDescription);  // alternate way to get textures					
					
					}
			}
#endif
		}
		
		// ---- Thumbnail Image -----
		TArray<FString> FileList;
		OutputPath.ParseIntoArray(FileList, TEXT("."), true);  // need to make sure we don't repeat the name second time after the '.' i.e. --Props--SM_Chair.SM_Chair.bmp
		const FString ImagePath = FString::Printf(TEXT("%s.bmp"), *FileList[0]);  // need to add bmp extension to force CreateBitmap not ue4 add numbers at the end
		JSONDict->SetStringField(TEXT("thumbnail"), ImagePath);
		
		FString ObjectFullName = AssetData.GetFullName();
		TArray<FName> ObjectFullNames = { *ObjectFullName };
		FThumbnailMap LoadedThumbnails;  // out tmap of <name:thumbnail>
		if (ThumbnailTools::ConditionallyLoadThumbnailsForObjects(ObjectFullNames, LoadedThumbnails))
		{			
			FObjectThumbnail* ObjectThumbnail = LoadedThumbnails.Find(*ObjectFullName);
		 			
			if (ObjectThumbnail && !ObjectThumbnail->IsEmpty())
			{
				//UE_LOG(LogPromethean, Display, TEXT("Thumbnail Found. Size: %i"), ObjectThumbnail->GetCompressedDataSize());			
				SaveThumbnail(ObjectThumbnail, ImagePath);  // will add extension at the end
			}
			else
			{
				UE_LOG(LogPromethean, Warning, TEXT("ObjectThumbnail not valid for %s!"), *ObjectFullName);
			}
		}
	}
	return JSONDict;
}

TSharedPtr<FJsonObject> getTextureAssetData(const FAssetData& AssetData, const bool bPerformHeavyOperations)
{
	TSharedPtr<FJsonObject> JSONDict = MakeShared<FJsonObject>();
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 1
	if (AssetData.AssetClassPath.GetAssetName().IsEqual("Texture2D"))
#else
	if (AssetData.AssetClass.IsEqual("Texture2D"))
#endif
	{		
		for (const auto& TagAndValuePair : AssetData.TagsAndValues)
		{	// tags include: Format, AssetImportData, SRGB, CompressionSettings, Filter, NeverStream, LODGroup, AddressY, AddressX, Dimensions
			// UE_LOG(LogPromethean, Display, TEXT("Tag NameValue Pair: %s : %s"), *TagAndValuePair.Key.ToString(), *TagAndValuePair.Value);
			if (TagAndValuePair.Key.ToString() == "Dimensions")
			{
				// ---- Texture Size
				TArray<FString> DimensionsList;				
				TagAndValuePair.Value.GetValue().ParseIntoArray(DimensionsList, TEXT("x"), true);  // value is "512x512"
				if (DimensionsList.Num() == 2)  // make sure we split correctly into 2 items. unreal crashes if index is out of range for whatever unlikely reason
				{
					int width = FCString::Atoi(*DimensionsList[0]);
					int height = FCString::Atoi(*DimensionsList[1]);
					JSONDict->SetNumberField(TEXT("width"), width);
					JSONDict->SetNumberField(TEXT("height"), height);
					UE_LOG(LogPromethean, Display, TEXT("Width: %i"), width);
					UE_LOG(LogPromethean, Display, TEXT("Height: %i"), height);
				}
			}
			else if (TagAndValuePair.Key.ToString() == "HasAlphaChannel")
			{
				// ---- Masked or Not
				int has_alpha_int_bool = 0;
				if (TagAndValuePair.Value.GetValue() == "True")
					has_alpha_int_bool = 1;
				JSONDict->SetNumberField(TEXT("has_alpha"), has_alpha_int_bool);
				UE_LOG(LogPromethean, Display, TEXT("Has Alpha: %i"), has_alpha_int_bool);
			}
		}		
		// ---- Unique ID - Name		
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 1
		const FString AssetPath = AssetData.GetSoftObjectPath().ToString();
#else
		const FString AssetPath = AssetData.ObjectPath.ToString();
#endif
		JSONDict->SetStringField(TEXT("path"), AssetPath);
		UE_LOG(LogPromethean, Display, TEXT("Asset Path: %s"), *AssetPath);
		// ---- Name		
		JSONDict->SetStringField(TEXT("name"), AssetData.AssetName.ToString());
		// ---- Type
		JSONDict->SetStringField(TEXT("type"), "texture");

		FString FileName = AssetPath;
		FileName = FileName.Replace(TEXT("/"), TEXT("--"));
		FString OutputPath = OutFolderPath + "/thumbnails/" + FileName;
		
		// --- Get source files paths of the asset
		TOptional<FAssetImportInfo> ImportInfo = FAssetSourceFilenameCache::ExtractAssetImportInfo(AssetData);
		if (ImportInfo.IsSet())
		{
			FString SourcePaths;
			for (const auto& File : ImportInfo->SourceFiles)
			{
				SourcePaths += " " + File.RelativeFilename;
			}
			JSONDict->SetStringField(TEXT("source_path"), SourcePaths);
			UE_LOG(LogPromethean, Display, TEXT("Source Assets: %s"), *SourcePaths);
		}

		// Get source file path of the asset
		JSONDict->SetStringField(TEXT("source_path"), GetJSONImportSourceDataFromAsset<UTexture>(AssetData.GetAsset()->GetPathName()));
		
		if (bPerformHeavyOperations)
		{
			// turn AssetData to UMaterialInstanceConstant
			UTexture2D* Texture2D = Cast<UTexture2D>(AssetData.GetAsset());
			GenerateHighResolutionThumbnail(Texture2D);
		}
		
		// ---- Thumbnail Image -----
		TArray<FString> FileList;
		OutputPath.ParseIntoArray(FileList, TEXT("."), true);  // need to make sure we don't repeat the name second time after the '.' i.e. --Props--SM_Chair.SM_Chair.bmp
		const FString ImagePath = FString::Printf(TEXT("%s.bmp"), *FileList[0]);  // need to add bmp extension to force CreateBitmap not ue4 add numbers at the end
		JSONDict->SetStringField(TEXT("thumbnail"), ImagePath);
		
		FString ObjectFullName = AssetData.GetFullName();
		TArray<FName> ObjectFullNames = { *ObjectFullName };
		FThumbnailMap LoadedThumbnails;  // out tmap of <name:thumbnail>
		if (ThumbnailTools::ConditionallyLoadThumbnailsForObjects(ObjectFullNames, LoadedThumbnails))
		{			
			FObjectThumbnail* ObjectThumbnail = LoadedThumbnails.Find(*ObjectFullName);
		 			
			if (ObjectThumbnail && !ObjectThumbnail->IsEmpty())
			{
				//UE_LOG(LogPromethean, Display, TEXT("Thumbnail Found. Size: %i"), ObjectThumbnail->GetCompressedDataSize());			
				SaveThumbnail(ObjectThumbnail, ImagePath);  // will add extension at the end
			}
			else
			{
				UE_LOG(LogPromethean, Warning, TEXT("ObjectThumbnail not valid for %s!"), *ObjectFullName);
			}
		}
	}
	return JSONDict;
}

TSharedPtr<FJsonObject> getLevelAssetData(const FAssetData& AssetData, const bool bPerformHeavyOperations)
{
	TSharedPtr<FJsonObject> JSONDict = MakeShared<FJsonObject>();
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 1
	if (AssetData.AssetClassPath.GetAssetName().IsEqual("World"))
#else
	if (AssetData.AssetClass.IsEqual("World"))
#endif
	{
		// ---- Thumbnail - do thumbnail sooner so it has more time to save
		
		// ---- Unique ID - Name		
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 1
		const FString AssetPath = AssetData.GetSoftObjectPath().ToString();
#else
		const FString AssetPath = AssetData.ObjectPath.ToString();
#endif
		JSONDict->SetStringField(TEXT("path"), AssetPath);
		
		FString FileName = AssetPath;
		FileName = FileName.Replace(TEXT("/"), TEXT("--"));
		FString OutputPath = OutFolderPath + "/thumbnails/" + FileName + ".png";  // needs to be bmp as expected by standalone promethean app
		CaptureScreenshot(OutputPath);
		JSONDict->SetStringField(TEXT("thumbnail"), OutputPath);
		UE_LOG(LogPromethean, Display, TEXT("Asset Path: %s"), *AssetPath);
		
		// ---- Name		
		JSONDict->SetStringField(TEXT("name"), AssetData.AssetName.ToString());
		// ---- Type
		JSONDict->SetStringField(TEXT("type"), "level");

		if (bPerformHeavyOperations)
		{
			// ---- Open Level, get assets
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 1
			OpenLevel(AssetData.GetSoftObjectPath().ToString());
#else
			OpenLevel(AssetData.ObjectPath.ToString());
#endif
			// ---- Get All Asset Paths
			TArray<FString> AssetPathArray = GetCurrentLevelArtAssets();
			TArray<TSharedPtr<FJsonValue>> AssetPathJsonArray;
			for (FString Path : AssetPathArray)
				AssetPathJsonArray.Add(MakeShared<FJsonValueString>(Path));
			JSONDict->SetArrayField(TEXT("asset_paths"), AssetPathJsonArray);
		}
	}
	return JSONDict;
}

TArray<FString> GetCurrentLevelArtAssets()
{
	UWorld* EditorWorld = GetEditorWorld();  // comes from PromehteanGeneric
	TArray<FString> OutPathArray;
	TArray<AActor*> AllValidActors = GetWorldValidActors(EditorWorld);
	for (AActor* CurrentLearnActor : AllValidActors)
	{
		USceneComponent* SceneComponent = GetValidSceneComponent(CurrentLearnActor);
		// --- skip if actor has no valid static mesh
		if (SceneComponent == nullptr)
			continue;

		auto StaticMeshComponent = Cast<UStaticMeshComponent>(SceneComponent);
		if (StaticMeshComponent != nullptr) {
			UStaticMesh* StaticMesh = StaticMeshComponent->GetStaticMesh();
			// --- skip empty static meshes which are most likely there to signify semantic groups
			if (StaticMesh->GetPathName() == FString("None"))
				continue;
			// --- art asset path
			OutPathArray.Add(StaticMesh->GetPathName());
		}
	}
	return OutPathArray;
}

TArray<UStaticMeshComponent*> GetStaticMeshComponents(UBlueprint* blueprint)
{   // https://answers.unrealengine.com/questions/140647/get-default-object-for-blueprint-class.html
	TArray<UStaticMeshComponent*> ActorStaticMeshComponents;
	
	if (blueprint && blueprint->SimpleConstructionScript)
	{
		for (auto scsnode : blueprint->SimpleConstructionScript->GetAllNodes())
		{
			if (scsnode)
			{
				if (auto itemComponent = Cast<UStaticMeshComponent>(scsnode->ComponentTemplate))
				{
					ActorStaticMeshComponents.AddUnique(itemComponent);
				}
			}
		}
	}

	return ActorStaticMeshComponents;
}

TSharedPtr<FJsonObject> getBlueprintAssetData(const FAssetData& AssetData, const bool bPerformHeavyOperations)
{
	TSharedPtr<FJsonObject> JSONDict = MakeShared<FJsonObject>();
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 1
	if (AssetData.AssetClassPath.GetAssetName().IsEqual("Blueprint"))
#else
	if (AssetData.AssetClass.IsEqual("Blueprint"))
#endif
	{
		// AssetData.PrintAssetData();
		// ---- Blueprint Flag
		JSONDict->SetBoolField(TEXT("blueprint"), true);
		// ---- Unique ID - Name
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 1
		const FString AssetPath = AssetData.GetSoftObjectPath().ToString();
#else
		const FString AssetPath = AssetData.ObjectPath.ToString();
#endif
		JSONDict->SetStringField(TEXT("path"), AssetPath);
		UE_LOG(LogPromethean, Display, TEXT("Asset Path: %s"), *AssetPath);

		FString FileName = AssetPath;
		FileName = FileName.Replace(TEXT("/"), TEXT("--"));
		FString OutputPath = OutFolderPath + "/thumbnails/" + FileName;  // define path globally because it's reused for triangle too
		
		// ---- Name
		JSONDict->SetStringField(TEXT("name"), AssetData.AssetName.ToString());
		// ---- Type
		JSONDict->SetStringField(TEXT("type"), "mesh");
		// ---- Vertex Color Channels   // TODO?
		JSONDict->SetNumberField(TEXT("vertex_color_channels"), 1);
		// ---- Source Files
		TOptional<FAssetImportInfo> ImportInfo = FAssetSourceFilenameCache::ExtractAssetImportInfo(AssetData);
		if (ImportInfo.IsSet())
		{
			FString SourcePaths;
			for (const auto& File : ImportInfo->SourceFiles)
			{
				SourcePaths += " " + File.RelativeFilename;
			}
			// JSONDict->SetStringField(TEXT("source_path"), SourcePaths);  // TODO: non english characters prevent json from being written :( 
			UE_LOG(LogPromethean, Display, TEXT("Source Assets: %s"), *SourcePaths);
		}

		if (bPerformHeavyOperations)
		{
			// ---- WARNING! Have to load the actual BP here.
			UBlueprint* BlueprintAsset = Cast<UBlueprint>(AssetData.GetAsset()); 
			// ---- High Res Thumbs
			GenerateHighResolutionThumbnail(BlueprintAsset);
			
			TArray<UStaticMeshComponent*> ActorStaticMeshComponents = GetStaticMeshComponents(BlueprintAsset);
			UE_LOG(LogPromethean, Display, TEXT("Blueprint static mesh components: %i"), ActorStaticMeshComponents.Num());
			// get unique static meshes
			TArray<UStaticMesh*> StaticMeshes;
			for (auto StaticMeshComponent : ActorStaticMeshComponents)
			{
				UStaticMesh* StaticMesh = StaticMeshComponent->GetStaticMesh();
				StaticMeshes.AddUnique(StaticMesh);
			}
			// get attributes
			TArray<int32> NumLods;
			TArray<int32> NumFaces;
			int32 NumVerts = 0;
			int32 NumUVSets = 0;
			int32 NumTris = 0;		
			for (auto StaticMesh : StaticMeshes)
			{
				NumLods.Add(StaticMesh->GetNumLODs());
				NumVerts += StaticMesh->GetNumVertices(0);  // TrianglesCount += StaticMesh->RenderData->LODResources[0].GetNumVertices();
				NumTris += StaticMesh->GetRenderData()->LODResources[0].GetNumTriangles();
				NumUVSets += StaticMesh->GetNumUVChannels(0);
			}

			// ---- Face Count
			UE_LOG(LogPromethean, Display, TEXT("Faces: %i"), NumTris);
			JSONDict->SetNumberField(TEXT("face_count"), NumTris);
			// ---- Vertex Count
			UE_LOG(LogPromethean, Display, TEXT("Vertices: %i"), NumVerts);
			JSONDict->SetNumberField(TEXT("vertex_count"), NumVerts);
			// ---- UV Channels
			UE_LOG(LogPromethean, Display, TEXT("UV Channels: %i"), NumUVSets);
			JSONDict->SetNumberField(TEXT("uv_sets"), NumUVSets);
			// ---- Num Lods
			UE_LOG(LogPromethean, Display, TEXT("Lod Count: %i"), NumLods.Max());
			JSONDict->SetNumberField(TEXT("lod_count"), NumLods.Max());
			// ---- Material Paths
			
			TArray<FString> MaterialPathStringArray;
			for (auto StaticMesh : StaticMeshes)
			{			
				for (FStaticMaterial StaticMaterial : StaticMesh->GetStaticMaterials())
				{
					UE_LOG(LogPromethean, Display, TEXT("Material Used: %s"), *StaticMaterial.MaterialInterface->GetPathName());  // gets material slots on the static mesh
					MaterialPathStringArray.AddUnique(StaticMaterial.MaterialInterface->GetPathName());
				}			
			}
			TArray<TSharedPtr<FJsonValue>> MaterialPathArray;
			for (auto UniqueMatPath : MaterialPathStringArray)
			{
				MaterialPathArray.AddUnique(MakeShareable(new FJsonValueString(UniqueMatPath)));
			}
			JSONDict->SetArrayField(TEXT("material_paths"), MaterialPathArray);
			// ---- Material Count				
			UE_LOG(LogPromethean, Display, TEXT("Material Count: %i"), MaterialPathArray.Num());
			JSONDict->SetNumberField(TEXT("material_count"), MaterialPathArray.Num());

			
			// ---- Size
			FBox CombinedBoundingBox;
			for (auto StaticMesh : StaticMeshes)
			{
				CombinedBoundingBox += StaticMesh->GetBoundingBox();
				
				// ---- Raw Triangles
				TArray<FVector> Vertexes = GetMeshTrianglePositionsFromLibraryStaticMesh(StaticMesh);
				if (Vertexes.Num() != 0)
				{
					FString VertexString = FVectorArrayToJsonString(Vertexes, TEXT("verts"));
					SaveToFile(OutputPath / StaticMesh->GetName(), VertexString);  // no file format is currently reserved for vertexes
					JSONDict->SetStringField(TEXT("verts"), OutputPath / StaticMesh->GetName());
				}
			}
			UE_LOG(LogPromethean, Display, TEXT("Size: %s"), *CombinedBoundingBox.GetSize().ToString());
			FVector Size = CombinedBoundingBox.GetSize();
			TArray<TSharedPtr<FJsonValue>> MeshScaleArray;
			MeshScaleArray.Add(MakeShareable(new FJsonValueNumber(Size.X)));
			MeshScaleArray.Add(MakeShareable(new FJsonValueNumber(Size.Y)));
			MeshScaleArray.Add(MakeShareable(new FJsonValueNumber(Size.Z)));
			JSONDict->SetArrayField(TEXT("bounding_box"), MeshScaleArray);
			
			// ---- Pivot Offset
			FVector PivotOffset = CombinedBoundingBox.GetCenter();
			PivotOffset[2] = CombinedBoundingBox.Min[2];  // min height
			TArray<TSharedPtr<FJsonValue>> MeshPivotOffsetArray;
			MeshPivotOffsetArray.Add(MakeShareable(new FJsonValueNumber(-PivotOffset.X)));
			MeshPivotOffsetArray.Add(MakeShareable(new FJsonValueNumber(-PivotOffset.Y)));
			MeshPivotOffsetArray.Add(MakeShareable(new FJsonValueNumber(-PivotOffset.Z)));
			JSONDict->SetArrayField(TEXT("pivot_offset"), MeshPivotOffsetArray);
		}
		
		// ---- Thumbnail Image -----
		TArray<FString> FileList;
		OutputPath.ParseIntoArray(FileList, TEXT("."), true);  // need to make sure we don't repeat the name second time after the '.' i.e. --Props--SM_Chair.SM_Chair.bmp
		const FString ImagePath = FString::Printf(TEXT("%s.bmp"), *FileList[0]);  // need to add bmp extension to force CreateBitmap not ue4 add numbers at the end
		JSONDict->SetStringField(TEXT("thumbnail"), ImagePath);
		
		FString ObjectFullName = AssetData.GetFullName();
		TArray<FName> ObjectFullNames = { *ObjectFullName };
		FThumbnailMap LoadedThumbnails;  // out tmap of <name:thumbnail>
		if (ThumbnailTools::ConditionallyLoadThumbnailsForObjects(ObjectFullNames, LoadedThumbnails))
		{			
			FObjectThumbnail* ObjectThumbnail = LoadedThumbnails.Find(*ObjectFullName);
		 			
			if (ObjectThumbnail && !ObjectThumbnail->IsEmpty())
			{
				//UE_LOG(LogPromethean, Display, TEXT("Thumbnail Found. Size: %i"), ObjectThumbnail->GetCompressedDataSize());			
				SaveThumbnail(ObjectThumbnail, ImagePath);  // will add extension at the end
			}
			else
			{
				UE_LOG(LogPromethean, Warning, TEXT("ObjectThumbnail not valid for %s!"), *ObjectFullName);
			}
		}
	}
	return JSONDict;
}

TSharedPtr<FJsonObject> getUnknownAssetData()
{
	TSharedPtr<FJsonObject> JSONDict = MakeShared<FJsonObject>();	
	JSONDict->SetStringField(TEXT("error"), "Unsupported data type");
	return JSONDict;
}

