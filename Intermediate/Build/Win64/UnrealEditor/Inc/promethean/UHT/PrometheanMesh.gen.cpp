// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "promethean/Public/PrometheanMesh.h"
#include "ProceduralMeshComponent.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodePrometheanMesh() {}
// Cross Module References
	COREUOBJECT_API UScriptStruct* Z_Construct_UScriptStruct_FLinearColor();
	COREUOBJECT_API UScriptStruct* Z_Construct_UScriptStruct_FVector();
	COREUOBJECT_API UScriptStruct* Z_Construct_UScriptStruct_FVector2D();
	ENGINE_API UClass* Z_Construct_UClass_AActor();
	PROCEDURALMESHCOMPONENT_API UClass* Z_Construct_UClass_UProceduralMeshComponent_NoRegister();
	PROCEDURALMESHCOMPONENT_API UScriptStruct* Z_Construct_UScriptStruct_FProcMeshTangent();
	PROMETHEAN_API UClass* Z_Construct_UClass_APrometheanMeshActor();
	PROMETHEAN_API UClass* Z_Construct_UClass_APrometheanMeshActor_NoRegister();
	PROMETHEAN_API UScriptStruct* Z_Construct_UScriptStruct_FPrometheanMeshParameters();
	UPackage* Z_Construct_UPackage__Script_promethean();
// End Cross Module References
	static FStructRegistrationInfo Z_Registration_Info_UScriptStruct_PrometheanMeshParameters;
