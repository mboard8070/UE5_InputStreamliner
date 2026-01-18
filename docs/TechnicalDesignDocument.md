# Input Streamliner - Technical Design Document

## 1. Overview

This document describes the technical architecture, data structures, and implementation approach for Input Streamliner, an Unreal Engine 5 editor plugin that generates multiplatform input systems from natural language descriptions.

---

## 2. Architecture

### 2.1 High-Level Architecture

```
┌─────────────────────────────────────────────────────────────────────────┐
│                      INPUT STREAMLINER PLUGIN                            │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│  ┌──────────────────┐    ┌──────────────────┐    ┌──────────────────┐  │
│  │   PRESENTATION   │    │   PROCESSING     │    │   GENERATION     │  │
│  │      LAYER       │───▶│     LAYER        │───▶│     LAYER        │  │
│  └──────────────────┘    └──────────────────┘    └──────────────────┘  │
│           │                       │                       │             │
│  ┌────────┴────────┐    ┌────────┴────────┐    ┌────────┴────────┐    │
│  │ WBP_Streamliner │    │ LLMIntentParser │    │InputAssetGen    │    │
│  │ WBP_RebindUI    │    │ ActionValidator │    │TouchControlGen  │    │
│  │ WBP_TouchLayout │    │ TemplateResolver│    │RebindUIGen      │    │
│  └─────────────────┘    └─────────────────┘    │CodeGen (C++/BP) │    │
│                                                 └─────────────────┘    │
│                                                                          │
│  ┌──────────────────────────────────────────────────────────────────┐  │
│  │                        DATA LAYER                                  │  │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐               │  │
│  │  │InputAction  │  │Platform     │  │TouchControl │               │  │
│  │  │Definition   │  │Template     │  │Template     │               │  │
│  │  └─────────────┘  └─────────────┘  └─────────────┘               │  │
│  └──────────────────────────────────────────────────────────────────┘  │
│                                                                          │
└─────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
                    ┌───────────────────────────────┐
                    │      EXTERNAL SERVICES        │
                    │  ┌─────────────────────────┐  │
                    │  │  Ollama (Local LLM)     │  │
                    │  │  localhost:11434        │  │
                    │  └─────────────────────────┘  │
                    └───────────────────────────────┘
```

### 2.2 Layer Responsibilities

| Layer | Responsibility | Key Classes |
|-------|----------------|-------------|
| Presentation | User interface, wizard flow, previews | Editor Utility Widgets (UMG) |
| Processing | NLP parsing, validation, template resolution | C++ subsystems |
| Generation | Asset creation, code generation, widget spawning | C++ asset factories |
| Data | Structured definitions, templates, configuration | USTRUCTs, Data Assets |

---

## 3. Data Structures

### 3.1 Core Definitions

```cpp
// InputActionDefinition.h

UENUM(BlueprintType)
enum class EInputActionType : uint8
{
    Bool        UMETA(DisplayName = "Button (Bool)"),
    Axis1D      UMETA(DisplayName = "1D Axis (Float)"),
    Axis2D      UMETA(DisplayName = "2D Axis (Vector2D)"),
    Axis3D      UMETA(DisplayName = "3D Axis (Vector)")
};

UENUM(BlueprintType)
enum class ETargetPlatform : uint8
{
    None        = 0,
    PC_Keyboard = 1 << 0,
    PC_Gamepad  = 1 << 1,
    Mac         = 1 << 2,
    iOS         = 1 << 3,
    Android     = 1 << 4,
    All         = 0xFF
};
ENUM_CLASS_FLAGS(ETargetPlatform);

USTRUCT(BlueprintType)
struct FInputActionDefinition
{
    GENERATED_BODY()

    // Unique identifier for this action
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName ActionName;

    // Human-readable description
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString DisplayName;

    // Action value type
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EInputActionType ActionType = EInputActionType::Bool;

    // Target platforms for this action
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Bitmask, BitmaskEnum = ETargetPlatform))
    int32 TargetPlatforms = static_cast<int32>(ETargetPlatform::All);

    // Whether players can rebind this action
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bAllowRebinding = true;

    // Category for UI grouping
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Category = TEXT("General");

    // Platform-specific bindings
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TMap<ETargetPlatform, FPlatformBinding> PlatformBindings;
};
```

### 3.2 Platform Binding Structure

```cpp
// PlatformBinding.h

USTRUCT(BlueprintType)
struct FKeyBinding
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FKey Key;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FKey> Modifiers; // Shift, Ctrl, Alt

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSubclassOf<UInputTrigger> TriggerClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<TSubclassOf<UInputModifier>> ModifierClasses;
};

USTRUCT(BlueprintType)
struct FPlatformBinding
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FKeyBinding> Bindings;

    // For mobile: associated touch control type
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ETouchControlType TouchControlType = ETouchControlType::None;
};
```

### 3.3 Touch Control Definitions

```cpp
// TouchControlDefinition.h

UENUM(BlueprintType)
enum class ETouchControlType : uint8
{
    None,
    VirtualJoystick_Fixed,
    VirtualJoystick_Floating,
    VirtualButton,
    VirtualDPad,
    RadialMenu,
    TouchRegion,
    GestureZone
};

UENUM(BlueprintType)
enum class EGestureType : uint8
{
    Tap,
    DoubleTap,
    LongPress,
    Swipe_4Dir,
    Swipe_8Dir,
    Pinch,
    Rotate,
    TwoFingerTap
};

USTRUCT(BlueprintType)
struct FTouchControlDefinition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ETouchControlType ControlType;

    // Screen position (0-1 normalized, origin bottom-left)
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector2D ScreenPosition = FVector2D(0.15f, 0.3f);

    // Size (0-1 normalized)
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector2D Size = FVector2D(0.2f, 0.2f);

    // Linked input action
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName LinkedActionName;

    // Visual customization
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Opacity = 0.7f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bRespectSafeArea = true;

    // Joystick-specific
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float DeadZone = 0.15f;

    // Radial menu specific
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FRadialMenuEntry> RadialEntries;

    // Gesture-specific
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EGestureType GestureType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float SwipeThreshold = 50.0f; // pixels
};

USTRUCT(BlueprintType)
struct FRadialMenuEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName ActionName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    UTexture2D* Icon;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText Label;
};
```

### 3.4 Gyroscope Configuration

