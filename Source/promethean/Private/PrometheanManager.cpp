// Fill out your copyright notice in the Description page of Project Settings.

#include "PrometheanManager.h"  // matching header has to go first
#include "PrometheanLibraryData.h"
#include "PrometheanLearning.h"
#include "PrometheanGeneric.h"
#include "PrometheanMesh.h"
#include "promethean.h"

#include "FileHelpers.h"  // save current level

#include <string>  // cstring

//#include "LevelEditor.h"  
#include "Editor.h" // GEditor
#include "Editor/EditorEngine.h"
#include "Interfaces/IPv4/IPv4Endpoint.h"
#include "SEditorViewport.h" // snap to surface toggle
#include "SocketSubsystem.h"

// VR Editor
#include "EditorWorldExtension.h"
#include "VREditorMode.h"
#include "ViewportWorldInteraction.h"
#include "Common/TcpSocketBuilder.h"

// --------------------------------------------------------------------
// --- GLOBAL CONNECTION VARIABLES
// --------------------------------------------------------------------

FSocket* ListenerSocket;
FSocket* ConnectionSocket;
FIPv4Endpoint RemoteAddressForConnection;
// Timer Handles to check for socket messages
FTimerHandle TCPSocketListenerTimerHandle;
FTimerHandle TCPConnectionListenerTimerHandle;
// Port Globals
const int32 PrometheanUE4Port = 62;
const int32 PrometheanBrowserPort = 1312;
const int32 PrometheanCommandLinePort = 1313;
const int32 PrometheanAssetDataPort = 1315;
// Command Queue
TArray<FString> GlobalCommandStrArray;
bool isCommandExecuting = false;
const float GLOBAL_SERVER_HEARTBEAT = 0.001f;

// --------------------------------------------------------------------
// --- VR Mode Hotkey Hack
// --------------------------------------------------------------------
FKey VrListenModePressedKey;
bool VrListenModeKeyPressed = false;


static void ViewportWorldInteraction_HandleInput(class FEditorViewportClient* ViewportClient, const FKey Key, const EInputEvent Event, bool& bWasHandled)
{
	// UE_LOG(LogPromethean, Log, TEXT("============ Promethean Subscribed Input Event Started"));
	FKey VRControllerKey = EKeys::C; // EKeys::MotionController_Right_FaceButton2  // TODO: fix 4.25 support
	if (Key == EKeys::C || Key == VRControllerKey)
	{
		// UE_LOG(LogPromethean, Log, TEXT("Correct input key!"));
		if (Event == EInputEvent::IE_Pressed && !VrListenModeKeyPressed)
		{
			VrListenModeKeyPressed = true;
			VrListenModePressedKey = Key;  // unused currently because we can get stuck in case of a crash
			UE_LOG(LogPromethean, Log, TEXT("Start voice capture from VR controller"));
			SendSingleTCPMessage(PrometheanCommandLinePort, "toggle_voice_capture");

		}
		else if (VrListenModeKeyPressed && Event == EInputEvent::IE_Released)
		{
			VrListenModeKeyPressed = false;
			SendSingleTCPMessage(PrometheanCommandLinePort, "toggle_voice_capture");
			UE_LOG(LogPromethean, Log, TEXT("Stop voice capture from VR controller"));
		}
	}
	else if (Key == VRControllerKey)
	{
		if (Event == EInputEvent::IE_Pressed && !VrListenModeKeyPressed)
		{
			SendSingleTCPMessage(PrometheanCommandLinePort, "reset_voice_capture");
			UE_LOG(LogPromethean, Log, TEXT("Reset voice capture from VR controller"));

		}
	}
}

void SubscribeToVRKeyEvents(UWorld* InWorld)
{
	UE_LOG(LogPromethean, Log, TEXT("============ Promethean Trying to Subscrbie to VR input events in case this is a valid VR world"));
	FTimerHandle TimerHandle;
	static int delay_sec = 1;
	InWorld->GetTimerManager().SetTimer(TimerHandle, [InWorld]()
		{
			// InWorld->GetTimerManager().ClearTimer(TimerHandle);  // if we set time to loop can clear it here
			// UE_LOG(LogPromethean, Log, TEXT("============ Promethean Trying to Subscrbie to Keyboard input events"));

			UViewportWorldInteraction* ViewportWorldInteraction = Cast<UViewportWorldInteraction>(GEditor->GetEditorWorldExtensionsManager()->GetEditorWorldExtensions(InWorld)->FindExtension(UViewportWorldInteraction::StaticClass()));
			if (ViewportWorldInteraction != nullptr)
			{
				ViewportWorldInteraction->OnHandleKeyInput().AddStatic(&ViewportWorldInteraction_HandleInput);
				UE_LOG(LogPromethean, Log, TEXT("============ Promethean Successfully Subscrbied to a VR World input events"));
			}
		}, delay_sec, false);  // last attribute is whether timer should loop
}

static void SubscribeToVRKeyEventsConsoleEntrypoint(const TArray<FString>& Args, UWorld* InWorld)
{
	SubscribeToVRKeyEvents(InWorld);
}