class UScriptStruct* FPrometheanMeshParameters::StaticStruct()
{
	if (!Z_Registration_Info_UScriptStruct_PrometheanMeshParameters.OuterSingleton)
	{
		Z_Registration_Info_UScriptStruct_PrometheanMeshParameters.OuterSingleton = GetStaticStruct(Z_Construct_UScriptStruct_FPrometheanMeshParameters, (UObject*)Z_Construct_UPackage__Script_promethean(), TEXT("PrometheanMeshParameters"));
	}
	return Z_Registration_Info_UScriptStruct_PrometheanMeshParameters.OuterSingleton;
}
template<> PROMETHEAN_API UScriptStruct* StaticStruct<FPrometheanMeshParameters>()
{
	return FPrometheanMeshParameters::StaticStruct();
}
	struct Z_Construct_UScriptStruct_FPrometheanMeshParameters_Statics
	{
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Struct_MetaDataParams[];
#endif
		static void* NewStructOps();
		static const UECodeGen_Private::FStructPropertyParams NewProp_VertLocations_Inner;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_VertLocations_MetaData[];
#endif
		static const UECodeGen_Private::FArrayPropertyParams NewProp_VertLocations;
		static const UECodeGen_Private::FIntPropertyParams NewProp_TriangleIDs_Inner;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_TriangleIDs_MetaData[];
#endif
		static const UECodeGen_Private::FArrayPropertyParams NewProp_TriangleIDs;
		static const UECodeGen_Private::FStructPropertyParams NewProp_Normals_Inner;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_Normals_MetaData[];
#endif
		static const UECodeGen_Private::FArrayPropertyParams NewProp_Normals;
		static const UECodeGen_Private::FStructPropertyParams NewProp_Tangents_Inner;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_Tangents_MetaData[];
#endif
		static const UECodeGen_Private::FArrayPropertyParams NewProp_Tangents;
		static const UECodeGen_Private::FStructPropertyParams NewProp_UVs_Inner;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_UVs_MetaData[];
#endif
		static const UECodeGen_Private::FArrayPropertyParams NewProp_UVs;
		static const UECodeGen_Private::FStructPropertyParams NewProp_VertexColors_Inner;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_VertexColors_MetaData[];
#endif
		static const UECodeGen_Private::FArrayPropertyParams NewProp_VertexColors;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
		static const UECodeGen_Private::FStructParams ReturnStructParams;
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FPrometheanMeshParameters_Statics::Struct_MetaDataParams[] = {
		{ "BlueprintType", "true" },
		{ "ModuleRelativePath", "Public/PrometheanMesh.h" },
	};
#endif
	void* Z_Construct_UScriptStruct_FPrometheanMeshParameters_Statics::NewStructOps()
	{
		return (UScriptStruct::ICppStructOps*)new UScriptStruct::TCppStructOps<FPrometheanMeshParameters>();
	}
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UScriptStruct_FPrometheanMeshParameters_Statics::NewProp_VertLocations_Inner = { "VertLocations", nullptr, (EPropertyFlags)0x0000000000020000, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, Z_Construct_UScriptStruct_FVector, METADATA_PARAMS(0, nullptr) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FPrometheanMeshParameters_Statics::NewProp_VertLocations_MetaData[] = {
		{ "Category", "Promethean" },
		{ "ModuleRelativePath", "Public/PrometheanMesh.h" },
	};
#endif
	const UECodeGen_Private::FArrayPropertyParams Z_Construct_UScriptStruct_FPrometheanMeshParameters_Statics::NewProp_VertLocations = { "VertLocations", nullptr, (EPropertyFlags)0x0010000000020005, UECodeGen_Private::EPropertyGenFlags::Array, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FPrometheanMeshParameters, VertLocations), EArrayPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FPrometheanMeshParameters_Statics::NewProp_VertLocations_MetaData), Z_Construct_UScriptStruct_FPrometheanMeshParameters_Statics::NewProp_VertLocations_MetaData) };
	const UECodeGen_Private::FIntPropertyParams Z_Construct_UScriptStruct_FPrometheanMeshParameters_Statics::NewProp_TriangleIDs_Inner = { "TriangleIDs", nullptr, (EPropertyFlags)0x0000000000020000, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, METADATA_PARAMS(0, nullptr) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FPrometheanMeshParameters_Statics::NewProp_TriangleIDs_MetaData[] = {
		{ "Category", "Promethean" },
		{ "ModuleRelativePath", "Public/PrometheanMesh.h" },
	};
#endif
	const UECodeGen_Private::FArrayPropertyParams Z_Construct_UScriptStruct_FPrometheanMeshParameters_Statics::NewProp_TriangleIDs = { "TriangleIDs", nullptr, (EPropertyFlags)0x0010000000020005, UECodeGen_Private::EPropertyGenFlags::Array, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FPrometheanMeshParameters, TriangleIDs), EArrayPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FPrometheanMeshParameters_Statics::NewProp_TriangleIDs_MetaData), Z_Construct_UScriptStruct_FPrometheanMeshParameters_Statics::NewProp_TriangleIDs_MetaData) };
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UScriptStruct_FPrometheanMeshParameters_Statics::NewProp_Normals_Inner = { "Normals", nullptr, (EPropertyFlags)0x0000000000020000, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, Z_Construct_UScriptStruct_FVector, METADATA_PARAMS(0, nullptr) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FPrometheanMeshParameters_Statics::NewProp_Normals_MetaData[] = {
		{ "Category", "Promethean" },
		{ "Comment", "// Index buffer indicating which vertices make up each triangle, must be a length of Vertices array length multiplied by 3.\n" },
		{ "ModuleRelativePath", "Public/PrometheanMesh.h" },
		{ "ToolTip", "Index buffer indicating which vertices make up each triangle, must be a length of Vertices array length multiplied by 3." },
	};
#endif
	const UECodeGen_Private::FArrayPropertyParams Z_Construct_UScriptStruct_FPrometheanMeshParameters_Statics::NewProp_Normals = { "Normals", nullptr, (EPropertyFlags)0x0010000000020005, UECodeGen_Private::EPropertyGenFlags::Array, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FPrometheanMeshParameters, Normals), EArrayPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FPrometheanMeshParameters_Statics::NewProp_Normals_MetaData), Z_Construct_UScriptStruct_FPrometheanMeshParameters_Statics::NewProp_Normals_MetaData) };
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UScriptStruct_FPrometheanMeshParameters_Statics::NewProp_Tangents_Inner = { "Tangents", nullptr, (EPropertyFlags)0x0000000000020000, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, Z_Construct_UScriptStruct_FProcMeshTangent, METADATA_PARAMS(0, nullptr) }; // 2099358922
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FPrometheanMeshParameters_Statics::NewProp_Tangents_MetaData[] = {
		{ "Category", "Promethean" },
		{ "Comment", "// must be same length as Vertices array.\n" },
		{ "ModuleRelativePath", "Public/PrometheanMesh.h" },
		{ "ToolTip", "must be same length as Vertices array." },
	};