```cpp
// GyroConfig.h

USTRUCT(BlueprintType)
struct FGyroConfiguration
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bEnabled = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName LinkedActionName; // Typically "Look" or "Aim"

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Sensitivity = 1.0f;

    // Which axis to use for horizontal look
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EGyroAxis HorizontalAxis = EGyroAxis::Yaw;

    // Which axis to use for vertical look
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EGyroAxis VerticalAxis = EGyroAxis::Pitch;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bInvertHorizontal = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bInvertVertical = false;

    // Only active when this action is held (e.g., ADS)
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName ActivationAction;
};

UENUM(BlueprintType)
enum class EGyroAxis : uint8
{
    Pitch,  // Tilt forward/back
    Yaw,    // Rotate left/right
    Roll    // Tilt left/right
};
```

### 3.5 Complete Input Configuration

```cpp
// InputStreamlinerConfiguration.h

USTRUCT(BlueprintType)
struct FInputStreamlinerConfiguration
{
    GENERATED_BODY()

    // Project name for generated assets
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString ProjectPrefix = TEXT("Game");

    // All defined input actions
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FInputActionDefinition> Actions;

    // Mobile touch controls
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FTouchControlDefinition> TouchControls;

    // Gyroscope configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGyroConfiguration GyroConfig;

    // Generation options
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ECodeGenerationType CodeGenType = ECodeGenerationType::Blueprint;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bGenerateRebindingUI = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bGenerateTouchControls = true;

    // Output paths
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString InputActionsPath = TEXT("/Game/Input/Actions");

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString MappingContextsPath = TEXT("/Game/Input/Contexts");

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString WidgetsPath = TEXT("/Game/UI/Input");

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString GeneratedCodePath = TEXT("Source/[ProjectName]/Input");
};

UENUM(BlueprintType)
enum class ECodeGenerationType : uint8
{
    Blueprint,
    CPlusPlus,
    Both
};
```

---

## 4. LLM Integration

### 4.1 Intent Parsing Architecture

```
┌─────────────────┐     ┌─────────────────┐     ┌─────────────────┐
│  Natural Lang   │────▶│  LLM Service    │────▶│  JSON Response  │
│  Input          │     │  (Ollama)       │     │                 │
└─────────────────┘     └─────────────────┘     └─────────────────┘
        │                       │                       │
        │                       │                       │
"Third-person           POST /api/generate        {
 action game                    │                   "actions": [
 with dodge,            ┌───────┴───────┐           {
 sprint, and            │ System Prompt │             "name": "Move",
 lock-on"               │ + Few-shot    │             "type": "Axis2D",
                        │ + User Input  │             "bindings": {...}
                        └───────────────┘           },
                                                    ...
                                                  ]
                                                }
```

### 4.2 System Prompt Design

```cpp
// LLMPrompts.h

const FString SYSTEM_PROMPT = TEXT(R"(
You are an Unreal Engine 5 input configuration assistant. You parse natural language
descriptions of game input needs into structured JSON.

OUTPUT FORMAT:
{
  "actions": [
    {
      "name": "ActionName",
      "displayName": "Human Readable Name",
      "type": "Bool|Axis1D|Axis2D",
      "category": "Movement|Combat|UI|Camera",
      "allowRebinding": true,
      "bindings": {
        "PC_Keyboard": [{"key": "W"}, {"key": "A"}, {"key": "S"}, {"key": "D"}],
        "PC_Gamepad": [{"key": "Gamepad_LeftStick"}],
        "iOS": {"touchControl": "VirtualJoystick_Fixed"},
        "Android": {"touchControl": "VirtualJoystick_Fixed"}
      }
    }
  ],
  "gyro": {
    "enabled": false,
    "linkedAction": "Look",
    "activationAction": "Aim"
  }
}

KEY NAMES: Use Unreal Engine FKey names exactly:
- Keyboard: A-Z, Zero-Nine, SpaceBar, LeftShift, LeftControl, Tab, Escape
- Mouse: LeftMouseButton, RightMouseButton, MouseX, MouseY, MouseWheelUp
- Gamepad: Gamepad_LeftStick, Gamepad_RightStick, Gamepad_FaceButton_Bottom (A/Cross),
  Gamepad_FaceButton_Right (B/Circle), Gamepad_LeftTrigger, Gamepad_RightTrigger,
  Gamepad_LeftShoulder, Gamepad_RightShoulder

TOUCH CONTROLS: VirtualJoystick_Fixed, VirtualJoystick_Floating, VirtualButton,
VirtualDPad, RadialMenu, TouchRegion, GestureZone

COMMON PATTERNS:
- Movement: Axis2D, WASD + Left Stick + Virtual Joystick
- Look/Camera: Axis2D, Mouse XY + Right Stick + Gyro/Touch Region
- Jump: Bool, Space + Face Button Bottom + Virtual Button
- Sprint: Bool (hold), Left Shift + Left Stick Click
- Interact: Bool, E + Face Button Right
- Aim/ADS: Bool (hold), Right Mouse + Left Trigger
- Fire: Bool, Left Mouse + Right Trigger + Virtual Button
- Dodge: Bool, Left Alt + Face Button Right (double tap often)
- Weapon Wheel: RadialMenu, Tab (hold) + Right Bumper (hold)

ONLY output valid JSON. No explanations.
)");
```

### 4.3 Few-Shot Examples