FAutoConsoleCommandWithWorldAndArgs PrometheanVRInputSubscribeCommand(
	TEXT("PMT.EnableVRInput"),
	TEXT("Editor VR mode would trigger a voice command in the standalone Promethean application"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(SubscribeToVRKeyEventsConsoleEntrypoint)
);


// --------------------------------------------------------------------
// --- Console Commands
// --------------------------------------------------------------------
static void PrometheanTestStatic(const TArray<FString>& Args, UWorld* InWorld)
{
	UE_LOG(LogPromethean, Display, TEXT("Promethean Test Command"));


	if (GEditor->PlayWorld != NULL)
	{
		/*
		auto SimWorldActor = GetActorByName<AActor>("Sphere_2", GEditor->PlayWorld);
		UE_LOG(LogPromethean, Display, TEXT("Dynamic Actor Name: %s"), *SimWorldActor->GetName());
		UE_LOG(LogPromethean, Display, TEXT("Dynamic Actor Name: %s"), *SimWorldActor->GetActorLocation().ToString());
		auto EditorActor = GetActorByName<AActor>("Sphere_2", GetEditorWorld());
		UE_LOG(LogPromethean, Display, TEXT("Editor Actor Name: %s"), *EditorActor->GetName());
		UE_LOG(LogPromethean, Display, TEXT("Editor Actor Name: %s"), *EditorActor->GetActorLocation().ToString());
		*/

		DisablePlayInEditor();
	}
	else
	{
		EnablePlayInEditor();
		//GEditor->RequestPlaySession(true, nullptr, false);
	}
	
	
	// GEditor->RequestEndPlayMap();  // kill play in editor session
	// GEditor->PlayWorld->End

	/*
	// set selected actor to be physically simulated
	USelection* SelectedActors = GEditor->GetSelectedActors();
	for (FSelectionIterator Iter(*SelectedActors); Iter; ++Iter)
	{
		AActor* Actor = Cast<AActor>(*Iter);		
		UE_LOG(LogPromethean, Display, TEXT("Selected Actor"));
		if (Actor)
		{
			UE_LOG(LogPromethean, Display, TEXT("Selected Actor Set Physics"));			
			UPrimitiveComponent* root = Cast<UPrimitiveComponent>(Actor->GetRootComponent());  // root component is USceneComponent* by default
			if (root)
			{
				root->SetSimulatePhysics(true);
			}
		}
	}
	// play in editor
	GEditor->PlayInEditor(GetEditorWorld(), true);
	*/


	// GEditor->StartPlayInEditorSession();  // TODO: not found in 4.24?
	// SubscribeToVRKeyEvents(InWorld);
	//TArray<AActor*> RenderedActors = GetRenderedValidActors();
	//FString NameString = ActorArrayToNamesString(RenderedActors);
	//UE_LOG(LogPromethean, Display, TEXT("StaticMeshActors on Screen: %i"), RenderedActors.Num());
	//UE_LOG(LogPromethean, Display, TEXT("StaticMeshActors on Screen: %s"), *NameString);


	// SendSingleTCPMessage(PrometheanCommandLinePort, "toggle_voice_capture");
	
	/*
	UWorld* EditorWorld = GetEditorWorld();	
	if (EditorWorld != nullptr)
	{
		// UE_LOG(LogPromethean, Display, TEXT("Editor Map: %s"), *EditorWorld->GetMapName());
		// PrometheanLearn(EditorWorld, "C:/Users/Anastaisya/Documents/PrometheanAI/AI/learning_cache.json"); // can't use GEditor->GetWorld() here - hard crash		
		// PrometheanLearn(InWorld, "C:/Users/Anastaisya/Documents/PrometheanAI/AI/learning_cache.json"); // ignoring InWorld to test ActorAgnostic setup
		// TCPSend(TEXT("TCP Message Test"));		
		UE_LOG(LogPromethean, Display, TEXT("Promethean Test Successful"));
	}
	*/
}

FAutoConsoleCommandWithWorldAndArgs PrometheanTestCommand(
	TEXT("PMT.Test"),
	TEXT("Debug test function"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(PrometheanTestStatic)
);


static void GenerateLearnDataStatic(const TArray<FString>& Args, UWorld* InWorld)
{
	UE_LOG(LogPromethean, Display, TEXT("Generating Learn Data..."));
	PrometheanLearn(InWorld, "C:/Users/Anastaisya/Documents/PrometheanAI/AI/learning_cache.json"); // can't use GEditor->GetWorld() here - hard crash
}

FAutoConsoleCommandWithWorldAndArgs SaveToFileCommand(
	TEXT("PMT.Learn"),
	TEXT("Saves the learning file for Promethean to pick up."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(GenerateLearnDataStatic)
);

static void GetAssetLibraryMeshDataStatic(const TArray<FString>& Args, UWorld* InWorld)
{
	UE_LOG(LogPromethean, Display, TEXT("Getting asset library mesh data..."));
	GetAssetLibraryData("StaticMesh");
}

FAutoConsoleCommandWithWorldAndArgs GetAssetLibraryMeshDataCommand(
	TEXT("PMT.GetAssetLibraryMeshData"),
	TEXT("Get Asset Library Mesh Data for display in Promethean Browser."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(GetAssetLibraryMeshDataStatic)
);

static void GetAssetLibraryMaterialDataStatic(const TArray<FString>& Args, UWorld* InWorld)
{
	UE_LOG(LogPromethean, Display, TEXT("Getting asset library Material data..."));
	GetAssetLibraryData("MaterialInstanceConstant");
	GetAssetLibraryData("Material");
}

FAutoConsoleCommandWithWorldAndArgs GetAssetLibraryMaterialDataCommand(
	TEXT("PMT.GetAssetLibraryMaterialData"),
	TEXT("Get Asset Library Material Data for display in Promethean Browser."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(GetAssetLibraryMaterialDataStatic)
);

static void OpenTCPConnectionStatic(const TArray<FString>& Args, UWorld* InWorld)
{	
	LaunchTCP();
}

FAutoConsoleCommandWithWorldAndArgs OpenTCPConnectionCommand(
	TEXT("PMT.Connect"),
	TEXT("Open a TCP connection for Promethean AI."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(OpenTCPConnectionStatic)
);

static void CloseTCPConnectionStatic(const TArray<FString>& Args, UWorld* InWorld)
{
	TCPCloseConnection();
}

FAutoConsoleCommandWithWorldAndArgs CloseTCPConnectionCommand(
	TEXT("PMT.Disconnect"),
	TEXT("Close TCP connection for Promethean AI."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(CloseTCPConnectionStatic)
);


// --------------------------------------------------------------------
// --- Parse TCP Commands
// --------------------------------------------------------------------

/*
	Takes TCPMessage, cuts out the command name and returns the rest of the message.
	Command name should be the first substring in the message, separated by space ' ' character.
	The rest is considered the parameters.
	Do not invoke more than once on the same message.
*/
static void ExtractCommandParameters(const FString& TCPMessage, FString& CommandNameOut, FString& CommandParameterStringOut) {
	int32 ParameterSectionIndex;
	if (!TCPMessage.FindChar(TEXT(' '), ParameterSectionIndex))
	{
		CommandNameOut = TCPMessage;
		return;
	}
	CommandParameterStringOut = TCPMessage.RightChop(ParameterSectionIndex + 1);
	CommandNameOut = TCPMessage.Left(ParameterSectionIndex);
}

// --------------------------------------------------------------------
// --- Execute TCP Commands
// --------------------------------------------------------------------

void ExecuteTCPCommand(FString MetaCommandStr)
{	
	UE_LOG(LogPromethean, Display, TEXT("Executing TCP Command:\n%s"), *MetaCommandStr);	
	TArray<FString> MetaCommandList;  // a command list with all the commands
	MetaCommandStr.ParseIntoArrayLines(MetaCommandList, true);  // cull empty is the second parameter
	UWorld* EditorWorld = GetEditorWorld(); //  this->GetWorld()
	if (EditorWorld == nullptr)
	{
		return;  // if not world to operate in. TODO: Is there a scenario where there is no world and promethean is still useful?
	}

	// Cache all hidden actors in the world to be ignored during raytracing
	// TODO: is this slow?
	HiddenActors.Empty();
    for (TActorIterator<AActor> It(EditorWorld); It; ++It)
    {
        AActor* Actor = *It;
        if (Actor && Actor->IsHiddenEd())
        {
            HiddenActors.Add(Actor);  // global variable defined in PrometheanGeneric.cpp
        }
    }
    UE_LOG(LogPromethean, Warning, TEXT("Number of Hidden Actors: %d"), HiddenActors.Num());

	GEditor->BeginTransaction(FText::FromString("PrometheanAI Command"));  // start undo, automatically detecs add and deletions, property updates need to be registered with ActorPtr->Modify()
	
	for (auto& CommandStr : MetaCommandList)
	{
		
		FString CommandName;
		FString CommandParameterString;
		TArray<FString> CommandParameterList;

		ExtractCommandParameters(CommandStr, CommandName, CommandParameterString);
		CommandParameterString.ParseIntoArray(CommandParameterList, TEXT(" "), true);

		if (CommandName == "scene_name")
		{
			FString SceneName = getSceneName(EditorWorld);
			UE_LOG(LogPromethean, Display, TEXT("Current Scene Name: %s"), *SceneName);
			TCPSend(SceneName);
		}
		else if (CommandName == "save_current_scene")
		{
			FEditorFileUtils::SaveCurrentLevel();
		}
		else if (CommandName == "learn" && CommandParameterList.Num() > 0)  // sending the filepath
		{
			if (CommandParameterList[0] == "selection")  // hard crash if sample index out of range so need to check
			{
				FString LearnPath = CommandStr.Replace(TEXT("learn selection "), TEXT(""));   // in case there are spaces in the path - get the whole str	
				FString SceneName = EditorWorld->GetName();
				PrometheanLearnSelection(SceneName, LearnPath);
			}
			else if (CommandParameterList[0] == "view")
			{
				FString LearnPath = CommandStr.Replace(TEXT("learn view "), TEXT(""));   // in case there are spaces in the path - get the whole str	
				FString SceneName = EditorWorld->GetName();
				PrometheanLearnView(SceneName, LearnPath);
			}
			else
			{
				FString LearnPath = CommandStr.Replace(TEXT("learn "), TEXT("")); // in case there are spaces in the path - get the whole str
				UE_LOG(LogPromethean, Display, TEXT("LearnPath: %s"), *LearnPath);
				PrometheanLearn(EditorWorld, LearnPath);
			}
		}
		else if (CommandName == "select" && CommandParameterList.Num() > 0)  // hard crash if sample index out of range so need to check
		{
			TArray<FString> SelectObjects;
			CommandParameterList[0].ParseIntoArray(SelectObjects, TEXT(","), true);  // object DCC names are separated by a comma
			SelectSceneActorsByName(SelectObjects);
		}
		else if (CommandName == "set_visible" && CommandParameterList.Num() > 0)  // hard crash if sample index out of range so need to check
		{
			TArray<FString> Objects;
			CommandParameterList[0].ParseIntoArray(Objects, TEXT(","), true);  // object DCC names are separated by a comma
			SetActorsVisibleByName(Objects, EditorWorld);
		}
		else if (CommandName == "set_hidden" && CommandParameterList.Num() > 0)  // hard crash if sample index out of range so need to check
		{
			TArray<FString> Objects;
			CommandParameterList[0].ParseIntoArray(Objects, TEXT(","), true);  // object DCC names are separated by a comma
			SetActorsHiddenByName(Objects, EditorWorld);
		}
		else if (CommandName == "clear_selection")
		{
			GEditor->SelectNone(true, true, true);  // deselect everything 
		}
		else if (CommandName == "focus" && CommandParameterList.Num() > 0)  // hard crash if sample index out of range so need to check
		{
			TArray<FString> FocusObjects;
			CommandParameterList[0].ParseIntoArray(FocusObjects, TEXT(","), true);  // object DCC names are separated by a comma
			FocusOnActorsByName(FocusObjects, EditorWorld);
		}
		else if (CommandName == "parent" && CommandParameterList.Num() > 0)  // "parent table,statue,statue1"
		{
			TArray<FString> ParentingObjectNames;
			CommandParameterList[0].ParseIntoArray(ParentingObjectNames, TEXT(","), true);  // object DCC names are separated by a comma.
			FString ParentName = ParentingObjectNames[0]; // first one is parent all the rest are children
			ParentingObjectNames.RemoveAt(0);  // remove parent leaving just the list of children
			ParentAllObjectsByName(ParentName, ParentingObjectNames, EditorWorld);
		}
		else if (CommandName == "get_parents" && CommandParameterList.Num() > 0)  // "parent table,statue,statue1"
		{
			TArray<FString> ParentingObjectNames;
			CommandParameterList[0].ParseIntoArray(ParentingObjectNames, TEXT(","), true);  // object DCC names are separated by a comma.			
			TArray<FString> ReturnNames = GetParentsForObjectsByName(ParentingObjectNames, EditorWorld);
			FString ReturnStr = StringArrayToString(ReturnNames);
			UE_LOG(LogPromethean, Display, TEXT("Parents of submitted objects in order: %s"), *ReturnStr);
			TCPSend(ReturnStr);
		}
		else if (CommandName == "unparent" && CommandParameterList.Num() > 0)  // "unparent table,statue,statue1"
		{
			TArray<FString> UnparentingObjectNames;
			CommandParameterList[0].ParseIntoArray(UnparentingObjectNames, TEXT(","), true);  // object DCC names are separated by a comma.
			UnparentObjectsByName(UnparentingObjectNames, EditorWorld);
		}
		else if (CommandName == "remove" && CommandParameterList.Num() > 0)
		{
			TArray<FString> removeObjects;
			CommandParameterList[0].ParseIntoArray(removeObjects, TEXT(","), true);  // object DCC names are separated by a comma
			RemoveActorsByName(removeObjects, EditorWorld);			
		}
		else if (CommandName == "remove_descendents" && CommandParameterList.Num() > 0)
		{
			TArray<FString> removeParentNames;
			CommandParameterList[0].ParseIntoArray(removeParentNames, TEXT(","), true);  // object DCC names are separated by a comma
			RemoveDescendentsFromActorsByName(removeParentNames, EditorWorld);
		}
		else if (CommandName == "remove_selected")
		{			
			RemoveSelectedActors();			
		}
		else if (CommandName == "transform" && CommandParameterList.Num() > 1)
		{
			TArray<FString> transformStrArray;
			CommandParameterList[0].ParseIntoArray(transformStrArray, TEXT(","), true);  // numbers are separated by a comma
			FTransform Transform = StringArrayToTransform(transformStrArray);
			if (Transform.ContainsNaN())
			{
				TArray<FString> ObjectNames;
				CommandParameterList[1].ParseIntoArray(ObjectNames, TEXT(","), true);  // object DCC names are separated by a comma
				TransformActorsByName(ObjectNames, Transform, EditorWorld);				
			}
			else
			{
				UE_LOG(LogPromethean, Display, TEXT("Not enough numbers in transform"));
			}
		}
		else if (CommandName == "add" && CommandParameterList.Num() > 0)
		{
			FString MeshPath = CommandParameterList[0];
			FString Name = "";
			FVector Location = FVector(0, 0, 0);
			FVector Rotation = FVector(0, 0, 0);
			FVector Scale = FVector(1, 1, 1);
			// getting optional arugments		
			if (CommandParameterList.Num() > 1)
			{
				Name = CommandParameterList[1];
			}
			TArray<FString> serviceStringArray;
			FVector ServiceVector;
			if (CommandParameterList.Num() > 2)  // get location
			{
				CommandParameterList[2].ParseIntoArray(serviceStringArray, TEXT(","), true);  // numbers are separated by a comma
				ServiceVector = StringArrayToVector(serviceStringArray);
				if (ServiceVector != NegativeInfinityVector)
				{
					Location = ServiceVector;
				}
			}
			if (CommandParameterList.Num() > 3)  // get rotation
			{
				CommandParameterList[3].ParseIntoArray(serviceStringArray, TEXT(","), true);  // numbers are separated by a comma
				ServiceVector = StringArrayToVector(serviceStringArray);
				if (ServiceVector != NegativeInfinityVector)
				{
					Rotation = ServiceVector;
				}
			}
			if (CommandParameterList.Num() > 4)  // get scale
			{
				CommandParameterList[4].ParseIntoArray(serviceStringArray, TEXT(","), true);  // numbers are separated by a comma
				ServiceVector = StringArrayToVector(serviceStringArray);
				if (ServiceVector != NegativeInfinityVector)
				{
					Scale = ServiceVector;
				}
			}

			addStaticMeshActor(EditorWorld, MeshPath, Name, Location, Rotation, Scale);
		}
		else if (CommandName == "add_mesh_on_selection" && CommandParameterList.Num() > 0)
		{
			// FString AssetPath = CommandParameterList[0];
			TArray<FString> AssetPaths;
			CommandParameterList[0].ParseIntoArray(AssetPaths, TEXT(","), true);  // object DCC names are separated by a comma
			addStaticMeshActorsOnSelection(AssetPaths, EditorWorld);
			//for(FString AssetPath: AssetPaths)
			//	addStaticMeshActorOnSelection(AssetPath, EditorWorld);
		}
		else if (CommandName == "add_objects" && CommandParameterList.Num() > 0)
		{
			FString ReturnJsonStr = addStaticMeshActors(EditorWorld, CommandParameterString);
			UE_LOG(LogPromethean, Display, TEXT("Add Objects return json string: %s"), *ReturnJsonStr);
			TCPSend(ReturnJsonStr);
		}
		else if (CommandName == "add_objects_from_triangles")
		{
			FString ReturnJsonStr = APrometheanMeshActor::GenerateMeshes(CommandParameterString);  // old unique DCC names matched with what UE4 can actually assign
			UE_LOG(LogPromethean, Display, TEXT("Add Objects from Triangles return json string: %s"), *ReturnJsonStr);
			TCPSend(ReturnJsonStr);
		}
		else if (CommandName == "convert_generated_mesh_to_static")
		{
			/*  // TODO 4.24 replaced FMeshDescription for FStaticMeshAttributes - need to fix!
			for (auto ObjectName : CommandParameterList)
			{
				UE_LOG(LogPromethean, Display, TEXT("Add Objects from Triangles return json string: %s"), *ObjectName);
				AActor* Actor = GetActorByName<AActor>(ObjectName, EditorWorld);
				if (Actor != nullptr)
				{
					auto GeneratedMeshActor = Cast<APrometheanMeshActor>(Actor);
					if (GeneratedMeshActor != nullptr)
					{
						GeneratedMeshActor->ConvertGeneratedMeshToStaticMesh();
					}
				}
			}
			*/
		}
		else if (CommandName == "add_group" && CommandParameterList.Num() > 0)
		{
			FString Name = CommandParameterList[0];
			FVector Location = FVector(0, 0, 0);
			FVector Rotation = FVector(0, 0, 0);
			FVector Scale = FVector(1, 1, 1);
			// getting optional arugments
			TArray<FString> serviceStringArray;
			FVector ServiceVector;
			if (CommandParameterList.Num() > 1)
			{
				CommandParameterList[1].ParseIntoArray(serviceStringArray, TEXT(","), true);  // numbers are separated by a comma
				ServiceVector = StringArrayToVector(serviceStringArray);
				if (ServiceVector != NegativeInfinityVector)
				{
					UE_LOG(LogPromethean, Display, TEXT("location: %s"), *ServiceVector.ToString());
					Location = ServiceVector;
				}
			}
			if (CommandParameterList.Num() > 2)
			{
				CommandParameterList[2].ParseIntoArray(serviceStringArray, TEXT(","), true);  // numbers are separated by a comma
				ServiceVector = StringArrayToVector(serviceStringArray);
				if (ServiceVector != NegativeInfinityVector)
				{
					Rotation = ServiceVector;
				}
			}
			if (CommandParameterList.Num() > 3)
			{
				CommandParameterList[3].ParseIntoArray(serviceStringArray, TEXT(","), true);  // numbers are separated by a comma
				ServiceVector = StringArrayToVector(serviceStringArray);
				if (ServiceVector != NegativeInfinityVector)
				{
					Scale = ServiceVector;
				}
			}
			addEmptyStaticMeshActor(EditorWorld, Name, Location, Rotation, Scale);
		}
		else if ((CommandName == "translate" || CommandName == "rotate" || CommandName == "scale" || CommandName == "translate_relative" || CommandName == "rotate_relative"  || CommandName == "scale_relative") && CommandParameterList.Num() > 1)
		{	// "scale 1.03,1.02,0.98 chair1,chair2,chair3"
			TArray<FString> sizeNumberStrArray;
			CommandParameterList[0].ParseIntoArray(sizeNumberStrArray, TEXT(","), true);  // numbers are separated by a comma
			FVector NewVector = StringArrayToVector(sizeNumberStrArray);
			if (NewVector != NegativeInfinityVector)
			{
				TArray<FString> ObjectNames;
				CommandParameterList[1].ParseIntoArray(ObjectNames, TEXT(","), true);  // object DCC names are separated by a comma
				if (CommandName == "scale")
				{
					ScaleActorsByName(ObjectNames, NewVector, EditorWorld);
				}
				else if (CommandName == "rotate")
				{
					RotateActorsByName(ObjectNames, NewVector, EditorWorld);
				}
				else if (CommandName == "translate")
				{
					TranslateActorsByName(ObjectNames, NewVector, EditorWorld);
				}
				else if (CommandName == "translate_relative")
				{
					RelativeTranslateActorsByName(ObjectNames, NewVector, EditorWorld);					
				}
				else if (CommandName == "rotate_relative")
				{
					RelativeRotateActorsByName(ObjectNames, NewVector, EditorWorld);
				}
				else if (CommandName == "scale_relative")
				{
					RelativeScaleActorsByName(ObjectNames, NewVector, EditorWorld);
				}
			}
			else
			{
				UE_LOG(LogPromethean, Display, TEXT("Not enough numbers in %s command"), *CommandName);
			}
		}
		else if (CommandName == "translate_and_raytrace" && CommandParameterList.Num() > 3)  
		{ // move to a spot and snap to surface below
			TArray<FString> NewLocationStrArray;
			CommandParameterList[0].ParseIntoArray(NewLocationStrArray, TEXT(","), true);  // numbers are separated by a comma
			FVector NewVector = StringArrayToVector(NewLocationStrArray);
			if (NewVector != NegativeInfinityVector)
			{
				float RayTraceDistance = FCString::Atof(*CommandParameterList[1]);
				float MaxNormalDeviation = 0;
				TArray<FString> ObjectNames;
				CommandParameterList[2].ParseIntoArray(ObjectNames, TEXT(","), true);  // object DCC names are separated by a comma
				TArray<FString> IgnoreObjectNames;
				CommandParameterList[3].ParseIntoArray(IgnoreObjectNames, TEXT(","), true);  // object DCC names are separated by a comma
				TranslateAndRayTraceActorsByName(ObjectNames, NewVector, RayTraceDistance, MaxNormalDeviation, IgnoreObjectNames, EditorWorld);
			}
		}
		else if (CommandName == "translate_and_snap" && CommandParameterList.Num() > 4)
		{  // move to a spot and snap to surface below and orient to its normal		
		TArray<FString> NewLocationStrArray;
		CommandParameterList[0].ParseIntoArray(NewLocationStrArray, TEXT(","), true);  // numbers are separated by a comma
		FVector NewVector = StringArrayToVector(NewLocationStrArray);
		if (NewVector != NegativeInfinityVector)
		{			
			float RayTraceDistance = FCString::Atof(*CommandParameterList[1]);
			float MaxNormalDeviation = FCString::Atof(*CommandParameterList[2]);
			TArray<FString> ObjectNames;
			CommandParameterList[3].ParseIntoArray(ObjectNames, TEXT(","), true);  // object DCC names are separated by a comma
			TArray<FString> IgnoreObjectNames;
			CommandParameterList[4].ParseIntoArray(IgnoreObjectNames, TEXT(","), true);  // object DCC names are separated by a comma
			TranslateAndRayTraceActorsByName(ObjectNames, NewVector, RayTraceDistance, MaxNormalDeviation, IgnoreObjectNames, EditorWorld);
		}
		}
		else if (CommandName == "fix" && CommandParameterList.Num() > 0 && CommandParameterList[0] == "names")  // conditions are checked in order so can reference element 1 after checking array size
		{
			FixObjectNames(EditorWorld);
			UE_LOG(LogPromethean, Display, TEXT("Fixing object Labels to Match their real Names"));
		}
		else if (CommandName == "set_mesh" && CommandParameterList.Num() > 1)  // set mesh asset for static mesh actors by name
		{
			FString AssetPath = CommandParameterList[0];
			TArray<FString> TargetObjectNames;
			CommandParameterList[1].ParseIntoArray(TargetObjectNames, TEXT(","), true);  // object DCC names are separated by a comma.
			auto ValidActors = GetNamedValidActors(TargetObjectNames, EditorWorld);
			for (auto& Actor : ValidActors)
			{
				SetStaticMeshAssetForValidActor(Actor, AssetPath);
			}
		}
		else if (CommandName == "set_mesh_on_selection" && CommandParameterList.Num() > 0)
		{
			FString AssetPath = CommandParameterList[0];
			auto SelectedValidActors = GetSelectedValidActors();
			for (auto& Actor : SelectedValidActors)
			{
				SetStaticMeshAssetForValidActor(Actor, AssetPath);
			}
		}		
		// Mesh Data
		else if (CommandName == "get_vertex_data_from_scene_objects" && CommandParameterList.Num() > 0)
		{
			// get vertex data from a lot of objects at once. full vertex buffer is streamed with every 3 consecutive vertexes forming a triangle
			TArray<FString> ObjectNames;
			CommandParameterList[0].ParseIntoArray(ObjectNames, TEXT(","), true);  // object DCC names are separated by a comma
			TSharedPtr<FJsonObject> RootObject = MakeShared<FJsonObject>();  // blank root json object
			for (auto& ObjectName : ObjectNames)
			{
				AActor* Actor = GetActorByName<AActor>(ObjectName, EditorWorld);
				if (Actor == nullptr)
				{
					continue;
				}
				TArray<FVector> Vertexes = GetMeshTrianglePositionsFromActor(Actor);				
				TArray<TSharedPtr<FJsonValue>> JSONDictArray = FVectorArrayToJsonArray(Vertexes);  // vertex arrray to json array
				RootObject->SetArrayField(*ObjectName, JSONDictArray);  // add to root json object with dcc_name as label						
				UE_LOG(LogPromethean, Display, TEXT("%s Vertex Number: %i"), *ObjectName, Vertexes.Num());
			}
			// output json data to string				
			FString ReturnString;
			TSharedRef< TJsonWriter<> > Writer = TJsonWriterFactory<>::Create(&ReturnString);
			FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);
			TCPSend(ReturnString);  // send back the final string
		}
		else if (CommandName == "get_vertex_data_from_scene_object" && CommandParameterList.Num() > 0)
		{			
			// get vertex data from one scene object at a time. full vertex buffer is streamed with every 3 consecutive vertexes forming a triangle
			FString ObjectName = CommandParameterList[0];
			AActor* Actor = GetActorByName<AActor>(ObjectName, EditorWorld);
			if (Actor == nullptr)
			{
				continue;
			}
			TArray<FVector> Vertexes = GetMeshTrianglePositionsFromActor(Actor);
			FString ReturnString = FVectorArrayToJsonString(Vertexes, TEXT("vertex_positions"));
			//UE_LOG(LogPromethean, Display, TEXT("\nStatic Mesh Vertex Data: %s"), *ReturnString);
			UE_LOG(LogPromethean, Display, TEXT("\nStatic Mesh Vertex Number: %i"), Vertexes.Num());
			TCPSend(ReturnString);
		}
		// Asset Browser		
		else if (CommandName == "get_asset_data_from_browser_selection" && CommandParameterList.Num() > 0)
		{
			// this is old will save to file. TODO: delete
			FString DataString = getJSONAssetDataFromAssetBrowserSelection();
			SaveToFile(CommandParameterString, DataString);
			FString ReturnString = "Done";
			UE_LOG(LogPromethean, Display, TEXT("\nAsset Browser Selection Data: %s"), *ReturnString);
			TCPSend(ReturnString);
		}
		else if (CommandName == "get_asset_data_from_paths" && CommandParameterList.Num() > 0)
		{
			FString DataString = getJSONAssetDataFromAssetPaths(CommandParameterList);
			DataString = DataString.Replace(TEXT("\n"), TEXT(""));  // replace in place, all of the new lines in json mess with promethean messaging			
			FString FinalMessage = TEXT("update_cmd_line_dcc_progress ") + DataString;
			UE_LOG(LogPromethean, Display, TEXT("\nGetting Asset Data From Asset Paths: %s"), *FinalMessage);
			SendSingleTCPMessage(PrometheanAssetDataPort, FinalMessage);  // learning port
		}
		else if (CommandName == "get_all_existing_assets_by_type" && CommandParameterList.Num() > 1)
		{
		FString AssetType = CommandParameterList[0];
		FString FilePath = CommandParameterString.Replace(TEXT("get_all_existing_assets_by_type "), TEXT(""));
		FString AssetTypeWithSpace = AssetType + " ";
		const TCHAR* AssetTypeTChar = *AssetTypeWithSpace;
		FilePath = FilePath.Replace(AssetTypeTChar, TEXT(""));
		FString ReturnString = getAllExistingAssetByType(FilePath, AssetType);  // parameter is asset type and file to output to in order
		UE_LOG(LogPromethean, Display, TEXT("\nGet all existing assets: %s - %s"), *CommandParameterList[0], *CommandParameterList[1]);
		}
		else if (CommandName == "get_asset_browser_selection")
		{
			FString SelectionString = GetAssetBrowserSelection();
			UE_LOG(LogPromethean, Display, TEXT("Asset Browser Selection: %s"), *SelectionString);
			if (SelectionString == "")
				SelectionString = "None";
			TCPSend(SelectionString);
		}
		else if (CommandName == "import_asset")
		{
			auto ImportedObjects = ImportFBXAssetsFromJsonString(CommandParameterString);
			if (ImportedObjects.Num() > 0)
			{
				SetAssetBrowserSelection(ImportedObjects);
			}
		}
		else if (CommandName == "import_texture")
		{
			auto ImportedObjects = ImportTextureAssetsFromJsonString(CommandParameterString);
			if (ImportedObjects.Num() > 0)
			{
				SetAssetBrowserSelection(ImportedObjects);
			}
		}
		else if (CommandName == "import_asset_to_asset_browser")
		{
			ImportAssetToCurrentPath();
		}
		else if (CommandName == "import_asset_to_asset_browser_at_path"  && CommandParameterList.Num() > 0)
		{
			FString ImportPath = CommandParameterList[0];
			ImportAsset(ImportPath);
		}
		else if (CommandName == "delete_selected_assets_in_browser")
		{
			DeleteSelectedAssets();
		}
		else if (CommandName == "edit" && CommandParameterList.Num() > 0)
		{
			EditAsset(CommandParameterList[0]);
		}
		else if (CommandName == "find" && CommandParameterList.Num() > 0)
		{
			FindAsset(CommandParameterList[0]);
		}
		else if (CommandName == "find_assets" && CommandParameterList.Num() > 0)
		{
			FindAssets(CommandParameterList);
		}		
		else if (CommandName == "load_assets" && CommandParameterList.Num() > 0)
		{
			LoadAssets(CommandParameterList);
		}
		// Get Actors from Scene		
		else if (CommandName == "get_pivots_for_static_mesh_actors" && CommandParameterList.Num() > 0)
		{
			TArray<FString> ActorNames;
			CommandParameterList[0].ParseIntoArray(ActorNames, TEXT(","), true);  // object DCC names are separated by a comma
			auto ValidActors = GetNamedValidActors(ActorNames, EditorWorld);
			FString OutString = GetValidActorsBottomCenterLocationJSONDict(ValidActors);
			// UE_LOG(LogPromethean, Display, TEXT("Actor Pivot Info: %s"), *OutString);  // debug output
			TCPSend(OutString);
		}
		else if (CommandName == "get_locations_for_static_mesh_actors" && CommandParameterList.Num() > 0)
		{
			TArray<FString> ActorNames;
			CommandParameterList[0].ParseIntoArray(ActorNames, TEXT(","), true);  // object DCC names are separated by a comma
			auto ValidActors = GetNamedValidActors(ActorNames, EditorWorld);
			FString OutString = GetValidActorsLocationJSONDict(ValidActors);
			UE_LOG(LogPromethean, Display, TEXT("Actor Location Info: %s"), *OutString);
			TCPSend(OutString);
		}
		else if (CommandName == "get_transforms_for_static_mesh_actors" && CommandParameterList.Num() > 0)
		{
			TArray<FString> ActorNames;
			CommandParameterList[0].ParseIntoArray(ActorNames, TEXT(","), true);  // object DCC names are separated by a comma
			auto ValidActors = GetNamedValidActors(ActorNames, EditorWorld);
			FString OutString = GetValidActorsExpandedTransformJSONDict(ValidActors, EditorWorld);
			UE_LOG(LogPromethean, Display, TEXT("Update Actor Info"));  // log that we did update
			// UE_LOG(LogPromethean, Display, TEXT("Actor Transform Info: %s"), *OutString);  // print all the outgoing data
			TCPSend(OutString);
		}		
		else if (CommandName == "get_all_valid_scene_actors")
		{
			TArray<AActor*> ValidActors = GetWorldValidActors(EditorWorld);
			FString NameString = ActorArrayToNamesString(ValidActors);
			UE_LOG(LogPromethean, Display, TEXT("Valid Actors in the Scene: %s"), *NameString);
			TCPSend(NameString);
		}
		else if (CommandName == "get_all_valid_scene_actors_and_paths")
		{
			TArray<AActor*> ValidActors = GetWorldValidActors(EditorWorld);
			TArray<FString> Names = ActorArrayToNamesArray(ValidActors);
			TArray<FString> Paths = ActorArrayToAssetPathsArray(ValidActors);
			TSharedPtr<FJsonObject> RootObject = MakeShared<FJsonObject>();  // blank root json object
			RootObject->SetArrayField("scene_names", FStringArrayToJsonArray(Names));
			RootObject->SetObjectField("scene_paths", FStringArrayToJsonIndexDict(Paths));
			FString ReturnString;
			TSharedRef< TJsonWriter<> > Writer = TJsonWriterFactory<>::Create(&ReturnString);
			FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);
			TCPSend(ReturnString);  // send back the final string
		}
		else if (CommandName == "get_visible_static_mesh_actors")
		{
			TArray<AActor*> RenderedActors = GetRenderedValidActors();
			FString NameString = ActorArrayToNamesString(RenderedActors);
			UE_LOG(LogPromethean, Display, TEXT("StaticMeshActors on Screen: %s"), *NameString);
			TCPSend(NameString);
		}
		else if (CommandName == "get_selected_static_mesh_actors")
		{			
			TArray<AActor*> SelectedValidActors = GetSelectedValidActors();
			FString SelectedNameString = ActorArrayToNamesString(SelectedValidActors);
			UE_LOG(LogPromethean, Display, TEXT("StaticMeshActors Selected: %s"), *SelectedNameString);
			TCPSend(SelectedNameString);
		}
		else if (CommandName == "get_selected_and_visible_static_mesh_actors")
		{			
			TArray<AActor*> SelectedActors = GetSelectedValidActors();			
			TArray<AActor*> RenderedActors = GetRenderedValidActors();
			// add descendents of selection to list of render objects
			TArray<AActor*> SelectedActorDescendents = GetAllDescendentsForObjectsRecursive(SelectedActors, EditorWorld);
			for (auto& Actor : SelectedActorDescendents)
				RenderedActors.AddUnique(Actor);  // merge a arrays
			// add objects overlapping with selected objects to render objects
			TArray<AActor*> SelectedActorOverlapping = GetAllObjectsIntersectingGivenObjects(SelectedActors);
			for (auto& Actor : SelectedActorOverlapping)
				RenderedActors.AddUnique(Actor);  // merge a arrays
			// get other data
			TArray<FVector> SelectedLocations = ActorArrayToPivotArray(SelectedActors);
			TArray<FVector> RenderedLocations = ActorArrayToPivotArray(RenderedActors);
			TArray<FString> SelectedNames = ActorArrayToNamesArray(SelectedActors);
			TArray<FString> RenderedNames = ActorArrayToNamesArray(RenderedActors);
			TArray<FString> SelectedPaths = ActorArrayToAssetPathsArray(SelectedActors);
			TArray<FString> RenderedPaths = ActorArrayToAssetPathsArray(RenderedActors);
			// camera location
			FEditorViewportClient* client = (FEditorViewportClient*)GEditor->GetActiveViewport()->GetClient();
			FVector editorCameraPosition = client->GetViewLocation();
			FVector editorCameraDirection = client->GetViewRotation().Vector();
			
			UE_LOG(LogPromethean, Display, TEXT("Valid Actors Selected: %i"), SelectedNames.Num());
			UE_LOG(LogPromethean, Display, TEXT("Valid Actors on Screen: %i"), RenderedNames.Num());
			UE_LOG(LogPromethean, Display, TEXT("Valid Actors Selected Paths: %i"), SelectedPaths.Num());			
			UE_LOG(LogPromethean, Display, TEXT("Valid Actors on Screen Paths: %i"), RenderedPaths.Num());

			TSharedPtr<FJsonObject> RootObject = MakeShared<FJsonObject>();  // blank root json object
			RootObject->SetArrayField("selected_names", FStringArrayToJsonArray(SelectedNames));
			RootObject->SetArrayField("rendered_names", FStringArrayToJsonArray(RenderedNames));
			RootObject->SetArrayField("selected_locations", FVectorArrayToJsonArray(SelectedLocations));
			RootObject->SetArrayField("rendered_locations", FVectorArrayToJsonArray(RenderedLocations));
			RootObject->SetObjectField("selected_paths", FStringArrayToJsonIndexDict(SelectedPaths));
			RootObject->SetObjectField("rendered_paths", FStringArrayToJsonIndexDict(RenderedPaths));
			RootObject->SetStringField("scene_name", getSceneName(EditorWorld));
			RootObject->SetArrayField("camera_location", FVectorToJsonArray(editorCameraPosition));
			RootObject->SetArrayField("camera_direction", FVectorToJsonArray(editorCameraDirection));
			FString ReturnString;
			TSharedRef< TJsonWriter<> > Writer = TJsonWriterFactory<>::Create(&ReturnString);
			FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);
			TCPSend(ReturnString);  // send back the final string
		}
		else if (CommandName == "get_visible_actors")  // TODO: doesn't work
		{
			TArray<AActor*> RenderedActors = GetRenderedActors();
			FString NameString = ActorArrayToNamesString(RenderedActors);
			UE_LOG(LogPromethean, Display, TEXT("Actors on Screen: %s"), *NameString);
			// TODO: Mangles the string on send. Need to investigate why but not sure if this will ever be needed. So future coder - be forewarned.
			TCPSend(NameString);
		}
		else if (CommandName == "set_camera_location" && CommandParameterList.Num() > 0)
		{	
			TArray<FString> locationStrArray;
			CommandParameterList[0].ParseIntoArray(locationStrArray, TEXT(","), true);  // numbers are separated by a comma
			FVector editorCameraPosition = StringArrayToVector(locationStrArray);
			FEditorViewportClient* client = (FEditorViewportClient*)GEditor->GetActiveViewport()->GetClient();
			client->SetViewLocation(editorCameraPosition);
		}
		else if (CommandName == "set_camera_direction" && CommandParameterList.Num() > 0)
		{
			TArray<FString> directionStrArray;
			CommandParameterList[0].ParseIntoArray(directionStrArray, TEXT(","), true);  // numbers are separated by a comma
			FVector editorCameraDirection = StringArrayToVector(directionStrArray);
			FEditorViewportClient* client = (FEditorViewportClient*)GEditor->GetActiveViewport()->GetClient();
			client->SetViewRotation(editorCameraDirection.Rotation());
        }
		else if (CommandName == "get_camera_info")
		{			
			FString OutString = getCameraInfoJSONDict(true);  // attribute: include objects_on_screen
			UE_LOG(LogPromethean, Display, TEXT("Camera Info: %s"), *OutString);  // debug output
			TCPSend(OutString);
		}
		else if (CommandName == "raytrace_from_camera")
		{
			FVector HitLocation = RayTraceFromCamera(EditorWorld);
			TSharedPtr<FJsonObject> RootObject = MakeShared<FJsonObject>();  // blank root json object			
			RootObject->SetArrayField("hit_location", FVectorToJsonArray(HitLocation));			
			FString ReturnString;
			TSharedRef< TJsonWriter<> > Writer = TJsonWriterFactory<>::Create(&ReturnString);
			FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);
			UE_LOG(LogPromethean, Display, TEXT("Raytrace from Camera: %s"), *ReturnString);  // debug output
			TCPSend(ReturnString);
		}
		else if (CommandName == "save_camera_info"  && CommandParameterList.Num() > 0)  // 
		{
			FString OutString = getCameraInfoJSONDict(true);
			UE_LOG(LogPromethean, Display, TEXT("Saving Camera Info: %s\nOutput path: %s"), *OutString, *CommandParameterString);  // debug output
			SaveToFile(CommandParameterString, OutString);
		}
		else if (CommandName == "get_static_mesh_actors_by_path" && CommandParameterList.Num() > 0)
		{
			FString StaticMeshPath = CommandParameterList[0];
			TArray<AActor*> FitActors = GetValidActorsByMeshAssetPath(StaticMeshPath, EditorWorld);
			FString OutString = ActorArrayToNamesString(FitActors);
			UE_LOG(LogPromethean, Display, TEXT("Get actors by path: %s"), *OutString);
			TCPSend(OutString);
		}
		else if (CommandName == "select_static_mesh_actors_by_path" && CommandParameterList.Num() > 0)
		{
			FString StaticMeshPath = CommandParameterList[0];
			TArray<AActor*> FitActors = GetValidActorsByMeshAssetPath(StaticMeshPath, EditorWorld);
			FString OutString = ActorArrayToNamesString(FitActors);
			TArray<FString> OutStringArray;
			OutString.ParseIntoArray(OutStringArray, TEXT(","), true);  // object DCC names are separated by a comma
			UE_LOG(LogPromethean, Display, TEXT("Select actors by path: %s"), *OutString);
			SelectSceneActorsByName(OutStringArray);
		}
		else if (CommandName == "get_static_mesh_actors_by_material_path" && CommandParameterList.Num() > 0)
		{
			FString MaterialPath = CommandParameterList[0];
			TArray<AActor*> FitActors = GetValidActorsByMaterialPath(MaterialPath, EditorWorld);
			FString OutString = ActorArrayToNamesString(FitActors);
			UE_LOG(LogPromethean, Display, TEXT("Get actors by material path: %s"), *OutString);
			TCPSend(OutString);
		}
		else if (CommandName == "select_static_mesh_actors_by_material_path" && CommandParameterList.Num() > 0)
		{
			FString MaterialPath = CommandParameterList[0];
			TArray<AActor*> FitActors = GetValidActorsByMaterialPath(MaterialPath, EditorWorld);
			FString OutString = ActorArrayToNamesString(FitActors);
			TArray<FString> OutStringArray;
			OutString.ParseIntoArray(OutStringArray, TEXT(","), true);  // object DCC names are separated by a comma
			UE_LOG(LogPromethean, Display, TEXT("Select actors by material path: %s"), *OutString);
			SelectSceneActorsByName(OutStringArray);
		}
		// Materials
		else if (CommandName == "create_material_instance")
		{
			CreateMaterialInstancesFromJsonString(CommandParameterString);
		}
		else if (CommandName == "set_mesh_asset_material")
		{
			SetMeshAssetMaterialFromJsonString(CommandParameterString);
		}
		else if (CommandName == "set_materials_for_actor" && CommandParameterList.Num() > 1)
		{
			FString ActorName = CommandParameterList[0];
			// Extract the whole JsonString by finding the first separator in CommandParameterString, we can't use CommandParameterList, as it will be chopped because of spaces.
			FString JsonString;
			int JsonStringStartIndex;
			CommandParameterString.FindChar(' ', JsonStringStartIndex);
			JsonString = CommandParameterString.RightChop(JsonStringStartIndex + 1);

			// Parse JsonString
			TSharedPtr<FJsonObject> JsonObject;
			if (!ParseJsonString(JsonString, JsonObject))
			{
				continue;
			}

			// Set materials for the actor
			auto Actor = GetValidActorByName(ActorName, EditorWorld);
			for (auto JsonValue : JsonObject->Values)
			{
				int32 MaterialIndex = FCString::Atoi(*JsonValue.Key);
				FString MaterialPath = JsonValue.Value->AsString();
				SetMaterialForValidActor(Actor, MaterialPath, MaterialIndex);
			}
		}
		else if (CommandName == "remove_material_overrides" && CommandParameterList.Num() > 0)
		{
			TArray<FString> TargetObjectNames;
			CommandParameterList[0].ParseIntoArray(TargetObjectNames, TEXT(","), true);  // object DCC names are separated by a comma.
			for (auto& TargetObjectName : TargetObjectNames)
			{
				RemoveMaterialOverride(TargetObjectName, EditorWorld);
			}
		}
		// set attributes
		else if (CommandName == "set_mat_instance_vec_attr_for_mesh_actors_by_name_and_index"  && CommandParameterList.Num() > 3)
		{
			FString AttrName = CommandParameterList[0];
			FLinearColor AttrValue = StringToLinearColor(CommandParameterList[1]);  // TODO: check that Linear Color is Valid
			TArray<FString> SetObjects;
			CommandParameterList[2].ParseIntoArray(SetObjects, TEXT(","), true);  // object DCC names are separated by a comma
			int Index = FCString::Atoi(*CommandParameterList[3]); // material index to set
			auto ValidActors = GetNamedValidActors(SetObjects, EditorWorld);
			for (AActor* Actor : ValidActors)
			{
				SetVecMatAttrsForValidActor(Actor, AttrName, AttrValue, Index);
			}
		}
		else if (CommandName == "set_mat_instance_vec_attr_for_mesh_actors_by_name"  && CommandParameterList.Num() > 2)
		{
			FString AttrName = CommandParameterList[0];
			FLinearColor AttrValue = StringToLinearColor(CommandParameterList[1]);  // TODO: check that Linear Color is Valid
			TArray<FString> SetObjects;
			CommandParameterList[2].ParseIntoArray(SetObjects, TEXT(","), true);  // object DCC names are separated by a comma
			auto ValidActors = GetNamedValidActors(SetObjects, EditorWorld);
			int index = 0;  // TODO: fix in 4.25 - add proper support
			for (auto& Actor : ValidActors)
			{
				SetVecMatAttrsForValidActor(Actor, AttrName, AttrValue, index);
			}
		}
		else if (CommandName == "set_mat_instance_scalar_attr_for_mesh_actors_by_name"  && CommandParameterList.Num() > 2)
		{
			FString AttrName = CommandParameterList[0];
			float AttrValue = FCString::Atof(*CommandParameterList[1]);  // TODO: need to check if the value is valid but still be ok when it's 0.0 
			TArray<FString> SetObjects;
			CommandParameterList[2].ParseIntoArray(SetObjects, TEXT(","), true);  // object DCC names are separated by a comma
			auto ValidActors = GetNamedValidActors(SetObjects, EditorWorld);
			for (auto& Actor : ValidActors)
			{
				SetScalarMatAttrsForValidActor(Actor, AttrName, AttrValue);
			}
		}
		// get attribute values
		else if (CommandName == "get_mat_instance_vec_attr_val_for_mesh_actors_by_name"  && CommandParameterList.Num() > 1)
		{
			FString AttrName = CommandParameterList[0];
			TArray<FString> SetObjects;
			CommandParameterList[1].ParseIntoArray(SetObjects, TEXT(","), true);  // object DCC names are separated by a comma
			int Index = -1; // default value
			if (CommandParameterList.Num() > 2)
			{
				Index = FCString::Atoi(*CommandParameterList[2]); // material index to set
			}
			auto ValidActors = GetNamedValidActors(SetObjects, EditorWorld);
			TArray<FLinearColor> OutVectors = GetVecMatAttrsForValidActors(ValidActors, AttrName, Index);
			FString AttributeString = LinearColorArrayToString(OutVectors);
			if (AttributeString == "")
				AttributeString = "None";
			UE_LOG(LogPromethean, Display, TEXT("%s material attribute values: %s"), *AttrName, *AttributeString);
			TCPSend(AttributeString);
		}
		else if (CommandName == "get_mat_instance_scalar_attr_val_for_mesh_actors_by_name"  && CommandParameterList.Num() > 1)
		{
			FString AttrName = CommandParameterList[0];
			TArray<FString> SetObjects;
			CommandParameterList[1].ParseIntoArray(SetObjects, TEXT(","), true);  // object DCC names are separated by a comma
			auto ValidActors = GetNamedValidActors(SetObjects, EditorWorld);
			TArray<float> OutFloats = GetScalarMatAttrsForValidActors(ValidActors, AttrName);			
			FString AttributeString = FloatArrayToString(OutFloats);
			UE_LOG(LogPromethean, Display, TEXT("%s material attribute values: %s"), *AttrName, *AttributeString);
			TCPSend(AttributeString);
		}		
		// get parameter and material names
		else if (CommandName == "get_mat_attrs_from_mesh_actors_by_name"  && CommandParameterList.Num() > 1)
		{
			FString AttrType = CommandParameterList[0];  // Scalar, Vector, Texture, StaticSwitch, ComponentMask
			TArray<FString> SelectObjects;
			CommandParameterList[1].ParseIntoArray(SelectObjects, TEXT(","), true);  // object DCC names are separated by a comma
			TArray<FString> AttributeNames = GetMaterialAttributeNamesFromValidActors(SelectObjects, AttrType, EditorWorld);
			FString MaterialNameString = StringArrayToString(AttributeNames);
			UE_LOG(LogPromethean, Display, TEXT("%s material attribute names: %s"), *AttrType, *MaterialNameString);
			TCPSend(MaterialNameString);
		}
		else if (CommandName == "get_mat_attrs_from_selected_mesh_actors"  && CommandParameterList.Num() > 0)
		{
			FString AttrType = CommandParameterList[0];  // Scalar, Vector, Texture, StaticSwitch, ComponentMask
			TArray<FString> AttributeNames = getMaterialAttributeNamesFromSelectedStaticMeshActors(AttrType);
			FString MaterialNameString = StringArrayToString(AttributeNames);
			UE_LOG(LogPromethean, Display, TEXT("%s material attribute names for selection: %s"), *AttrType, *MaterialNameString);
			TCPSend(MaterialNameString);
		}		
		else if (CommandName == "get_materials_from_mesh_actors_by_name"  && CommandParameterList.Num() > 0)
		{
			TArray<FString> ActorNames;
			CommandParameterList[0].ParseIntoArray(ActorNames, TEXT(","), true);  // object DCC names are separated by a comma
			FString MaterialNames = GetMaterialPathJSONDictFromActorsByName(ActorNames, EditorWorld);
			TCPSend(MaterialNames);
		}
		else if (CommandName == "get_materials_from_selected_mesh_actor")
		{
			TArray<FString> MaterialNames = GetMaterialPathsFromSelectedValidActors();
			FString MaterialNameString = StringArrayToString(MaterialNames);
			UE_LOG(LogPromethean, Display, TEXT("Materials on selections: %s"), *MaterialNameString);
			TCPSend(MaterialNameString);
		}
		else if (CommandName == "set_materials_for_selected_mesh_actor"  && CommandParameterList.Num() > 0)
		{
			FString MaterialAssetPath = CommandParameterList[0];
			int Index = 0;
			if (CommandParameterList.Num() > 1)
				Index = FCString::Atoi(*CommandParameterList[1]); // material index to set
			UE_LOG(LogPromethean, Display, TEXT("Material to set: %s. Index num: %i"), *MaterialAssetPath, Index);
			SetMaterialForSelectedValidActors(MaterialAssetPath, Index);
		}
		// ray tracing 
		else if (CommandName == "raytrace_points"  && CommandParameterList.Num() > 1)		
		{
			FVector Direction = StringToVector(CommandParameterList[0]);			
			// ignore actors
			TArray<FString> ActorNames;
			CommandParameterList[1].ParseIntoArray(ActorNames, TEXT(","), true);  // object DCC names are separated by a comma
			TArray<AActor*> IgnoreActors = {};
			for (auto& ActorName : ActorNames)
			{ 
				auto StaticMeshActor = GetActorByName<AActor>(ActorName, EditorWorld);
				if (StaticMeshActor != nullptr)
				{
					IgnoreActors.Add(StaticMeshActor);
				}
			}
			// points
			CommandParameterList.RemoveAt(0);  // remove direction
			CommandParameterList.RemoveAt(0);  // remove ingore names  - leave only points
			TArray<FVector> StartPoints;
			for (auto& VectorString : CommandParameterList)
			{
				StartPoints.Add(StringToVector(VectorString));
			}
			FString HitLocationJSONDict = RayTraceFromPointsJSON(StartPoints, Direction, EditorWorld, IgnoreActors);						
			TCPSend(HitLocationJSONDict);
		}
		else if (CommandName == "raytrace"  && CommandParameterList.Num() > 1)
		{
			FVector Offset = StringToVector(CommandParameterList[0]);
			TArray<FString> ActorNames;
			CommandParameterList[1].ParseIntoArray(ActorNames, TEXT(","), true);  // object DCC names are separated by a comma
			FString HitLocationJSONDict = RayTraceFromActorsByNameJSON(ActorNames, Offset, EditorWorld);
			TCPSend(HitLocationJSONDict);
		}
		else if (CommandName == "raytrace_bidirectional"  && CommandParameterList.Num() > 1)
		{  // take a direction vector and raycasts along it and it's inverse to find the closest intersection with a normal facing to direction vector
			FVector Offset = StringToVector(CommandParameterList[0]);
			TArray<FString> ActorNames;
			CommandParameterList[1].ParseIntoArray(ActorNames, TEXT(","), true);  // object DCC names are separated by a comma
			FString HitLocationJSONDict = RayTraceMultiFromActorsByNameJSON(ActorNames, Offset, EditorWorld);
			TCPSend(HitLocationJSONDict);
		}
		else if (CommandName == "screenshot"  && CommandParameterList.Num() > 0)
		{
			CaptureScreenshot(CommandParameterString);
			TCPSend("Done");
		}
		// simulation
		else if (CommandName == "start_simulation")
		{
			EnablePlayInEditor();
		}
		else if (CommandName == "end_simulation")
		{
			DisablePlayInEditor();
		}
		else if (CommandName == "get_transform_data_from_simulating_actors_by_name" && CommandParameterList.Num() > 0)
		{
			TArray<FString> ObjectNames;
			CommandParameterList[0].ParseIntoArray(ObjectNames, TEXT(","), true);  // object DCC names are separated by a comma
			FString OutJsonDict = GetSimulatingActorsTransformJSONDictByNames(ObjectNames);  // actor dcc names as key and t,t,t,r,r,r,s,s,s for value
			TCPSend(OutJsonDict);
		}
		else if (CommandName == "get_simulation_on_actors_by_name" && CommandParameterList.Num() > 0)
		{
			TArray<FString> ObjectNames;
			CommandParameterList[0].ParseIntoArray(ObjectNames, TEXT(","), true);  // object DCC names are separated by a comma			
			FString SimulationStateJSONDict = GetActorsPhysicsSimulationStateByName(ObjectNames);
			TCPSend(SimulationStateJSONDict);
		}
		else if (CommandName == "enable_simulation_on_actors_by_name" && CommandParameterList.Num() > 0)
		{
			TArray<FString> ObjectNames;
			CommandParameterList[0].ParseIntoArray(ObjectNames, TEXT(","), true);  // object DCC names are separated by a comma
			SetActorsToBePhysicallySimulatedByName(ObjectNames, true);  // second argument is simulation state
		}
		else if (CommandName == "disable_simulation_on_actors_by_name" && CommandParameterList.Num() > 0)
		{
			TArray<FString> ObjectNames;
			CommandParameterList[0].ParseIntoArray(ObjectNames, TEXT(","), true);  // object DCC names are separated by a comma
			SetActorsToBePhysicallySimulatedByName(ObjectNames, false);  // second argument is simulation state
		}
		// misc
		else if (CommandName == "toggle_surface_snapping")
		{
			// SEditorViewport::OnToggleSurfaceSnap();
			auto* Settings = GetMutableDefault<ULevelEditorViewportSettings>();
			Settings->SnapToSurface.bEnabled = !Settings->SnapToSurface.bEnabled;
		}		
		else if (CommandName == "rename" && CommandParameterList.Num() > 1)
		{
		// TODO: get this to return the resulting DCC names. For now just one sided
		RenameActor(CommandParameterList[0], CommandParameterList[1], EditorWorld);
		}
		else if (CommandName == "_kill_")
		{
			ToggleKillNamesOnSelection();
		}
		else if (CommandName == "open_level"  && CommandParameterList.Num() > 0)
		{
			FString LevelPath = CommandParameterList[0];
			OpenLevel(LevelPath);
		}
		else if (CommandName == "load_level" && CommandParameterList.Num() > 0)
		{		
			FString LevelPath = CommandParameterList[0];
			LoadLevel(LevelPath, EditorWorld);
		}
		else if (CommandName == "unload_level" && CommandParameterList.Num() > 0) 
		{
			FString LevelPath = CommandParameterList[0];
			UnloadLevel(LevelPath, EditorWorld);
		}
		else if (CommandName == "set_level_visible" && CommandParameterList.Num() > 0)
		{
			FString LevelPath = CommandParameterList[0];
			SetLevelVisibility(LevelPath, true, EditorWorld);
		}
		else if (CommandName == "set_level_invisible" && CommandParameterList.Num() > 0)
		{
			FString LevelPath = CommandParameterList[0];
			SetLevelVisibility(LevelPath, false, EditorWorld);
		}
		else if (CommandName == "set_level_current" && CommandParameterList.Num() > 0)
		{
			FString LevelPath = CommandParameterList[0];
			SetLevelCurrent(LevelPath, EditorWorld);
		}
		else if (CommandName == "get_current_level_path")
		{
			FString LevelPath = GetCurrentLevelPath(EditorWorld);
			TCPSend(LevelPath);
		}		
		else if (CommandName == "undo")
		{
			GEditor->EndTransaction(); // close currently open undo stack
			GEditor->UndoTransaction(true);
		}
		else if (CommandName == "redo")
		{			
			GEditor->RedoTransaction();	// TODO: this is not working	
		}
		else if (CommandName == "report_done")
		{
			TCPSend("Done");
		}
		else
		{
			UE_LOG(LogPromethean, Display, TEXT("%s - is not a recognized command!"), *CommandName);
		}
	}	
	GEditor->EndTransaction(); // end undo
	UE_LOG(LogPromethean, Display, TEXT("Finished Executing TCP Command"));
	GEditor->NoteSelectionChange(true);  // wheh object is moved from code gizmo is left in old location. this will refresh it
	// UE_LOG(LogPromethean, Display, TEXT("Finished Updating Transform Locator"));
}

void MetaExecuteTCPCommand()
{
	isCommandExecuting = true;
	while (GlobalCommandStrArray.Num() != 0)
	{
		FString MetaCommandStr = GlobalCommandStrArray[0];
		GlobalCommandStrArray.RemoveAt(0);
		ExecuteTCPCommand(MetaCommandStr);
	}
	isCommandExecuting = false;
}

// --------------------------------------------------------------------
// --- Network Single Message Code
// --------------------------------------------------------------------
// Creates a connection sends a message and closes it
// Some reference: https://wiki.unrealengine.com/Third_Party_Socket_Server_Connection

bool SendSingleTCPMessage(const int32 TargetPort, FString ToSend)
{	
	// check message
	if (ToSend.IsEmpty()) {
		UE_LOG(LogPromethean, Display, TEXT("Single TCP Message Is empty"));
		return false;
	}
	// create socket
	FSocket* SingleMessageSocket = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateSocket("SingleMessageSocket", TEXT("SingleMessageSocket description"), false);  // Type, Description, ForceUDP
	TSharedRef<FInternetAddr> addr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	FIPv4Address ip(127, 0, 0, 1);
	addr->SetIp(ip.Value);
	addr->SetPort(TargetPort);
	bool connected = SingleMessageSocket->Connect(*addr);

	if (!SingleMessageSocket || !connected) {
		UE_LOG(LogPromethean, Display, TEXT("Single Message TCP connection Failed"));
		return false;
	}	
	// build the message - m2u example
	int32 DestLen = TStringConvert<TCHAR, ANSICHAR>::ConvertedLength(*ToSend, ToSend.Len());
	uint8* Dest = new uint8[DestLen + 1];
	TStringConvert<TCHAR, ANSICHAR>::Convert((ANSICHAR*)Dest, DestLen, *ToSend, ToSend.Len());
	Dest[DestLen] = '\0';
	int32 BytesSent = 0;

	if (!SingleMessageSocket->Send(Dest, DestLen, BytesSent))
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Error sending Promethean message")));
	}

	SingleMessageSocket->Close(); // close just in case there is an open one
	UE_LOG(LogPromethean, Display, TEXT("TCP Single Message Succeeded"));
	return true;
}

// --------------------------------------------------------------------
// --- Network Code
// --------------------------------------------------------------------
// https://wiki.unrealengine.com/TCP_Socket_Listener,_Receive_Binary_Data_From_an_IP/Port_Into_UE4,_(Full_Code_Sample)
// https://forums.unrealengine.com/development-discussion/c-gameplay-programming/10427-tcp-socket-listener-receiving-binary-data-into-ue4-from-a-python-script?18566-TCP-Socket-Listener-Receiving-Binary-Data-into-UE4-From-a-Python-Script!=&viewfull=1


// TCP Server Code
// Entry point to launching the server
bool LaunchTCP()
{
	TCPCloseConnection(); // close just in case there is an open one
	UE_LOG(LogPromethean, Display, TEXT("=========================== Establishing a TCP connection..."));
	if (!StartTCPReceiver("PrometheanSocketListener", "127.0.0.1", PrometheanUE4Port))
	{
		UE_LOG(LogPromethean, Display, TEXT("=========================== TCP connection Failed"));
		return false;
	}
	UE_LOG(LogPromethean, Display, TEXT("=========================== TCP connection Succeeded"));
	return true;
}


// Start TCP Receiver
// The receiver creates a listener and binds it to the connection listener
bool StartTCPReceiver(const FString& YourChosenSocketName, const FString& TheIP, const int32 ThePort) 
{	
	ListenerSocket = CreateTCPListenerSocket(YourChosenSocketName, TheIP, ThePort);
	if (!ListenerSocket)  //Not created?
	{
		UE_LOG(LogPromethean, Error, TEXT("PrometheanAITCPReceiver>> Listen socket could not be created! ~> %s %d"), *TheIP, ThePort);
		if (GEngine != nullptr) {
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Promethean Plugin Connection Failed!")));
		}
		return false;
	}
	//Start the Listener! //thread this eventually
	FTimerDelegate TimerCallback;  // bind a basic function to a TimerDelegate to set a timer without needing an object
	TimerCallback.BindLambda([]
	{
		TCPConnectionListener();
	});
	GEditor->GetTimerManager()->SetTimer(TCPConnectionListenerTimerHandle, TimerCallback, GLOBAL_SERVER_HEARTBEAT, true);
	GEngine->AddOnScreenDebugMessage(-1, 7.f, FColor::Green, FString::Printf(TEXT("Promethean Plugin Ready!")));	
	return true;
}

// Create TCP Listener Socket
FSocket* CreateTCPListenerSocket(const FString& YourChosenSocketName, const FString& TheIP, const int32 ThePort, const int32 ReceiveBufferSize)
{
	uint8 IP4Nums[4];
	if (!FormatIP4ToNumber(TheIP, IP4Nums))	
		return nullptr;
	//Create Socket	
	FIPv4Endpoint Endpoint(FIPv4Address(IP4Nums[0], IP4Nums[1], IP4Nums[2], IP4Nums[3]), ThePort);
	FSocket* ListenSocket = FTcpSocketBuilder(*YourChosenSocketName)
		.AsReusable()
		.BoundToEndpoint(Endpoint)
		.Listening(8);	
	if (!ListenSocket) {		
		UE_LOG(LogPromethean, Display, TEXT("Failed to create the socket as configured!"));
		return nullptr;  // sometimes the socket doesn't get created correctly - FTcpSocketBuilder: Failed to create the socket SingleMessageSocket as configured
	}
	//Set Buffer Size
	int32 NewSize = 0;
	ListenSocket->SetReceiveBufferSize(ReceiveBufferSize, NewSize);	
	return ListenSocket;
}

// TCP Connection Listener
// Connection listener gets incoming message socket and send it to TCPSocketListener() to recieve the message
void TCPConnectionListener()
{	
	if (!ListenerSocket) return;	
	//Remote address
	TSharedRef<FInternetAddr> RemoteAddress = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	bool Pending;
	// handle incoming connections
	ListenerSocket->HasPendingConnection(Pending);
	if (Pending)
	{		
		if (ConnectionSocket)  // Already have a Connection? destroy previous
		{
			ConnectionSocket->Close();
			ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ConnectionSocket);
		}
		//New Connection receive
		ConnectionSocket = ListenerSocket->Accept(*RemoteAddress, TEXT("Received Socket Connection"));
		if (ConnectionSocket != NULL)
		{
			//Global cache of current Remote Address
			RemoteAddressForConnection = FIPv4Endpoint(RemoteAddress);

			//can thread this too			
			FTimerDelegate TimerCallback;  // bind a basic functionto a TimerDelegate to set a timer without needing an object
			TimerCallback.BindLambda([]
			{
				TCPSocketListener();
			});
			GEditor->GetTimerManager()->SetTimer(TCPSocketListenerTimerHandle, TimerCallback, GLOBAL_SERVER_HEARTBEAT, true, 0.0f);
		}
	}
}


// TCP Socket Listener
// Receives the message from the socket and sends it to be executed
void TCPSocketListener()
{
	if (!ConnectionSocket) return;

	TArray<uint8> ReceivedData;  //Binary Array

	uint32 Size;
	while (ConnectionSocket->HasPendingData(Size))
	{
		ReceivedData.Init(FMath::Min(Size, 65507u), Size);

		int32 Read = 0;
		ConnectionSocket->Recv(ReceivedData.GetData(), ReceivedData.Num(), Read);
	}

	if (ReceivedData.Num() <= 0) return;

	//	String From Binary Array
	const FString ReceivedUE4String = StringFromBinaryArray(ReceivedData);

	// GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, FString::Printf(TEXT("TCP Command Received ~> %s"), *ReceivedUE4String));	
	// UE_LOG(LogPromethean, Display, TEXT("Received and added TCP Command to Queue: %s"), *ReceivedUE4String);
	ExecuteTCPCommand(ReceivedUE4String);	
	/*
	GlobalCommandStrArray.Add(ReceivedUE4String);  // add to global array
	if (!isCommandExecuting)  // if already executing we should just pile on
	{
		MetaExecuteTCPCommand();
	}
	*/
	
};

void TCPCloseConnection()
{
	UWorld* World = GetEditorWorld();  // safe function to get editor world

	World->GetTimerManager().ClearTimer(TCPConnectionListenerTimerHandle);
	World->GetTimerManager().ClearTimer(TCPSocketListenerTimerHandle);

	if (ConnectionSocket != NULL) {
		ConnectionSocket->Close();
	}
	if (ListenerSocket != NULL) {
		ListenerSocket->Close();
	}
}


void TCPSend(FString ToSend) {

	/*  // - original rama example - starts sending corrupted messages over certain message size
	TCHAR *SerializedChar = ToSend.GetCharArray().GetData();
	int32 Size = FCString::Strlen(SerializedChar);
	int32 Sent = 0;
	uint8* ResultChars = (uint8*)TCHAR_TO_UTF8(SerializedChar);
	*/

	if (ConnectionSocket == nullptr) {
		return;
	}
	if (ToSend.IsEmpty()) {
		ToSend = FString("None");  // an empty string can't be sent so sending the word "None" instead
	}

	// m2u example - works great
	int32 DestLen = TStringConvert<TCHAR, ANSICHAR>::ConvertedLength(*ToSend, ToSend.Len());
	uint8* Dest = new uint8[DestLen + 1];
	TStringConvert<TCHAR, ANSICHAR>::Convert((ANSICHAR*)Dest, DestLen, *ToSend, ToSend.Len());
	Dest[DestLen] = '\0';
	int32 BytesSent = 0;

	if (!ConnectionSocket->Send(Dest, DestLen, BytesSent)) {  //if (!ConnectionSocket->Send(ResultChars, Size, Sent))  // - old rama code
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Error sending Promethean message")));
	}
}

// --------------------------------------------------------------------
// --- Misc Formatting Functions
// --------------------------------------------------------------------

//Format IP String as Number Parts
bool FormatIP4ToNumber(const FString& TheIP, uint8(&Out)[4])
{
	//IP Formatting
    // TheIP = TheIP.Replace(TEXT(" "), TEXT(""));

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	//					   IP 4 Parts

	//String Parts
	TArray<FString> Parts;
	TheIP.ParseIntoArray(Parts, TEXT("."), true);
	if (Parts.Num() != 4)
		return false;

	//String to Number Parts
	for (int32 i = 0; i < 4; ++i)
	{
		Out[i] = FCString::Atoi(*Parts[i]);
	}

	return true;
}

//String From Binary Array
FString StringFromBinaryArray(TArray<uint8> BinaryArray)
{

	//Create a string from a byte array!
	const std::string cstr(reinterpret_cast<const char*>(BinaryArray.GetData()), BinaryArray.Num());

	return FString(cstr.c_str());

	//BinaryArray.Add(0); // Add 0 termination. Even if the string is already 0-terminated, it doesn't change the results.
	// Create a string from a byte array. The string is expected to be 0 terminated (i.e. a byte set to 0).
	// Use UTF8_TO_TCHAR if needed.
	// If you happen to know the data is UTF-16 (USC2) formatted, you do not need any conversion to begin with.
	// Otherwise you might have to write your own conversion algorithm to convert between multilingual UTF-16 planes.
	//return FString(ANSI_TO_TCHAR(reinterpret_cast<const char*>(BinaryArray.GetData())));
}