#endif
	const UECodeGen_Private::FArrayPropertyParams Z_Construct_UScriptStruct_FPrometheanMeshParameters_Statics::NewProp_Tangents = { "Tangents", nullptr, (EPropertyFlags)0x0010000000020005, UECodeGen_Private::EPropertyGenFlags::Array, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FPrometheanMeshParameters, Tangents), EArrayPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FPrometheanMeshParameters_Statics::NewProp_Tangents_MetaData), Z_Construct_UScriptStruct_FPrometheanMeshParameters_Statics::NewProp_Tangents_MetaData) }; // 2099358922
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UScriptStruct_FPrometheanMeshParameters_Statics::NewProp_UVs_Inner = { "UVs", nullptr, (EPropertyFlags)0x0000000000020000, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, Z_Construct_UScriptStruct_FVector2D, METADATA_PARAMS(0, nullptr) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FPrometheanMeshParameters_Statics::NewProp_UVs_MetaData[] = {
		{ "Category", "Promethean" },
		{ "Comment", "// must be same length as Vertices array.\n" },
		{ "ModuleRelativePath", "Public/PrometheanMesh.h" },
		{ "ToolTip", "must be same length as Vertices array." },
	};
#endif
	const UECodeGen_Private::FArrayPropertyParams Z_Construct_UScriptStruct_FPrometheanMeshParameters_Statics::NewProp_UVs = { "UVs", nullptr, (EPropertyFlags)0x0010000000020005, UECodeGen_Private::EPropertyGenFlags::Array, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FPrometheanMeshParameters, UVs), EArrayPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FPrometheanMeshParameters_Statics::NewProp_UVs_MetaData), Z_Construct_UScriptStruct_FPrometheanMeshParameters_Statics::NewProp_UVs_MetaData) };
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UScriptStruct_FPrometheanMeshParameters_Statics::NewProp_VertexColors_Inner = { "VertexColors", nullptr, (EPropertyFlags)0x0000000000020000, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, Z_Construct_UScriptStruct_FLinearColor, METADATA_PARAMS(0, nullptr) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FPrometheanMeshParameters_Statics::NewProp_VertexColors_MetaData[] = {
		{ "Category", "Promethean" },
		{ "Comment", "// must be same length as Vertices array.\n" },
		{ "ModuleRelativePath", "Public/PrometheanMesh.h" },
		{ "ToolTip", "must be same length as Vertices array." },
	};
#endif
	const UECodeGen_Private::FArrayPropertyParams Z_Construct_UScriptStruct_FPrometheanMeshParameters_Statics::NewProp_VertexColors = { "VertexColors", nullptr, (EPropertyFlags)0x0010000000020005, UECodeGen_Private::EPropertyGenFlags::Array, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FPrometheanMeshParameters, VertexColors), EArrayPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FPrometheanMeshParameters_Statics::NewProp_VertexColors_MetaData), Z_Construct_UScriptStruct_FPrometheanMeshParameters_Statics::NewProp_VertexColors_MetaData) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UScriptStruct_FPrometheanMeshParameters_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FPrometheanMeshParameters_Statics::NewProp_VertLocations_Inner,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FPrometheanMeshParameters_Statics::NewProp_VertLocations,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FPrometheanMeshParameters_Statics::NewProp_TriangleIDs_Inner,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FPrometheanMeshParameters_Statics::NewProp_TriangleIDs,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FPrometheanMeshParameters_Statics::NewProp_Normals_Inner,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FPrometheanMeshParameters_Statics::NewProp_Normals,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FPrometheanMeshParameters_Statics::NewProp_Tangents_Inner,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FPrometheanMeshParameters_Statics::NewProp_Tangents,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FPrometheanMeshParameters_Statics::NewProp_UVs_Inner,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FPrometheanMeshParameters_Statics::NewProp_UVs,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FPrometheanMeshParameters_Statics::NewProp_VertexColors_Inner,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FPrometheanMeshParameters_Statics::NewProp_VertexColors,
	};
	const UECodeGen_Private::FStructParams Z_Construct_UScriptStruct_FPrometheanMeshParameters_Statics::ReturnStructParams = {
		(UObject* (*)())Z_Construct_UPackage__Script_promethean,
		nullptr,
		&NewStructOps,
		"PrometheanMeshParameters",
		Z_Construct_UScriptStruct_FPrometheanMeshParameters_Statics::PropPointers,
		UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FPrometheanMeshParameters_Statics::PropPointers),
		sizeof(FPrometheanMeshParameters),
		alignof(FPrometheanMeshParameters),
		RF_Public|RF_Transient|RF_MarkAsNative,
		EStructFlags(0x00000001),
		METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FPrometheanMeshParameters_Statics::Struct_MetaDataParams), Z_Construct_UScriptStruct_FPrometheanMeshParameters_Statics::Struct_MetaDataParams)
	};
	static_assert(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FPrometheanMeshParameters_Statics::PropPointers) < 2048);
	UScriptStruct* Z_Construct_UScriptStruct_FPrometheanMeshParameters()
	{
		if (!Z_Registration_Info_UScriptStruct_PrometheanMeshParameters.InnerSingleton)
		{
			UECodeGen_Private::ConstructUScriptStruct(Z_Registration_Info_UScriptStruct_PrometheanMeshParameters.InnerSingleton, Z_Construct_UScriptStruct_FPrometheanMeshParameters_Statics::ReturnStructParams);
		}
		return Z_Registration_Info_UScriptStruct_PrometheanMeshParameters.InnerSingleton;
	}
	DEFINE_FUNCTION(APrometheanMeshActor::execTryToSetMaterial)
	{
		P_GET_PROPERTY(FStrProperty,Z_Param_MaterialPath);
		P_GET_PROPERTY(FIntProperty,Z_Param_MaterialIndex);
		P_FINISH;
		P_NATIVE_BEGIN;
		*(bool*)Z_Param__Result=P_THIS->TryToSetMaterial(Z_Param_MaterialPath,Z_Param_MaterialIndex);
		P_NATIVE_END;
	}
	DEFINE_FUNCTION(APrometheanMeshActor::execRegeneratePrometheanMesh)
	{
		P_FINISH;
		P_NATIVE_BEGIN;
		P_THIS->RegeneratePrometheanMesh();
		P_NATIVE_END;
	}
	DEFINE_FUNCTION(APrometheanMeshActor::execGenerateMeshes)
	{
		P_GET_PROPERTY(FStrProperty,Z_Param_JsonInputString);
		P_FINISH;
		P_NATIVE_BEGIN;
		*(FString*)Z_Param__Result=APrometheanMeshActor::GenerateMeshes(Z_Param_JsonInputString);
		P_NATIVE_END;
	}
	void APrometheanMeshActor::StaticRegisterNativesAPrometheanMeshActor()
	{
		UClass* Class = APrometheanMeshActor::StaticClass();
		static const FNameNativePtrPair Funcs[] = {
			{ "GenerateMeshes", &APrometheanMeshActor::execGenerateMeshes },
			{ "RegeneratePrometheanMesh", &APrometheanMeshActor::execRegeneratePrometheanMesh },
			{ "TryToSetMaterial", &APrometheanMeshActor::execTryToSetMaterial },
		};
		FNativeFunctionRegistrar::RegisterFunctions(Class, Funcs, UE_ARRAY_COUNT(Funcs));
	}
	struct Z_Construct_UFunction_APrometheanMeshActor_GenerateMeshes_Statics
	{
		struct PrometheanMeshActor_eventGenerateMeshes_Parms
		{
			FString JsonInputString;
			FString ReturnValue;
		};
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_JsonInputString_MetaData[];
#endif
		static const UECodeGen_Private::FStrPropertyParams NewProp_JsonInputString;
		static const UECodeGen_Private::FStrPropertyParams NewProp_ReturnValue;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_APrometheanMeshActor_GenerateMeshes_Statics::NewProp_JsonInputString_MetaData[] = {
		{ "NativeConst", "" },
	};
#endif
	const UECodeGen_Private::FStrPropertyParams Z_Construct_UFunction_APrometheanMeshActor_GenerateMeshes_Statics::NewProp_JsonInputString = { "JsonInputString", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Str, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(PrometheanMeshActor_eventGenerateMeshes_Parms, JsonInputString), METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_APrometheanMeshActor_GenerateMeshes_Statics::NewProp_JsonInputString_MetaData), Z_Construct_UFunction_APrometheanMeshActor_GenerateMeshes_Statics::NewProp_JsonInputString_MetaData) };
	const UECodeGen_Private::FStrPropertyParams Z_Construct_UFunction_APrometheanMeshActor_GenerateMeshes_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Str, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(PrometheanMeshActor_eventGenerateMeshes_Parms, ReturnValue), METADATA_PARAMS(0, nullptr) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_APrometheanMeshActor_GenerateMeshes_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_APrometheanMeshActor_GenerateMeshes_Statics::NewProp_JsonInputString,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_APrometheanMeshActor_GenerateMeshes_Statics::NewProp_ReturnValue,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_APrometheanMeshActor_GenerateMeshes_Statics::Function_MetaDataParams[] = {
		{ "ModuleRelativePath", "Public/PrometheanMesh.h" },
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_APrometheanMeshActor_GenerateMeshes_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_APrometheanMeshActor, nullptr, "GenerateMeshes", nullptr, nullptr, Z_Construct_UFunction_APrometheanMeshActor_GenerateMeshes_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_APrometheanMeshActor_GenerateMeshes_Statics::PropPointers), sizeof(Z_Construct_UFunction_APrometheanMeshActor_GenerateMeshes_Statics::PrometheanMeshActor_eventGenerateMeshes_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00022401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_APrometheanMeshActor_GenerateMeshes_Statics::Function_MetaDataParams), Z_Construct_UFunction_APrometheanMeshActor_GenerateMeshes_Statics::Function_MetaDataParams) };
	static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_APrometheanMeshActor_GenerateMeshes_Statics::PropPointers) < 2048);
	static_assert(sizeof(Z_Construct_UFunction_APrometheanMeshActor_GenerateMeshes_Statics::PrometheanMeshActor_eventGenerateMeshes_Parms) < MAX_uint16);
	UFunction* Z_Construct_UFunction_APrometheanMeshActor_GenerateMeshes()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_APrometheanMeshActor_GenerateMeshes_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	struct Z_Construct_UFunction_APrometheanMeshActor_RegeneratePrometheanMesh_Statics
	{
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_APrometheanMeshActor_RegeneratePrometheanMesh_Statics::Function_MetaDataParams[] = {
		{ "Category", "Promethean|Mesh" },
		{ "ModuleRelativePath", "Public/PrometheanMesh.h" },
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_APrometheanMeshActor_RegeneratePrometheanMesh_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_APrometheanMeshActor, nullptr, "RegeneratePrometheanMesh", nullptr, nullptr, nullptr, 0, 0, RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_APrometheanMeshActor_RegeneratePrometheanMesh_Statics::Function_MetaDataParams), Z_Construct_UFunction_APrometheanMeshActor_RegeneratePrometheanMesh_Statics::Function_MetaDataParams) };
	UFunction* Z_Construct_UFunction_APrometheanMeshActor_RegeneratePrometheanMesh()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_APrometheanMeshActor_RegeneratePrometheanMesh_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	struct Z_Construct_UFunction_APrometheanMeshActor_TryToSetMaterial_Statics
	{
		struct PrometheanMeshActor_eventTryToSetMaterial_Parms
		{
			FString MaterialPath;
			int32 MaterialIndex;
			bool ReturnValue;
		};
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_MaterialPath_MetaData[];
#endif
		static const UECodeGen_Private::FStrPropertyParams NewProp_MaterialPath;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_MaterialIndex_MetaData[];
#endif
		static const UECodeGen_Private::FIntPropertyParams NewProp_MaterialIndex;
		static void NewProp_ReturnValue_SetBit(void* Obj);
		static const UECodeGen_Private::FBoolPropertyParams NewProp_ReturnValue;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_APrometheanMeshActor_TryToSetMaterial_Statics::NewProp_MaterialPath_MetaData[] = {
		{ "NativeConst", "" },
	};
#endif
	const UECodeGen_Private::FStrPropertyParams Z_Construct_UFunction_APrometheanMeshActor_TryToSetMaterial_Statics::NewProp_MaterialPath = { "MaterialPath", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Str, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(PrometheanMeshActor_eventTryToSetMaterial_Parms, MaterialPath), METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_APrometheanMeshActor_TryToSetMaterial_Statics::NewProp_MaterialPath_MetaData), Z_Construct_UFunction_APrometheanMeshActor_TryToSetMaterial_Statics::NewProp_MaterialPath_MetaData) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_APrometheanMeshActor_TryToSetMaterial_Statics::NewProp_MaterialIndex_MetaData[] = {
		{ "NativeConst", "" },
	};