```cpp
const FString FEW_SHOT_EXAMPLES = TEXT(R"(
USER: basic platformer controls
ASSISTANT: {"actions":[{"name":"Move","displayName":"Move","type":"Axis2D","category":"Movement","allowRebinding":true,"bindings":{"PC_Keyboard":[{"key":"A","axis":"-X"},{"key":"D","axis":"+X"}],"PC_Gamepad":[{"key":"Gamepad_LeftStick"}],"iOS":{"touchControl":"VirtualJoystick_Fixed"},"Android":{"touchControl":"VirtualJoystick_Fixed"}}},{"name":"Jump","displayName":"Jump","type":"Bool","category":"Movement","allowRebinding":true,"bindings":{"PC_Keyboard":[{"key":"SpaceBar"}],"PC_Gamepad":[{"key":"Gamepad_FaceButton_Bottom"}],"iOS":{"touchControl":"VirtualButton"},"Android":{"touchControl":"VirtualButton"}}}]}

USER: FPS with ADS and weapon switching
ASSISTANT: {"actions":[{"name":"Move","displayName":"Move","type":"Axis2D","category":"Movement","allowRebinding":true,"bindings":{"PC_Keyboard":[{"key":"W","axis":"+Y"},{"key":"S","axis":"-Y"},{"key":"A","axis":"-X"},{"key":"D","axis":"+X"}],"PC_Gamepad":[{"key":"Gamepad_LeftStick"}],"iOS":{"touchControl":"VirtualJoystick_Fixed"},"Android":{"touchControl":"VirtualJoystick_Fixed"}}},{"name":"Look","displayName":"Look","type":"Axis2D","category":"Camera","allowRebinding":false,"bindings":{"PC_Keyboard":[{"key":"MouseXY"}],"PC_Gamepad":[{"key":"Gamepad_RightStick"}],"iOS":{"touchControl":"TouchRegion"},"Android":{"touchControl":"TouchRegion"}}},{"name":"Fire","displayName":"Fire","type":"Bool","category":"Combat","allowRebinding":true,"bindings":{"PC_Keyboard":[{"key":"LeftMouseButton"}],"PC_Gamepad":[{"key":"Gamepad_RightTrigger"}],"iOS":{"touchControl":"VirtualButton"},"Android":{"touchControl":"VirtualButton"}}},{"name":"Aim","displayName":"Aim Down Sights","type":"Bool","category":"Combat","allowRebinding":true,"bindings":{"PC_Keyboard":[{"key":"RightMouseButton"}],"PC_Gamepad":[{"key":"Gamepad_LeftTrigger"}],"iOS":{"touchControl":"VirtualButton"},"Android":{"touchControl":"VirtualButton"}}},{"name":"WeaponWheel","displayName":"Weapon Wheel","type":"Bool","category":"Combat","allowRebinding":true,"bindings":{"PC_Keyboard":[{"key":"Tab","trigger":"Hold"}],"PC_Gamepad":[{"key":"Gamepad_RightShoulder","trigger":"Hold"}],"iOS":{"touchControl":"RadialMenu"},"Android":{"touchControl":"RadialMenu"}}}],"gyro":{"enabled":true,"linkedAction":"Look","activationAction":"Aim"}}
)");
```

### 4.4 LLM Service Class

```cpp
// LLMIntentParser.h

DECLARE_DELEGATE_TwoParams(FOnParseComplete, bool /*bSuccess*/, const FInputStreamlinerConfiguration& /*Config*/);

UCLASS()
class INPUTSTREAMLINER_API ULLMIntentParser : public UObject
{
    GENERATED_BODY()

public:
    // Configure the LLM endpoint
    UFUNCTION(BlueprintCallable, Category = "Input Streamliner|LLM")
    void SetEndpoint(const FString& URL, int32 Port = 11434);

    // Set the model to use
    UFUNCTION(BlueprintCallable, Category = "Input Streamliner|LLM")
    void SetModel(const FString& ModelName = TEXT("nemotron:8b"));

    // Parse natural language asynchronously
    UFUNCTION(BlueprintCallable, Category = "Input Streamliner|LLM")
    void ParseInputDescriptionAsync(const FString& Description, FOnParseComplete OnComplete);

    // Synchronous version (blocks editor, use sparingly)
    UFUNCTION(BlueprintCallable, Category = "Input Streamliner|LLM")
    bool ParseInputDescription(const FString& Description, FInputStreamlinerConfiguration& OutConfig);

private:
    FString BuildPrompt(const FString& UserDescription) const;
    bool ParseJSONResponse(const FString& JSONString, FInputStreamlinerConfiguration& OutConfig) const;

    FString EndpointURL = TEXT("http://localhost");
    int32 EndpointPort = 11434;
    FString ModelName = TEXT("nemotron:8b");
};
```

### 4.5 HTTP Request Implementation

```cpp
// LLMIntentParser.cpp

void ULLMIntentParser::ParseInputDescriptionAsync(const FString& Description, FOnParseComplete OnComplete)
{
    TSharedRef<IHttpRequest> Request = FHttpModule::Get().CreateRequest();

    Request->SetURL(FString::Printf(TEXT("%s:%d/api/generate"), *EndpointURL, EndpointPort));
    Request->SetVerb(TEXT("POST"));
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));

    // Build request body
    TSharedPtr<FJsonObject> RequestBody = MakeShareable(new FJsonObject());
    RequestBody->SetStringField(TEXT("model"), ModelName);
    RequestBody->SetStringField(TEXT("prompt"), BuildPrompt(Description));
    RequestBody->SetBoolField(TEXT("stream"), false);

    FString RequestString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestString);
    FJsonSerializer::Serialize(RequestBody.ToSharedRef(), Writer);

    Request->SetContentAsString(RequestString);

    Request->OnProcessRequestComplete().BindLambda(
        [this, OnComplete](FHttpRequestPtr Req, FHttpResponsePtr Resp, bool bSuccess)
        {
            FInputStreamlinerConfiguration Config;
            bool bParseSuccess = false;

            if (bSuccess && Resp.IsValid() && Resp->GetResponseCode() == 200)
            {
                // Extract response from Ollama format
                TSharedPtr<FJsonObject> JsonResponse;
                TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Resp->GetContentAsString());

                if (FJsonSerializer::Deserialize(Reader, JsonResponse))
                {
                    FString ResponseText = JsonResponse->GetStringField(TEXT("response"));
                    bParseSuccess = ParseJSONResponse(ResponseText, Config);
                }
            }

            OnComplete.ExecuteIfBound(bParseSuccess, Config);
        });

    Request->ProcessRequest();
}

FString ULLMIntentParser::BuildPrompt(const FString& UserDescription) const
{
    return FString::Printf(TEXT("%s\n\n%s\n\nUSER: %s\nASSISTANT:"),
        *SYSTEM_PROMPT,
        *FEW_SHOT_EXAMPLES,
        *UserDescription);
}
```

---

## 5. Dynamic Input Management

### 5.1 Overview

Input Streamliner supports dynamic addition and removal of input actions at any time. This allows developers to iteratively build their input system without regenerating everything from scratch.

### 5.2 Input Management Interface