#endif
	const UECodeGen_Private::FIntPropertyParams Z_Construct_UFunction_APrometheanMeshActor_TryToSetMaterial_Statics::NewProp_MaterialIndex = { "MaterialIndex", nullptr, (EPropertyFlags)0x0010000000000082, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(PrometheanMeshActor_eventTryToSetMaterial_Parms, MaterialIndex), METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_APrometheanMeshActor_TryToSetMaterial_Statics::NewProp_MaterialIndex_MetaData), Z_Construct_UFunction_APrometheanMeshActor_TryToSetMaterial_Statics::NewProp_MaterialIndex_MetaData) };
	void Z_Construct_UFunction_APrometheanMeshActor_TryToSetMaterial_Statics::NewProp_ReturnValue_SetBit(void* Obj)
	{
		((PrometheanMeshActor_eventTryToSetMaterial_Parms*)Obj)->ReturnValue = 1;
	}
	const UECodeGen_Private::FBoolPropertyParams Z_Construct_UFunction_APrometheanMeshActor_TryToSetMaterial_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(PrometheanMeshActor_eventTryToSetMaterial_Parms), &Z_Construct_UFunction_APrometheanMeshActor_TryToSetMaterial_Statics::NewProp_ReturnValue_SetBit, METADATA_PARAMS(0, nullptr) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_APrometheanMeshActor_TryToSetMaterial_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_APrometheanMeshActor_TryToSetMaterial_Statics::NewProp_MaterialPath,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_APrometheanMeshActor_TryToSetMaterial_Statics::NewProp_MaterialIndex,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_APrometheanMeshActor_TryToSetMaterial_Statics::NewProp_ReturnValue,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_APrometheanMeshActor_TryToSetMaterial_Statics::Function_MetaDataParams[] = {
		{ "Category", "Promethean|Mesh" },
		{ "ModuleRelativePath", "Public/PrometheanMesh.h" },
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_APrometheanMeshActor_TryToSetMaterial_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_APrometheanMeshActor, nullptr, "TryToSetMaterial", nullptr, nullptr, Z_Construct_UFunction_APrometheanMeshActor_TryToSetMaterial_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_APrometheanMeshActor_TryToSetMaterial_Statics::PropPointers), sizeof(Z_Construct_UFunction_APrometheanMeshActor_TryToSetMaterial_Statics::PrometheanMeshActor_eventTryToSetMaterial_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_APrometheanMeshActor_TryToSetMaterial_Statics::Function_MetaDataParams), Z_Construct_UFunction_APrometheanMeshActor_TryToSetMaterial_Statics::Function_MetaDataParams) };
	static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_APrometheanMeshActor_TryToSetMaterial_Statics::PropPointers) < 2048);
	static_assert(sizeof(Z_Construct_UFunction_APrometheanMeshActor_TryToSetMaterial_Statics::PrometheanMeshActor_eventTryToSetMaterial_Parms) < MAX_uint16);
	UFunction* Z_Construct_UFunction_APrometheanMeshActor_TryToSetMaterial()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_APrometheanMeshActor_TryToSetMaterial_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(APrometheanMeshActor);
	UClass* Z_Construct_UClass_APrometheanMeshActor_NoRegister()
	{
		return APrometheanMeshActor::StaticClass();
	}
	struct Z_Construct_UClass_APrometheanMeshActor_Statics
	{
		static UObject* (*const DependentSingletons[])();
		static const FClassFunctionLinkInfo FuncInfo[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_GeneratedMeshParams_MetaData[];
#endif
		static const UECodeGen_Private::FStructPropertyParams NewProp_GeneratedMeshParams;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_GeneratedMesh_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPropertyParams NewProp_GeneratedMesh;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_APrometheanMeshActor_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_AActor,
		(UObject* (*)())Z_Construct_UPackage__Script_promethean,
	};
	static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_APrometheanMeshActor_Statics::DependentSingletons) < 16);
	const FClassFunctionLinkInfo Z_Construct_UClass_APrometheanMeshActor_Statics::FuncInfo[] = {
		{ &Z_Construct_UFunction_APrometheanMeshActor_GenerateMeshes, "GenerateMeshes" }, // 3775945384
		{ &Z_Construct_UFunction_APrometheanMeshActor_RegeneratePrometheanMesh, "RegeneratePrometheanMesh" }, // 2032913546
		{ &Z_Construct_UFunction_APrometheanMeshActor_TryToSetMaterial, "TryToSetMaterial" }, // 2158936053
	};
	static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_APrometheanMeshActor_Statics::FuncInfo) < 2048);
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_APrometheanMeshActor_Statics::Class_MetaDataParams[] = {
		{ "IncludePath", "PrometheanMesh.h" },
		{ "ModuleRelativePath", "Public/PrometheanMesh.h" },
	};