```cpp
// InputStreamlinerManager.h

UCLASS(BlueprintType)
class INPUTSTREAMLINER_API UInputStreamlinerManager : public UEditorSubsystem
{
    GENERATED_BODY()

public:
    // Add a new input action to the configuration
    UFUNCTION(BlueprintCallable, Category = "Input Streamliner|Management")
    bool AddInputAction(const FInputActionDefinition& NewAction);

    // Remove an input action by name
    UFUNCTION(BlueprintCallable, Category = "Input Streamliner|Management")
    bool RemoveInputAction(FName ActionName);

    // Update an existing input action
    UFUNCTION(BlueprintCallable, Category = "Input Streamliner|Management")
    bool UpdateInputAction(FName ActionName, const FInputActionDefinition& UpdatedAction);

    // Duplicate an existing action as a starting point
    UFUNCTION(BlueprintCallable, Category = "Input Streamliner|Management")
    bool DuplicateInputAction(FName SourceActionName, FName NewActionName);

    // Reorder actions (for UI display)
    UFUNCTION(BlueprintCallable, Category = "Input Streamliner|Management")
    void ReorderAction(FName ActionName, int32 NewIndex);

    // Bulk operations
    UFUNCTION(BlueprintCallable, Category = "Input Streamliner|Management")
    void RemoveAllActions();

    UFUNCTION(BlueprintCallable, Category = "Input Streamliner|Management")
    void ImportActionsFromContext(UInputMappingContext* ExistingContext);

    // Get current configuration
    UFUNCTION(BlueprintPure, Category = "Input Streamliner|Management")
    const FInputStreamlinerConfiguration& GetCurrentConfiguration() const { return CurrentConfig; }

    // Get action by name
    UFUNCTION(BlueprintPure, Category = "Input Streamliner|Management")
    bool GetActionByName(FName ActionName, FInputActionDefinition& OutAction) const;

    // Get all action names
    UFUNCTION(BlueprintPure, Category = "Input Streamliner|Management")
    TArray<FName> GetAllActionNames() const;

    // Validation
    UFUNCTION(BlueprintPure, Category = "Input Streamliner|Management")
    bool IsActionNameUnique(FName ActionName) const;

    UFUNCTION(BlueprintPure, Category = "Input Streamliner|Management")
    bool ValidateAction(const FInputActionDefinition& Action, FString& OutError) const;

    // Events
    UPROPERTY(BlueprintAssignable, Category = "Input Streamliner|Management")
    FOnActionAdded OnActionAdded;

    UPROPERTY(BlueprintAssignable, Category = "Input Streamliner|Management")
    FOnActionRemoved OnActionRemoved;

    UPROPERTY(BlueprintAssignable, Category = "Input Streamliner|Management")
    FOnActionUpdated OnActionUpdated;

    UPROPERTY(BlueprintAssignable, Category = "Input Streamliner|Management")
    FOnConfigurationChanged OnConfigurationChanged;

private:
    UPROPERTY()
    FInputStreamlinerConfiguration CurrentConfig;

    void NotifyConfigurationChanged();
    void SaveConfigurationToDisk();
    void LoadConfigurationFromDisk();
};
```

### 5.3 Implementation Details

```cpp
// InputStreamlinerManager.cpp

bool UInputStreamlinerManager::AddInputAction(const FInputActionDefinition& NewAction)
{
    // Validate the action
    FString ValidationError;
    if (!ValidateAction(NewAction, ValidationError))
    {
        UE_LOG(LogInputStreamliner, Warning, TEXT("Failed to add action: %s"), *ValidationError);
        return false;
    }

    // Check for duplicate names
    if (!IsActionNameUnique(NewAction.ActionName))
    {
        UE_LOG(LogInputStreamliner, Warning, TEXT("Action name '%s' already exists"), *NewAction.ActionName.ToString());
        return false;
    }

    // Add to configuration
    CurrentConfig.Actions.Add(NewAction);

    // Broadcast events
    OnActionAdded.Broadcast(NewAction.ActionName);
    NotifyConfigurationChanged();

    return true;
}

bool UInputStreamlinerManager::RemoveInputAction(FName ActionName)
{
    int32 IndexToRemove = CurrentConfig.Actions.IndexOfByPredicate(
        [ActionName](const FInputActionDefinition& Action)
        {
            return Action.ActionName == ActionName;
        });

    if (IndexToRemove == INDEX_NONE)
    {
        UE_LOG(LogInputStreamliner, Warning, TEXT("Action '%s' not found"), *ActionName.ToString());
        return false;
    }

    // Remove associated touch controls
    CurrentConfig.TouchControls.RemoveAll(
        [ActionName](const FTouchControlDefinition& Control)
        {
            return Control.LinkedActionName == ActionName;
        });

    // Remove the action
    CurrentConfig.Actions.RemoveAt(IndexToRemove);

    // Broadcast events
    OnActionRemoved.Broadcast(ActionName);
    NotifyConfigurationChanged();

    return true;
}

bool UInputStreamlinerManager::UpdateInputAction(FName ActionName, const FInputActionDefinition& UpdatedAction)
{
    FInputActionDefinition* ExistingAction = CurrentConfig.Actions.FindByPredicate(
        [ActionName](const FInputActionDefinition& Action)
        {
            return Action.ActionName == ActionName;
        });

    if (!ExistingAction)
    {
        UE_LOG(LogInputStreamliner, Warning, TEXT("Action '%s' not found"), *ActionName.ToString());
        return false;
    }

    // If name changed, check for conflicts
    if (UpdatedAction.ActionName != ActionName && !IsActionNameUnique(UpdatedAction.ActionName))
    {
        UE_LOG(LogInputStreamliner, Warning, TEXT("New action name '%s' already exists"),
            *UpdatedAction.ActionName.ToString());
        return false;
    }

    // Update touch controls if name changed
    if (UpdatedAction.ActionName != ActionName)
    {
        for (FTouchControlDefinition& Control : CurrentConfig.TouchControls)
        {
            if (Control.LinkedActionName == ActionName)
            {
                Control.LinkedActionName = UpdatedAction.ActionName;
            }
        }
    }

    // Apply update
    *ExistingAction = UpdatedAction;

    // Broadcast events
    OnActionUpdated.Broadcast(ActionName);
    NotifyConfigurationChanged();

    return true;
}
```

### 5.4 Asset Synchronization

When inputs are added or removed, the generated assets need to stay in sync:

```cpp
// InputAssetSynchronizer.h

UCLASS()
class INPUTSTREAMLINER_API UInputAssetSynchronizer : public UObject
{
    GENERATED_BODY()

public:
    // Sync generated assets with current configuration
    UFUNCTION(BlueprintCallable, Category = "Input Streamliner|Sync")
    void SynchronizeAssets(const FInputStreamlinerConfiguration& Config);

    // Options for sync behavior
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sync")
    bool bDeleteOrphanedAssets = false; // Delete assets for removed actions

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sync")
    bool bUpdateExistingAssets = true; // Update assets when bindings change

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sync")
    bool bPreserveCustomizations = true; // Don't overwrite manual edits

private:
    void CreateMissingAssets(const FInputStreamlinerConfiguration& Config);
    void UpdateModifiedAssets(const FInputStreamlinerConfiguration& Config);
    void RemoveOrphanedAssets(const FInputStreamlinerConfiguration& Config);
    void UpdateMappingContexts(const FInputStreamlinerConfiguration& Config);

    TSet<FName> GetExistingAssetNames() const;
    TSet<FName> GetConfigActionNames(const FInputStreamlinerConfiguration& Config) const;
};
```

### 5.5 Undo/Redo Support

```cpp
// InputStreamlinerTransaction.h

// Supports editor undo/redo for all input modifications
class FInputStreamlinerTransaction
{
public:
    static void BeginTransaction(const FText& Description);
    static void EndTransaction();

    // Snapshot current state for undo
    static void RecordState(const FInputStreamlinerConfiguration& Config);
    static void RestoreState(FInputStreamlinerConfiguration& OutConfig);

private:
    static TArray<FInputStreamlinerConfiguration> UndoStack;
    static TArray<FInputStreamlinerConfiguration> RedoStack;
};

// Usage in manager:
bool UInputStreamlinerManager::AddInputAction(const FInputActionDefinition& NewAction)
{
    FScopedTransaction Transaction(LOCTEXT("AddInputAction", "Add Input Action"));
    FInputStreamlinerTransaction::RecordState(CurrentConfig);

    // ... add logic ...

    return true;
}
```

### 5.6 UI Integration

The editor widget provides intuitive controls for dynamic management:

```
┌─────────────────────────────────────────────────────────────────┐
│  INPUT ACTIONS                                    [+ Add Action] │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │ ≡  Move (Axis2D)                         [Edit] [Delete] │   │
│  │    PC: WASD | Gamepad: Left Stick | Mobile: Joystick    │   │
│  └─────────────────────────────────────────────────────────┘   │
│                                                                  │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │ ≡  Jump (Bool)                           [Edit] [Delete] │   │
│  │    PC: Space | Gamepad: A/Cross | Mobile: Button        │   │
│  └─────────────────────────────────────────────────────────┘   │
│                                                                  │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │ ≡  Fire (Bool)                           [Edit] [Delete] │   │
│  │    PC: LMB | Gamepad: RT | Mobile: Button               │   │
│  └─────────────────────────────────────────────────────────┘   │
│                                                                  │
│  [Duplicate Selected]  [Remove Selected]  [Clear All]           │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

Features:
- **Drag handle (≡)** - Reorder actions via drag-and-drop
- **Inline summary** - See bindings at a glance
- **Quick edit** - Click to expand inline editor
- **Batch operations** - Multi-select for bulk delete/duplicate
- **Undo/Redo** - Full support via Ctrl+Z/Ctrl+Y

---

## 6. Asset Generation

### 6.1 Input Asset Generator

```cpp
// InputAssetGenerator.h

UCLASS()
class INPUTSTREAMLINER_API UInputAssetGenerator : public UObject
{
    GENERATED_BODY()

public:
    // Generate all input assets from configuration
    UFUNCTION(BlueprintCallable, Category = "Input Streamliner|Generation")
    bool GenerateInputAssets(const FInputStreamlinerConfiguration& Config, TArray<UObject*>& OutCreatedAssets);

    // Generate individual components
    UFUNCTION(BlueprintCallable, Category = "Input Streamliner|Generation")
    UInputAction* GenerateInputAction(const FInputActionDefinition& Definition, const FString& Path);

    UFUNCTION(BlueprintCallable, Category = "Input Streamliner|Generation")
    UInputMappingContext* GenerateMappingContext(
        ETargetPlatform Platform,
        const TArray<FInputActionDefinition>& Actions,
        const FString& Path);

private:
    void ConfigureActionTriggers(UInputAction* Action, const FInputActionDefinition& Definition);
    void AddMappingToContext(UInputMappingContext* Context, UInputAction* Action, const FKeyBinding& Binding);
};
```

### 6.2 Generation Implementation

```cpp
// InputAssetGenerator.cpp

UInputAction* UInputAssetGenerator::GenerateInputAction(
    const FInputActionDefinition& Definition,
    const FString& Path)
{
    // Create package
    FString PackagePath = Path / Definition.ActionName.ToString();
    UPackage* Package = CreatePackage(*PackagePath);

    // Create Input Action asset
    UInputAction* InputAction = NewObject<UInputAction>(
        Package,
        *FString::Printf(TEXT("IA_%s"), *Definition.ActionName.ToString()),
        RF_Public | RF_Standalone);

    // Set value type based on definition
    switch (Definition.ActionType)
    {
        case EInputActionType::Bool:
            InputAction->ValueType = EInputActionValueType::Boolean;
            break;
        case EInputActionType::Axis1D:
            InputAction->ValueType = EInputActionValueType::Axis1D;
            break;
        case EInputActionType::Axis2D:
            InputAction->ValueType = EInputActionValueType::Axis2D;
            break;
        case EInputActionType::Axis3D:
            InputAction->ValueType = EInputActionValueType::Axis3D;
            break;
    }

    // Configure triggers
    ConfigureActionTriggers(InputAction, Definition);

    // Mark package dirty and save
    Package->MarkPackageDirty();
    FAssetRegistryModule::AssetCreated(InputAction);

    // Save to disk
    FString PackageFileName = FPackageName::LongPackageNameToFilename(
        PackagePath, FPackageName::GetAssetPackageExtension());
    UPackage::SavePackage(Package, InputAction, RF_Public | RF_Standalone, *PackageFileName);

    return InputAction;
}