#endif
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_APrometheanMeshActor_Statics::NewProp_GeneratedMeshParams_MetaData[] = {
		{ "Category", "Promethean|Mesh" },
		{ "Comment", "/*  // TODO 4.24 replaced FMeshDescription for FStaticMeshAttributes - need to fix!\n\x09UFUNCTION(BlueprintCallable, Category = \"Promethean|Mesh\")\n\x09\x09void ConvertGeneratedMeshToStaticMesh();\n\x09*/" },
		{ "ModuleRelativePath", "Public/PrometheanMesh.h" },
		{ "ToolTip", "// TODO 4.24 replaced FMeshDescription for FStaticMeshAttributes - need to fix!\n     UFUNCTION(BlueprintCallable, Category = \"Promethean|Mesh\")\n             void ConvertGeneratedMeshToStaticMesh();" },
	};
#endif
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UClass_APrometheanMeshActor_Statics::NewProp_GeneratedMeshParams = { "GeneratedMeshParams", nullptr, (EPropertyFlags)0x0010000000000014, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(APrometheanMeshActor, GeneratedMeshParams), Z_Construct_UScriptStruct_FPrometheanMeshParameters, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_APrometheanMeshActor_Statics::NewProp_GeneratedMeshParams_MetaData), Z_Construct_UClass_APrometheanMeshActor_Statics::NewProp_GeneratedMeshParams_MetaData) }; // 959015509
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_APrometheanMeshActor_Statics::NewProp_GeneratedMesh_MetaData[] = {
		{ "Category", "Promethean|Mesh" },
		{ "EditInline", "true" },
		{ "ModuleRelativePath", "Public/PrometheanMesh.h" },
	};