UInputMappingContext* UInputAssetGenerator::GenerateMappingContext(
    ETargetPlatform Platform,
    const TArray<FInputActionDefinition>& Actions,
    const FString& Path)
{
    FString PlatformName = GetPlatformName(Platform);
    FString ContextName = FString::Printf(TEXT("IMC_%s"), *PlatformName);
    FString PackagePath = Path / ContextName;

    UPackage* Package = CreatePackage(*PackagePath);

    UInputMappingContext* Context = NewObject<UInputMappingContext>(
        Package, *ContextName, RF_Public | RF_Standalone);

    // Add mappings for each action
    for (const FInputActionDefinition& ActionDef : Actions)
    {
        // Check if this action targets this platform
        if (!(ActionDef.TargetPlatforms & static_cast<int32>(Platform)))
        {
            continue;
        }

        // Find or load the generated Input Action
        FString ActionPath = FString::Printf(TEXT("%s/IA_%s"),
            *Config.InputActionsPath, *ActionDef.ActionName.ToString());
        UInputAction* Action = LoadObject<UInputAction>(nullptr, *ActionPath);

        if (!Action) continue;

        // Get bindings for this platform
        const FPlatformBinding* Binding = ActionDef.PlatformBindings.Find(Platform);
        if (!Binding) continue;

        // Add each key binding
        for (const FKeyBinding& KeyBind : Binding->Bindings)
        {
            AddMappingToContext(Context, Action, KeyBind);
        }
    }

    // Save
    Package->MarkPackageDirty();
    FAssetRegistryModule::AssetCreated(Context);

    FString PackageFileName = FPackageName::LongPackageNameToFilename(
        PackagePath, FPackageName::GetAssetPackageExtension());
    UPackage::SavePackage(Package, Context, RF_Public | RF_Standalone, *PackageFileName);

    return Context;
}
```

---

## 6. Touch Control Generation

### 6.1 Widget Templates

The plugin includes pre-built widget templates that are cloned and configured:

```
Plugins/InputStreamliner/Content/Templates/
├── WBP_VirtualJoystick_Template.uasset
├── WBP_VirtualButton_Template.uasset
├── WBP_VirtualDPad_Template.uasset
├── WBP_RadialMenu_Template.uasset
├── WBP_TouchRegion_Template.uasset
└── WBP_GestureZone_Template.uasset
```

### 6.2 Touch Control Generator

```cpp
// TouchControlGenerator.h

UCLASS()
class INPUTSTREAMLINER_API UTouchControlGenerator : public UObject
{
    GENERATED_BODY()

public:
    // Generate all touch control widgets
    UFUNCTION(BlueprintCallable, Category = "Input Streamliner|Touch")
    bool GenerateTouchControls(
        const TArray<FTouchControlDefinition>& Controls,
        const FString& OutputPath,
        TArray<UUserWidget*>& OutWidgets);

    // Generate the main touch HUD that contains all controls
    UFUNCTION(BlueprintCallable, Category = "Input Streamliner|Touch")
    UUserWidget* GenerateTouchHUD(
        const TArray<FTouchControlDefinition>& Controls,
        const FString& OutputPath);

private:
    UUserWidget* CloneTemplate(ETouchControlType Type, const FString& OutputPath, const FString& Name);
    void ConfigureJoystick(UUserWidget* Widget, const FTouchControlDefinition& Def);
    void ConfigureButton(UUserWidget* Widget, const FTouchControlDefinition& Def);
    void ConfigureRadialMenu(UUserWidget* Widget, const FTouchControlDefinition& Def);
    void ConfigureGestureZone(UUserWidget* Widget, const FTouchControlDefinition& Def);

    FVector2D CalculateSafeAreaPosition(const FVector2D& DesiredPosition, const FVector2D& Size);
};
```

### 6.3 Virtual Joystick Implementation

```cpp
// VirtualJoystickWidget.h (Template Base Class)

UCLASS()
class INPUTSTREAMLINERRUNTIME_API UVirtualJoystickWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    // Configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Joystick")
    bool bIsFloating = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Joystick")
    float DeadZone = 0.15f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Joystick")
    float VisualSize = 150.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Joystick")
    float ThumbSize = 50.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Joystick")
    float Opacity = 0.7f;

    // Linked Input Action
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Joystick")
    UInputAction* LinkedAction;

    // Current output value
    UPROPERTY(BlueprintReadOnly, Category = "Joystick")
    FVector2D CurrentValue;

protected:
    virtual FReply NativeOnTouchStarted(const FGeometry& Geometry, const FPointerEvent& Event) override;
    virtual FReply NativeOnTouchMoved(const FGeometry& Geometry, const FPointerEvent& Event) override;
    virtual FReply NativeOnTouchEnded(const FGeometry& Geometry, const FPointerEvent& Event) override;

private:
    void UpdateJoystickPosition(const FVector2D& TouchPosition);
    void InjectInputValue(const FVector2D& Value);

    FVector2D CenterPosition;
    int32 ActiveTouchIndex = -1;
};
```

### 6.4 Gyroscope Handler

```cpp
// GyroscopeHandler.h

UCLASS(BlueprintType, Blueprintable)
class INPUTSTREAMLINERRUNTIME_API UGyroscopeHandler : public UActorComponent
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gyro")
    FGyroConfiguration Config;

    UPROPERTY(BlueprintReadOnly, Category = "Gyro")
    bool bIsCalibrated = false;

    UFUNCTION(BlueprintCallable, Category = "Gyro")
    void StartCalibration();

    UFUNCTION(BlueprintCallable, Category = "Gyro")
    void SetEnabled(bool bEnabled);

    UFUNCTION(BlueprintPure, Category = "Gyro")
    FVector2D GetCurrentLookDelta() const;

protected:
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    void ProcessGyroInput();
    void InjectLookInput(const FVector2D& Delta);

    FRotator CalibrationOffset;
    FRotator LastRotation;
    bool bEnabled = false;
};
```

---

## 7. Rebinding System

### 7.1 Rebinding Manager

```cpp
// InputRebindingManager.h

UCLASS(BlueprintType)
class INPUTSTREAMLINERRUNTIME_API UInputRebindingManager : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    // Get current binding for an action
    UFUNCTION(BlueprintCallable, Category = "Rebinding")
    TArray<FKey> GetBindingsForAction(UInputAction* Action) const;

    // Start listening for new key
    UFUNCTION(BlueprintCallable, Category = "Rebinding")
    void StartRebinding(UInputAction* Action);

    // Cancel rebinding
    UFUNCTION(BlueprintCallable, Category = "Rebinding")
    void CancelRebinding();

    // Apply new binding
    UFUNCTION(BlueprintCallable, Category = "Rebinding")
    bool ApplyBinding(UInputAction* Action, FKey NewKey);

    // Check for conflicts
    UFUNCTION(BlueprintCallable, Category = "Rebinding")
    bool HasConflict(UInputAction* Action, FKey Key, UInputAction*& OutConflictingAction) const;

    // Reset to defaults
    UFUNCTION(BlueprintCallable, Category = "Rebinding")
    void ResetToDefaults(UInputAction* Action = nullptr);

    // Persistence
    UFUNCTION(BlueprintCallable, Category = "Rebinding")
    void SaveBindings();

    UFUNCTION(BlueprintCallable, Category = "Rebinding")
    void LoadBindings();

    // Events
    UPROPERTY(BlueprintAssignable, Category = "Rebinding")
    FOnRebindComplete OnRebindComplete;

    UPROPERTY(BlueprintAssignable, Category = "Rebinding")
    FOnKeyPressed OnAnyKeyPressed; // For capture UI

private:
    void OnKeyDown(const FKeyEvent& Event);
    void OnGamepadButton(const FKeyEvent& Event);

    UPROPERTY()
    UInputMappingContext* ActiveContext;

    UPROPERTY()
    UInputAction* PendingRebindAction;

    TMap<UInputAction*, TArray<FKey>> DefaultBindings;
    TMap<UInputAction*, TArray<FKey>> CurrentBindings;
};
```

### 7.2 Rebinding UI Generator

```cpp
// RebindingUIGenerator.h

UCLASS()
class INPUTSTREAMLINER_API URebindingUIGenerator : public UObject
{
    GENERATED_BODY()

public:
    // Generate complete rebinding settings UI
    UFUNCTION(BlueprintCallable, Category = "Input Streamliner|Rebinding")
    UUserWidget* GenerateRebindingUI(
        const FInputStreamlinerConfiguration& Config,
        const FString& OutputPath);

private:
    UWidget* CreateActionRow(const FInputActionDefinition& Action);
    UWidget* CreateCategoryHeader(const FString& Category);
    UWidget* CreateKeyBindButton(UInputAction* Action, int32 BindingIndex);
    UWidget* CreateResetButton(UInputAction* Action);
    UWidget* CreateSensitivitySlider(const FString& Label, float DefaultValue);
    UWidget* CreateKeyCaptureModal();
    UWidget* CreateConflictDialog();
};
```

### 7.3 Save Data Structure

```cpp
// InputBindingSaveData.h

USTRUCT(BlueprintType)
struct FActionBindingSave
{
    GENERATED_BODY()

    UPROPERTY()
    FName ActionName;

    UPROPERTY()
    TArray<FKey> Keys;
};

USTRUCT(BlueprintType)
struct FInputBindingSaveData
{
    GENERATED_BODY()

    UPROPERTY()
    int32 Version = 1;

    UPROPERTY()
    TArray<FActionBindingSave> Bindings;

    UPROPERTY()
    float MouseSensitivity = 1.0f;

    UPROPERTY()
    float GamepadSensitivity = 1.0f;

    UPROPERTY()
    float GyroSensitivity = 1.0f;

    UPROPERTY()
    bool bInvertY = false;

    UPROPERTY()
    bool bGyroEnabled = false;

    // Mobile touch layout overrides
    UPROPERTY()
    TMap<FName, FVector2D> TouchControlPositions;
};
```

---

## 8. Code Generation

### 8.1 C++ Code Generator

```cpp
// CodeGenerator.h

UCLASS()
class INPUTSTREAMLINER_API UCodeGenerator : public UObject
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Input Streamliner|CodeGen")
    bool GenerateCPPBindings(
        const FInputStreamlinerConfiguration& Config,
        const FString& OutputPath);

    UFUNCTION(BlueprintCallable, Category = "Input Streamliner|CodeGen")
    bool GenerateBlueprintBindings(
        const FInputStreamlinerConfiguration& Config,
        const FString& OutputPath);

private:
    FString GenerateHeaderFile(const FInputStreamlinerConfiguration& Config);
    FString GenerateSourceFile(const FInputStreamlinerConfiguration& Config);
    FString GenerateBindingFunction(const FInputActionDefinition& Action);
};
```

### 8.2 Generated C++ Example

```cpp
// Generated: GameInputComponent.h

#pragma once

#include "CoreMinimal.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "GameInputComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class YOURGAME_API UGameInputComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UGameInputComponent();

    virtual void SetupInputBindings(UEnhancedInputComponent* InputComponent);

    // Input Actions
    UPROPERTY(EditDefaultsOnly, Category = "Input")
    TObjectPtr<UInputAction> IA_Move;

    UPROPERTY(EditDefaultsOnly, Category = "Input")
    TObjectPtr<UInputAction> IA_Look;

    UPROPERTY(EditDefaultsOnly, Category = "Input")
    TObjectPtr<UInputAction> IA_Jump;

    UPROPERTY(EditDefaultsOnly, Category = "Input")
    TObjectPtr<UInputAction> IA_Sprint;

protected:
    // Binding callbacks - implement in your subclass or bind directly
    UFUNCTION()
    void OnMove(const FInputActionValue& Value);

    UFUNCTION()
    void OnLook(const FInputActionValue& Value);

    UFUNCTION()
    void OnJumpStarted(const FInputActionValue& Value);

    UFUNCTION()
    void OnJumpCompleted(const FInputActionValue& Value);

    UFUNCTION()
    void OnSprintStarted(const FInputActionValue& Value);

    UFUNCTION()
    void OnSprintCompleted(const FInputActionValue& Value);
};
```

```cpp
// Generated: GameInputComponent.cpp

#include "GameInputComponent.h"
#include "EnhancedInputSubsystems.h"

UGameInputComponent::UGameInputComponent()
{
    PrimaryComponentTick.bCanEverTick = false;

    // Load Input Actions
    static ConstructorHelpers::FObjectFinder<UInputAction> MoveAction(
        TEXT("/Game/Input/Actions/IA_Move"));
    IA_Move = MoveAction.Object;

    static ConstructorHelpers::FObjectFinder<UInputAction> LookAction(
        TEXT("/Game/Input/Actions/IA_Look"));
    IA_Look = LookAction.Object;

    // ... etc
}