#endif
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_APrometheanMeshActor_Statics::NewProp_GeneratedMesh = { "GeneratedMesh", nullptr, (EPropertyFlags)0x00100000000a001d, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(APrometheanMeshActor, GeneratedMesh), Z_Construct_UClass_UProceduralMeshComponent_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_APrometheanMeshActor_Statics::NewProp_GeneratedMesh_MetaData), Z_Construct_UClass_APrometheanMeshActor_Statics::NewProp_GeneratedMesh_MetaData) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_APrometheanMeshActor_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_APrometheanMeshActor_Statics::NewProp_GeneratedMeshParams,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_APrometheanMeshActor_Statics::NewProp_GeneratedMesh,
	};
	const FCppClassTypeInfoStatic Z_Construct_UClass_APrometheanMeshActor_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<APrometheanMeshActor>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_APrometheanMeshActor_Statics::ClassParams = {
		&APrometheanMeshActor::StaticClass,
		"Engine",
		&StaticCppClassTypeInfo,
		DependentSingletons,
		FuncInfo,
		Z_Construct_UClass_APrometheanMeshActor_Statics::PropPointers,
		nullptr,
		UE_ARRAY_COUNT(DependentSingletons),
		UE_ARRAY_COUNT(FuncInfo),
		UE_ARRAY_COUNT(Z_Construct_UClass_APrometheanMeshActor_Statics::PropPointers),
		0,
		0x009000A4u,
		METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_APrometheanMeshActor_Statics::Class_MetaDataParams), Z_Construct_UClass_APrometheanMeshActor_Statics::Class_MetaDataParams)
	};
	static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_APrometheanMeshActor_Statics::PropPointers) < 2048);
	UClass* Z_Construct_UClass_APrometheanMeshActor()
	{
		if (!Z_Registration_Info_UClass_APrometheanMeshActor.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_APrometheanMeshActor.OuterSingleton, Z_Construct_UClass_APrometheanMeshActor_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_APrometheanMeshActor.OuterSingleton;
	}
	template<> PROMETHEAN_API UClass* StaticClass<APrometheanMeshActor>()
	{
		return APrometheanMeshActor::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(APrometheanMeshActor);
	APrometheanMeshActor::~APrometheanMeshActor() {}
	struct Z_CompiledInDeferFile_FID_PrometheanAI_promethean_ue_Packaged_5_3_promethean_HostProject_Plugins_promethean_Source_promethean_Public_PrometheanMesh_h_Statics
	{
		static const FStructRegisterCompiledInInfo ScriptStructInfo[];
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FStructRegisterCompiledInInfo Z_CompiledInDeferFile_FID_PrometheanAI_promethean_ue_Packaged_5_3_promethean_HostProject_Plugins_promethean_Source_promethean_Public_PrometheanMesh_h_Statics::ScriptStructInfo[] = {
		{ FPrometheanMeshParameters::StaticStruct, Z_Construct_UScriptStruct_FPrometheanMeshParameters_Statics::NewStructOps, TEXT("PrometheanMeshParameters"), &Z_Registration_Info_UScriptStruct_PrometheanMeshParameters, CONSTRUCT_RELOAD_VERSION_INFO(FStructReloadVersionInfo, sizeof(FPrometheanMeshParameters), 959015509U) },
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_PrometheanAI_promethean_ue_Packaged_5_3_promethean_HostProject_Plugins_promethean_Source_promethean_Public_PrometheanMesh_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_APrometheanMeshActor, APrometheanMeshActor::StaticClass, TEXT("APrometheanMeshActor"), &Z_Registration_Info_UClass_APrometheanMeshActor, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(APrometheanMeshActor), 3460684738U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_PrometheanAI_promethean_ue_Packaged_5_3_promethean_HostProject_Plugins_promethean_Source_promethean_Public_PrometheanMesh_h_1090884392(TEXT("/Script/promethean"),
		Z_CompiledInDeferFile_FID_PrometheanAI_promethean_ue_Packaged_5_3_promethean_HostProject_Plugins_promethean_Source_promethean_Public_PrometheanMesh_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_PrometheanAI_promethean_ue_Packaged_5_3_promethean_HostProject_Plugins_promethean_Source_promethean_Public_PrometheanMesh_h_Statics::ClassInfo),
		Z_CompiledInDeferFile_FID_PrometheanAI_promethean_ue_Packaged_5_3_promethean_HostProject_Plugins_promethean_Source_promethean_Public_PrometheanMesh_h_Statics::ScriptStructInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_PrometheanAI_promethean_ue_Packaged_5_3_promethean_HostProject_Plugins_promethean_Source_promethean_Public_PrometheanMesh_h_Statics::ScriptStructInfo),
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