void UGameInputComponent::SetupInputBindings(UEnhancedInputComponent* InputComponent)
{
    if (!InputComponent) return;

    // Movement (Axis2D - continuous)
    InputComponent->BindAction(IA_Move, ETriggerEvent::Triggered, this, &UGameInputComponent::OnMove);

    // Look (Axis2D - continuous)
    InputComponent->BindAction(IA_Look, ETriggerEvent::Triggered, this, &UGameInputComponent::OnLook);

    // Jump (Bool - started/completed)
    InputComponent->BindAction(IA_Jump, ETriggerEvent::Started, this, &UGameInputComponent::OnJumpStarted);
    InputComponent->BindAction(IA_Jump, ETriggerEvent::Completed, this, &UGameInputComponent::OnJumpCompleted);

    // Sprint (Bool - started/completed for hold)
    InputComponent->BindAction(IA_Sprint, ETriggerEvent::Started, this, &UGameInputComponent::OnSprintStarted);
    InputComponent->BindAction(IA_Sprint, ETriggerEvent::Completed, this, &UGameInputComponent::OnSprintCompleted);
}

void UGameInputComponent::OnMove(const FInputActionValue& Value)
{
    // Override in subclass or bind to delegate
    FVector2D Movement = Value.Get<FVector2D>();
}

// ... other implementations
```

---

## 9. Plugin Module Structure

### 9.1 Module Definition

```cpp
// InputStreamliner.Build.cs

using UnrealBuildTool;

public class InputStreamliner : ModuleRules
{
    public InputStreamliner(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "EnhancedInput",
            "UMG",
            "Slate",
            "SlateCore"
        });

        PrivateDependencyModuleNames.AddRange(new string[] {
            "UnrealEd",
            "Blutility",          // Editor Utility Widgets
            "HTTP",               // LLM API calls
            "Json",
            "JsonUtilities",
            "AssetTools",
            "ContentBrowser"
        });
    }
}
```

### 9.2 Plugin Descriptor

```json
// InputStreamliner.uplugin
{
    "FileVersion": 3,
    "Version": 1,
    "VersionName": "1.0",
    "FriendlyName": "Input Streamliner",
    "Description": "AI-powered multiplatform input configuration tool",
    "Category": "Editor",
    "CreatedBy": "Your Name",
    "CreatedByURL": "",
    "DocsURL": "",
    "MarketplaceURL": "",
    "CanContainContent": true,
    "IsBetaVersion": true,
    "IsExperimentalVersion": false,
    "Installed": false,
    "Modules": [
        {
            "Name": "InputStreamliner",
            "Type": "Editor",
            "LoadingPhase": "Default"
        },
        {
            "Name": "InputStreamlinerRuntime",
            "Type": "Runtime",
            "LoadingPhase": "Default"
        }
    ],
    "Plugins": [
        {
            "Name": "EnhancedInput",
            "Enabled": true
        }
    ]
}
```

---

## 10. File Structure

```
Plugins/InputStreamliner/
├── InputStreamliner.uplugin
├── Resources/
│   └── Icon128.png
├── Source/
│   ├── InputStreamliner/                    # Editor module
│   │   ├── InputStreamliner.Build.cs
│   │   ├── Public/
│   │   │   ├── InputStreamlinerModule.h
│   │   │   ├── InputStreamlinerManager.h       ← Dynamic add/remove management
│   │   │   ├── InputAssetSynchronizer.h        ← Asset sync when config changes
│   │   │   ├── InputStreamlinerTransaction.h   ← Undo/redo support
│   │   │   ├── InputActionDefinition.h
│   │   │   ├── PlatformBinding.h
│   │   │   ├── TouchControlDefinition.h
│   │   │   ├── InputStreamlinerConfiguration.h
│   │   │   ├── LLMIntentParser.h
│   │   │   ├── InputAssetGenerator.h
│   │   │   ├── TouchControlGenerator.h
│   │   │   ├── RebindingUIGenerator.h
│   │   │   └── CodeGenerator.h
│   │   └── Private/
│   │       ├── InputStreamlinerModule.cpp
│   │       ├── InputStreamlinerManager.cpp
│   │       ├── InputAssetSynchronizer.cpp
│   │       ├── LLMIntentParser.cpp
│   │       ├── InputAssetGenerator.cpp
│   │       ├── TouchControlGenerator.cpp
│   │       ├── RebindingUIGenerator.cpp
│   │       └── CodeGenerator.cpp
│   │
│   └── InputStreamlinerRuntime/             # Runtime module (for generated widgets)
│       ├── InputStreamlinerRuntime.Build.cs
│       ├── Public/
│       │   ├── InputStreamlinerRuntimeModule.h
│       │   ├── VirtualJoystickWidget.h
│       │   ├── VirtualButtonWidget.h
│       │   ├── RadialMenuWidget.h
│       │   ├── GestureDetector.h
│       │   ├── GyroscopeHandler.h
│       │   └── InputRebindingManager.h
│       └── Private/
│           └── [implementations]
│
└── Content/
    ├── UI/
    │   └── WBP_InputStreamliner.uasset      # Main editor widget
    ├── Templates/
    │   ├── WBP_VirtualJoystick.uasset
    │   ├── WBP_VirtualButton.uasset
    │   ├── WBP_VirtualDPad.uasset
    │   ├── WBP_RadialMenu.uasset
    │   ├── WBP_TouchRegion.uasset
    │   ├── WBP_RebindingUI.uasset
    │   └── WBP_TouchHUD.uasset
    └── Data/
        ├── DT_DefaultBindings_PC.uasset
        ├── DT_DefaultBindings_Gamepad.uasset
        ├── DT_DefaultBindings_Mobile.uasset
        └── DT_GenrePresets.uasset
```

---

## 11. Testing Strategy

### 11.1 Unit Tests

- LLM JSON parsing with various inputs
- Asset generation verification
- Binding conflict detection
- Save/load data integrity

### 11.2 Integration Tests

- Full flow: description to generated assets
- Cross-platform binding correctness
- Touch control functionality on device
- Rebinding persistence across sessions

### 11.3 Manual Testing

- UI/UX evaluation
- Generated code compilation
- Mobile device testing (iOS/Android)
- Performance profiling of LLM calls

---

## 12. Dependencies

| Dependency | Version | Purpose |
|------------|---------|---------|
| Unreal Engine | 5.3+ | Core platform |
| Enhanced Input Plugin | (bundled) | Input system |
| Ollama | Latest | Local LLM inference |
| Nemotron 8B | Latest | Language model |

---

## 13. Open Technical Questions

1. **Streaming responses**: Should LLM responses stream to show progress, or wait for complete response?
2. **Asset hot-reload**: Can generated assets be used immediately without editor restart?
3. **Mobile preview**: Is there value in an in-editor mobile input simulator?
4. **Version control**: How should generated assets handle merge conflicts?

---

*Document Version: 1.0*
*Last Updated: January 2025*
*Author: [Your Name]*
