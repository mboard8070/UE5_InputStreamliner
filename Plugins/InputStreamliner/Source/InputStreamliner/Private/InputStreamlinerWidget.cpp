// Copyright Epic Games, Inc. All Rights Reserved.

#include "InputStreamlinerWidget.h"
#include "InputStreamlinerModule.h"
#include "LLMIntentParser.h"
#include "InputAssetGenerator.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Misc/FileHelper.h"
#include "JsonObjectConverter.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/MultiLineEditableTextBox.h"
#include "Components/EditableTextBox.h"
#include "Components/ScrollBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Spacer.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetTree.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "ObjectTools.h"
#include "Components/ComboBoxString.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

UInputStreamlinerWidget::UInputStreamlinerWidget()
{
}

void UInputStreamlinerWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	UE_LOG(LogInputStreamliner, Warning, TEXT("NativePreConstruct called"));
}

void UInputStreamlinerWidget::NativeConstruct()
{
	Super::NativeConstruct();

	UE_LOG(LogInputStreamliner, Warning, TEXT("NativeConstruct called, ContentContainer=%s"), ContentContainer ? TEXT("bound") : TEXT("null"));

	// Create subsystems
	LLMParser = NewObject<ULLMIntentParser>(this);
	AssetGenerator = NewObject<UInputAssetGenerator>(this);

	// Set default LLM configuration
	LLMParser->SetEndpoint(
		CurrentConfiguration.LLMEndpointURL,
		CurrentConfiguration.LLMEndpointPort
	);
	LLMParser->SetModel(CurrentConfiguration.LLMModelName);

	// Build the UI (uses ContentContainer from Blueprint)
	BuildUI();

	// Fetch available models from Ollama
	RefreshModelList();

	UE_LOG(LogInputStreamliner, Log, TEXT("InputStreamlinerWidget constructed"));
}

void UInputStreamlinerWidget::NativeDestruct()
{
	Super::NativeDestruct();

	UE_LOG(LogInputStreamliner, Log, TEXT("InputStreamlinerWidget destructed"));
}

// ==================== LLM Integration ====================

void UInputStreamlinerWidget::ParseDescription(const FString& Description)
{
	if (!LLMParser)
	{
		OnLLMParseComplete.Broadcast(false, TEXT("LLM Parser not initialized"));
		return;
	}

	if (Description.IsEmpty())
	{
		OnLLMParseComplete.Broadcast(false, TEXT("Description is empty"));
		return;
	}

	UE_LOG(LogInputStreamliner, Log, TEXT("Parsing description: %s"), *Description.Left(100));

	FOnParseComplete Callback;
	Callback.BindDynamic(this, &UInputStreamlinerWidget::HandleLLMParseComplete);
	LLMParser->ParseInputDescriptionAsync(Description, Callback);
}

bool UInputStreamlinerWidget::IsParsingInProgress() const
{
	return LLMParser && LLMParser->IsParseInProgress();
}

void UInputStreamlinerWidget::TestLLMConnection()
{
	if (!LLMParser)
	{
		OnLLMParseComplete.Broadcast(false, TEXT("LLM Parser not initialized"));
		return;
	}

	FOnParseComplete Callback;
	Callback.BindDynamic(this, &UInputStreamlinerWidget::HandleLLMParseComplete);
	LLMParser->CheckConnection(Callback);
}

void UInputStreamlinerWidget::SetLLMEndpoint(const FString& URL, int32 Port, const FString& ModelName)
{
	CurrentConfiguration.LLMEndpointURL = URL;
	CurrentConfiguration.LLMEndpointPort = Port;
	CurrentConfiguration.LLMModelName = ModelName;

	if (LLMParser)
	{
		LLMParser->SetEndpoint(URL, Port);
		LLMParser->SetModel(ModelName);
	}

	BroadcastConfigurationUpdate();
}

void UInputStreamlinerWidget::HandleLLMParseComplete(bool bSuccess, const FString& ErrorMessage)
{
	if (bSuccess && LLMParser)
	{
		// Get the parsed configuration and merge/replace
		const FInputStreamlinerConfiguration& ParsedConfig = LLMParser->GetLastParsedConfiguration();

		// Add parsed actions to current configuration
		for (const FInputActionDefinition& Action : ParsedConfig.Actions)
		{
			// Check if action already exists
			if (!CurrentConfiguration.HasAction(Action.ActionName))
			{
				CurrentConfiguration.Actions.Add(Action);
			}
		}

		// Add parsed touch controls
		for (const FTouchControlDefinition& Control : ParsedConfig.TouchControls)
		{
			CurrentConfiguration.TouchControls.Add(Control);
		}

		BroadcastConfigurationUpdate();
		FString SuccessMsg = FString::Printf(TEXT("Parsed %d actions"), ParsedConfig.Actions.Num());
		SetStatusText(SuccessMsg, FLinearColor::Green);
		OnLLMParseComplete.Broadcast(true, SuccessMsg);
	}
	else
	{
		SetStatusText(FString::Printf(TEXT("Parse failed: %s"), *ErrorMessage), FLinearColor::Red);
		OnLLMParseComplete.Broadcast(false, ErrorMessage);
	}
}

// ==================== Configuration Management ====================

void UInputStreamlinerWidget::SetConfiguration(const FInputStreamlinerConfiguration& NewConfiguration)
{
	CurrentConfiguration = NewConfiguration;

	// Update LLM settings
	if (LLMParser)
	{
		LLMParser->SetEndpoint(CurrentConfiguration.LLMEndpointURL, CurrentConfiguration.LLMEndpointPort);
		LLMParser->SetModel(CurrentConfiguration.LLMModelName);
	}

	BroadcastConfigurationUpdate();
}

void UInputStreamlinerWidget::SetProjectPrefix(const FString& Prefix)
{
	CurrentConfiguration.ProjectPrefix = Prefix;
	BroadcastConfigurationUpdate();
}

void UInputStreamlinerWidget::SetCodeGenerationType(ECodeGenerationType Type)
{
	CurrentConfiguration.CodeGenType = Type;
	BroadcastConfigurationUpdate();
}

void UInputStreamlinerWidget::SetOutputPaths(const FString& ActionsPath, const FString& ContextsPath, const FString& WidgetsPath, const FString& CodePath)
{
	CurrentConfiguration.InputActionsPath = ActionsPath;
	CurrentConfiguration.MappingContextsPath = ContextsPath;
	CurrentConfiguration.WidgetsPath = WidgetsPath;
	CurrentConfiguration.GeneratedCodePath = CodePath;
	BroadcastConfigurationUpdate();
}

// ==================== Action Management ====================

bool UInputStreamlinerWidget::GetActionAtIndex(int32 Index, FInputActionDefinition& OutAction) const
{
	if (CurrentConfiguration.Actions.IsValidIndex(Index))
	{
		OutAction = CurrentConfiguration.Actions[Index];
		return true;
	}
	return false;
}

bool UInputStreamlinerWidget::GetActionByName(FName ActionName, FInputActionDefinition& OutAction) const
{
	const FInputActionDefinition* Found = CurrentConfiguration.FindAction(ActionName);
	if (Found)
	{
		OutAction = *Found;
		return true;
	}
	return false;
}

void UInputStreamlinerWidget::AddAction(const FInputActionDefinition& Action)
{
	CurrentConfiguration.Actions.Add(Action);
	BroadcastConfigurationUpdate();
	UE_LOG(LogInputStreamliner, Log, TEXT("Added action: %s"), *Action.ActionName.ToString());
}

bool UInputStreamlinerWidget::UpdateAction(FName ActionName, const FInputActionDefinition& UpdatedAction)
{
	FInputActionDefinition* Found = CurrentConfiguration.FindAction(ActionName);
	if (Found)
	{
		*Found = UpdatedAction;
		BroadcastConfigurationUpdate();
		return true;
	}
	return false;
}

bool UInputStreamlinerWidget::RemoveAction(FName ActionName)
{
	int32 RemovedCount = CurrentConfiguration.Actions.RemoveAll([ActionName](const FInputActionDefinition& Action)
	{
		return Action.ActionName == ActionName;
	});

	if (RemovedCount > 0)
	{
		BroadcastConfigurationUpdate();
		UE_LOG(LogInputStreamliner, Log, TEXT("Removed action: %s"), *ActionName.ToString());
		return true;
	}
	return false;
}

bool UInputStreamlinerWidget::RemoveActionAtIndex(int32 Index)
{
	if (CurrentConfiguration.Actions.IsValidIndex(Index))
	{
		FName RemovedName = CurrentConfiguration.Actions[Index].ActionName;
		CurrentConfiguration.Actions.RemoveAt(Index);

		// Adjust selection if needed
		if (SelectedActionIndex >= CurrentConfiguration.Actions.Num())
		{
			SelectedActionIndex = CurrentConfiguration.Actions.Num() - 1;
		}

		BroadcastConfigurationUpdate();
		UE_LOG(LogInputStreamliner, Log, TEXT("Removed action at index %d: %s"), Index, *RemovedName.ToString());
		return true;
	}
	return false;
}

void UInputStreamlinerWidget::ClearAllActions()
{
	CurrentConfiguration.Actions.Empty();
	SelectedActionIndex = -1;
	BroadcastConfigurationUpdate();
	UE_LOG(LogInputStreamliner, Log, TEXT("Cleared all actions"));
}

bool UInputStreamlinerWidget::DuplicateAction(FName ActionName, FName NewName)
{
	const FInputActionDefinition* Source = CurrentConfiguration.FindAction(ActionName);
	if (!Source)
	{
		return false;
	}

	if (CurrentConfiguration.HasAction(NewName))
	{
		return false; // Name already exists
	}

	FInputActionDefinition Duplicate = *Source;
	Duplicate.ActionName = NewName;
	CurrentConfiguration.Actions.Add(Duplicate);
	BroadcastConfigurationUpdate();
	return true;
}

TArray<FInputActionDefinition> UInputStreamlinerWidget::GetActionsInCategory(const FString& Category) const
{
	return CurrentConfiguration.GetActionsInCategory(Category);
}

// ==================== Touch Controls ====================

void UInputStreamlinerWidget::AddTouchControl(const FTouchControlDefinition& Control)
{
	CurrentConfiguration.TouchControls.Add(Control);
	BroadcastConfigurationUpdate();
}

bool UInputStreamlinerWidget::RemoveTouchControl(FName ControlName)
{
	int32 RemovedCount = CurrentConfiguration.TouchControls.RemoveAll([ControlName](const FTouchControlDefinition& Control)
	{
		return Control.ControlName == ControlName;
	});

	if (RemovedCount > 0)
	{
		BroadcastConfigurationUpdate();
		return true;
	}
	return false;
}

bool UInputStreamlinerWidget::UpdateTouchControl(FName ControlName, const FTouchControlDefinition& UpdatedControl)
{
	for (FTouchControlDefinition& Control : CurrentConfiguration.TouchControls)
	{
		if (Control.ControlName == ControlName)
		{
			Control = UpdatedControl;
			BroadcastConfigurationUpdate();
			return true;
		}
	}
	return false;
}

void UInputStreamlinerWidget::SetGyroConfiguration(const FGyroConfiguration& Config)
{
	CurrentConfiguration.GyroConfig = Config;
	BroadcastConfigurationUpdate();
}

// ==================== Generation ====================

void UInputStreamlinerWidget::GenerateAssets()
{
	if (!AssetGenerator)
	{
		SetStatusText(TEXT("Asset Generator not initialized"), FLinearColor::Red);
		OnGenerationComplete.Broadcast(false, TEXT("Asset Generator not initialized"));
		return;
	}

	TArray<FString> Errors;
	if (!ValidateConfiguration(Errors))
	{
		FString ErrorMsg = FString::Printf(TEXT("Validation failed: %s"), *FString::Join(Errors, TEXT(", ")));
		SetStatusText(ErrorMsg, FLinearColor::Red);
		OnGenerationComplete.Broadcast(false, ErrorMsg);
		return;
	}

	UE_LOG(LogInputStreamliner, Log, TEXT("Generating assets for %d actions"), CurrentConfiguration.Actions.Num());

	TArray<UObject*> CreatedAssets;
	bool bSuccess = AssetGenerator->GenerateInputAssets(CurrentConfiguration, CreatedAssets);

	if (bSuccess)
	{
		FString SuccessMsg = FString::Printf(TEXT("Generated %d assets successfully"), CreatedAssets.Num());
		SetStatusText(SuccessMsg, FLinearColor::Green);
		OnGenerationComplete.Broadcast(true, SuccessMsg);
	}
	else
	{
		SetStatusText(TEXT("Asset generation failed"), FLinearColor::Red);
		OnGenerationComplete.Broadcast(false, TEXT("Asset generation failed"));
	}
}

void UInputStreamlinerWidget::GenerateInputActions()
{
	if (!AssetGenerator)
	{
		OnGenerationComplete.Broadcast(false, TEXT("Asset Generator not initialized"));
		return;
	}

	int32 SuccessCount = 0;
	for (const FInputActionDefinition& ActionDef : CurrentConfiguration.Actions)
	{
		UInputAction* Action = AssetGenerator->GenerateInputAction(ActionDef, CurrentConfiguration.InputActionsPath);
		if (Action)
		{
			SuccessCount++;
		}
	}

	bool bSuccess = SuccessCount == CurrentConfiguration.Actions.Num();
	OnGenerationComplete.Broadcast(bSuccess, FString::Printf(TEXT("Generated %d/%d Input Actions"), SuccessCount, CurrentConfiguration.Actions.Num()));
}

void UInputStreamlinerWidget::GenerateMappingContexts()
{
	if (!AssetGenerator)
	{
		OnGenerationComplete.Broadcast(false, TEXT("Asset Generator not initialized"));
		return;
	}

	int32 SuccessCount = 0;

	// Generate mapping context for each platform that has bindings
	TArray<ETargetPlatform> Platforms = { ETargetPlatform::PC_Keyboard, ETargetPlatform::PC_Gamepad };

	for (ETargetPlatform Platform : Platforms)
	{
		UInputMappingContext* Context = AssetGenerator->GenerateMappingContext(
			Platform,
			CurrentConfiguration.Actions,
			CurrentConfiguration.MappingContextsPath,
			CurrentConfiguration
		);
		if (Context)
		{
			SuccessCount++;
		}
	}

	bool bSuccess = SuccessCount > 0;
	OnGenerationComplete.Broadcast(bSuccess, FString::Printf(TEXT("Generated %d Mapping Contexts"), SuccessCount));
}

void UInputStreamlinerWidget::GenerateTouchControls()
{
	// TODO: Implement touch control widget generation
	OnGenerationComplete.Broadcast(false, TEXT("Touch control generation not yet implemented"));
}

TArray<FString> UInputStreamlinerWidget::PreviewGeneration() const
{
	TArray<FString> Paths;

	// Input Actions - matches GenerateInputAction naming: IA_ActionName
	for (const FInputActionDefinition& Action : CurrentConfiguration.Actions)
	{
		Paths.Add(FString::Printf(TEXT("%s/IA_%s"),
			*CurrentConfiguration.InputActionsPath,
			*Action.ActionName.ToString()));
	}

	// Mapping Contexts - matches GenerateMappingContext naming: IMC_PlatformName
	if (CurrentConfiguration.bGenerateMappingContexts)
	{
		Paths.Add(FString::Printf(TEXT("%s/IMC_PC_Keyboard"), *CurrentConfiguration.MappingContextsPath));
		Paths.Add(FString::Printf(TEXT("%s/IMC_PC_Gamepad"), *CurrentConfiguration.MappingContextsPath));
		Paths.Add(FString::Printf(TEXT("%s/IMC_iOS"), *CurrentConfiguration.MappingContextsPath));
		Paths.Add(FString::Printf(TEXT("%s/IMC_Android"), *CurrentConfiguration.MappingContextsPath));
		Paths.Add(FString::Printf(TEXT("%s/IMC_Mac"), *CurrentConfiguration.MappingContextsPath));
	}

	return Paths;
}

// ==================== Persistence ====================

bool UInputStreamlinerWidget::SaveConfiguration(const FString& FilePath)
{
	FString JsonString;
	if (FJsonObjectConverter::UStructToJsonObjectString(CurrentConfiguration, JsonString))
	{
		if (FFileHelper::SaveStringToFile(JsonString, *FilePath))
		{
			UE_LOG(LogInputStreamliner, Log, TEXT("Configuration saved to: %s"), *FilePath);
			return true;
		}
	}

	UE_LOG(LogInputStreamliner, Warning, TEXT("Failed to save configuration to: %s"), *FilePath);
	return false;
}

bool UInputStreamlinerWidget::LoadConfiguration(const FString& FilePath)
{
	FString JsonString;
	if (FFileHelper::LoadFileToString(JsonString, *FilePath))
	{
		FInputStreamlinerConfiguration LoadedConfig;
		if (FJsonObjectConverter::JsonObjectStringToUStruct(JsonString, &LoadedConfig))
		{
			SetConfiguration(LoadedConfig);
			UE_LOG(LogInputStreamliner, Log, TEXT("Configuration loaded from: %s"), *FilePath);
			return true;
		}
	}

	UE_LOG(LogInputStreamliner, Warning, TEXT("Failed to load configuration from: %s"), *FilePath);
	return false;
}

FString UInputStreamlinerWidget::GetDefaultConfigPath() const
{
	return FPaths::ProjectSavedDir() / TEXT("InputStreamliner") / TEXT("Configuration.json");
}

void UInputStreamlinerWidget::ExportToClipboard()
{
	FString JsonString;
	if (FJsonObjectConverter::UStructToJsonObjectString(CurrentConfiguration, JsonString))
	{
		FPlatformApplicationMisc::ClipboardCopy(*JsonString);
		UE_LOG(LogInputStreamliner, Log, TEXT("Configuration exported to clipboard"));
	}
}

bool UInputStreamlinerWidget::ImportFromClipboard()
{
	FString ClipboardContent;
	FPlatformApplicationMisc::ClipboardPaste(ClipboardContent);

	if (ClipboardContent.IsEmpty())
	{
		return false;
	}

	FInputStreamlinerConfiguration LoadedConfig;
	if (FJsonObjectConverter::JsonObjectStringToUStruct(ClipboardContent, &LoadedConfig))
	{
		SetConfiguration(LoadedConfig);
		UE_LOG(LogInputStreamliner, Log, TEXT("Configuration imported from clipboard"));
		return true;
	}

	return false;
}

// ==================== Validation ====================

bool UInputStreamlinerWidget::ValidateConfiguration(TArray<FString>& OutErrors) const
{
	OutErrors.Empty();

	if (CurrentConfiguration.ProjectPrefix.IsEmpty())
	{
		OutErrors.Add(TEXT("Project prefix is empty"));
	}

	if (CurrentConfiguration.Actions.Num() == 0)
	{
		OutErrors.Add(TEXT("No actions defined"));
	}

	// Check for duplicate action names
	TSet<FName> SeenNames;
	for (const FInputActionDefinition& Action : CurrentConfiguration.Actions)
	{
		if (Action.ActionName.IsNone())
		{
			OutErrors.Add(TEXT("Action has no name"));
		}
		else if (SeenNames.Contains(Action.ActionName))
		{
			OutErrors.Add(FString::Printf(TEXT("Duplicate action name: %s"), *Action.ActionName.ToString()));
		}
		else
		{
			SeenNames.Add(Action.ActionName);
		}
	}

	// Validate paths
	if (CurrentConfiguration.InputActionsPath.IsEmpty())
	{
		OutErrors.Add(TEXT("Input Actions path is empty"));
	}

	return OutErrors.Num() == 0;
}

bool UInputStreamlinerWidget::IsValidActionName(FName ActionName, FName ExcludeName) const
{
	if (ActionName.IsNone())
	{
		return false;
	}

	for (const FInputActionDefinition& Action : CurrentConfiguration.Actions)
	{
		if (Action.ActionName == ActionName && Action.ActionName != ExcludeName)
		{
			return false; // Name already exists
		}
	}

	return true;
}

// ==================== UI State ====================

void UInputStreamlinerWidget::SelectAction(int32 Index)
{
	if (CurrentConfiguration.Actions.IsValidIndex(Index))
	{
		SelectedActionIndex = Index;
		OnActionSelected.Broadcast(CurrentConfiguration.Actions[Index]);
	}
	else
	{
		SelectedActionIndex = -1;
	}
}

bool UInputStreamlinerWidget::GetSelectedAction(FInputActionDefinition& OutAction) const
{
	return GetActionAtIndex(SelectedActionIndex, OutAction);
}

void UInputStreamlinerWidget::BroadcastConfigurationUpdate()
{
	OnConfigurationUpdated.Broadcast(CurrentConfiguration);
	RefreshActionsList();
}

// ==================== UI Building ====================

void UInputStreamlinerWidget::BuildUI()
{
	UE_LOG(LogInputStreamliner, Warning, TEXT("BuildUI called! ContentContainer=%s"), ContentContainer ? TEXT("valid") : TEXT("null"));

	if (!ContentContainer)
	{
		UE_LOG(LogInputStreamliner, Error, TEXT("ContentContainer is null! Add a VerticalBox named 'ContentContainer' in the Blueprint Designer."));
		return;
	}

	RootBox = ContentContainer;

	// Compact font size for all text
	const int32 FontSize = 8;
	const float Pad = 2.f;
	const float SectionPad = 4.f;

	// ===== Title =====
	UTextBlock* TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
	TitleText->SetText(FText::FromString(TEXT("Input Streamliner")));
	FSlateFontInfo TitleFont = TitleText->GetFont();
	TitleFont.Size = FontSize + 1;
	TitleText->SetFont(TitleFont);
	UVerticalBoxSlot* TitleSlot = RootBox->AddChildToVerticalBox(TitleText);
	TitleSlot->SetPadding(FMargin(Pad, Pad, Pad, Pad));

	// ===== Project Prefix Row =====
	UHorizontalBox* PrefixRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("PrefixRow"));
	UVerticalBoxSlot* PrefixRowSlot = RootBox->AddChildToVerticalBox(PrefixRow);
	PrefixRowSlot->SetPadding(FMargin(Pad, Pad));

	UTextBlock* PrefixLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PrefixLabel"));
	PrefixLabel->SetText(FText::FromString(TEXT("Prefix:")));
	FSlateFontInfo PrefixFont = PrefixLabel->GetFont();
	PrefixFont.Size = FontSize;
	PrefixLabel->SetFont(PrefixFont);
	UHorizontalBoxSlot* PrefixLabelSlot = PrefixRow->AddChildToHorizontalBox(PrefixLabel);
	PrefixLabelSlot->SetPadding(FMargin(0.f, 0.f, Pad, 0.f));
	PrefixLabelSlot->SetVerticalAlignment(VAlign_Center);

	ProjectPrefixInput = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), TEXT("ProjectPrefixInput"));
	ProjectPrefixInput->SetText(FText::FromString(CurrentConfiguration.ProjectPrefix));
	UHorizontalBoxSlot* PrefixInputSlot = PrefixRow->AddChildToHorizontalBox(ProjectPrefixInput);
	PrefixInputSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	// ===== Description Section =====
	UTextBlock* DescLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DescLabel"));
	DescLabel->SetText(FText::FromString(TEXT("Description:")));
	FSlateFontInfo DescFont = DescLabel->GetFont();
	DescFont.Size = FontSize;
	DescLabel->SetFont(DescFont);
	UVerticalBoxSlot* DescLabelSlot = RootBox->AddChildToVerticalBox(DescLabel);
	DescLabelSlot->SetPadding(FMargin(Pad, SectionPad, Pad, Pad));

	DescriptionInput = WidgetTree->ConstructWidget<UMultiLineEditableTextBox>(UMultiLineEditableTextBox::StaticClass(), TEXT("DescriptionInput"));
	DescriptionInput->SetHintText(FText::FromString(TEXT("Describe input needs...")));
	UVerticalBoxSlot* DescInputSlot = RootBox->AddChildToVerticalBox(DescriptionInput);
	DescInputSlot->SetPadding(FMargin(Pad, 0.f));
	DescInputSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	// ===== Model Selection Row =====
	UHorizontalBox* ModelRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ModelRow"));
	UVerticalBoxSlot* ModelRowSlot = RootBox->AddChildToVerticalBox(ModelRow);
	ModelRowSlot->SetPadding(FMargin(Pad, Pad));

	UTextBlock* ModelLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ModelLabel"));
	ModelLabel->SetText(FText::FromString(TEXT("Model:")));
	FSlateFontInfo ModelLabelFont = ModelLabel->GetFont();
	ModelLabelFont.Size = FontSize;
	ModelLabel->SetFont(ModelLabelFont);
	UHorizontalBoxSlot* ModelLabelSlot = ModelRow->AddChildToHorizontalBox(ModelLabel);
	ModelLabelSlot->SetPadding(FMargin(0.f, 0.f, Pad, 0.f));
	ModelLabelSlot->SetVerticalAlignment(VAlign_Center);

	ModelDropdown = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("ModelDropdown"));
	ModelDropdown->AddOption(CurrentConfiguration.LLMModelName);
	ModelDropdown->SetSelectedOption(CurrentConfiguration.LLMModelName);
	ModelDropdown->OnSelectionChanged.AddDynamic(this, &UInputStreamlinerWidget::OnModelSelectionChanged);
	// Set small font for dropdown
	ModelDropdown->Font.Size = FontSize;
	UHorizontalBoxSlot* ModelDropSlot = ModelRow->AddChildToHorizontalBox(ModelDropdown);
	ModelDropSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	// ===== AI Buttons Row =====
	UHorizontalBox* AIButtonsRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("AIButtonsRow"));
	UVerticalBoxSlot* AIButtonsSlot = RootBox->AddChildToVerticalBox(AIButtonsRow);
	AIButtonsSlot->SetPadding(FMargin(Pad, SectionPad));

	UButton* ParseButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ParseButton"));
	ParseButton->OnClicked.AddDynamic(this, &UInputStreamlinerWidget::OnParseButtonClicked);
	UHorizontalBoxSlot* ParseBtnSlot = AIButtonsRow->AddChildToHorizontalBox(ParseButton);
	ParseBtnSlot->SetPadding(FMargin(0.f, 0.f, Pad, 0.f));

	UTextBlock* ParseBtnText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ParseBtnText"));
	ParseBtnText->SetText(FText::FromString(TEXT("Parse")));
	FSlateFontInfo ParseFont = ParseBtnText->GetFont();
	ParseFont.Size = FontSize;
	ParseBtnText->SetFont(ParseFont);
	ParseButton->AddChild(ParseBtnText);

	UButton* TestConnBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("TestConnBtn"));
	TestConnBtn->OnClicked.AddDynamic(this, &UInputStreamlinerWidget::OnTestConnectionButtonClicked);
	UHorizontalBoxSlot* TestConnSlot = AIButtonsRow->AddChildToHorizontalBox(TestConnBtn);
	TestConnSlot->SetPadding(FMargin(Pad, 0.f));

	UTextBlock* TestConnText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TestConnText"));
	TestConnText->SetText(FText::FromString(TEXT("Test LLM")));
	FSlateFontInfo TestFont = TestConnText->GetFont();
	TestFont.Size = FontSize;
	TestConnText->SetFont(TestFont);
	TestConnBtn->AddChild(TestConnText);

	// ===== Actions Section Header =====
	UHorizontalBox* ActionsHeader = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ActionsHeader"));
	UVerticalBoxSlot* ActionsHeaderSlot = RootBox->AddChildToVerticalBox(ActionsHeader);
	ActionsHeaderSlot->SetPadding(FMargin(Pad, SectionPad, Pad, Pad));

	UTextBlock* ActionsLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ActionsLabel"));
	ActionsLabel->SetText(FText::FromString(TEXT("Actions:")));
	FSlateFontInfo ActionsFont = ActionsLabel->GetFont();
	ActionsFont.Size = FontSize;
	ActionsLabel->SetFont(ActionsFont);
	UHorizontalBoxSlot* ActionsLabelSlot = ActionsHeader->AddChildToHorizontalBox(ActionsLabel);
	ActionsLabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	ActionsLabelSlot->SetVerticalAlignment(VAlign_Center);

	UButton* AddActionBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("AddActionBtn"));
	AddActionBtn->OnClicked.AddDynamic(this, &UInputStreamlinerWidget::OnAddActionButtonClicked);
	UHorizontalBoxSlot* AddBtnSlot = ActionsHeader->AddChildToHorizontalBox(AddActionBtn);
	AddBtnSlot->SetPadding(FMargin(Pad, 0.f));

	UTextBlock* AddActionText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("AddActionText"));
	AddActionText->SetText(FText::FromString(TEXT("+")));
	FSlateFontInfo AddFont = AddActionText->GetFont();
	AddFont.Size = FontSize;
	AddActionText->SetFont(AddFont);
	AddActionBtn->AddChild(AddActionText);

	UButton* ClearAllBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ClearAllBtn"));
	ClearAllBtn->OnClicked.AddDynamic(this, &UInputStreamlinerWidget::OnClearAllButtonClicked);
	UHorizontalBoxSlot* ClearBtnSlot = ActionsHeader->AddChildToHorizontalBox(ClearAllBtn);
	ClearBtnSlot->SetPadding(FMargin(Pad, 0.f, 0.f, 0.f));

	UTextBlock* ClearAllText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ClearAllText"));
	ClearAllText->SetText(FText::FromString(TEXT("Clear")));
	FSlateFontInfo ClearFont = ClearAllText->GetFont();
	ClearFont.Size = FontSize;
	ClearAllText->SetFont(ClearFont);
	ClearAllBtn->AddChild(ClearAllText);

	// ===== Actions List (ScrollBox) =====
	ActionsScrollBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("ActionsScrollBox"));
	UVerticalBoxSlot* ScrollSlot = RootBox->AddChildToVerticalBox(ActionsScrollBox);
	ScrollSlot->SetPadding(FMargin(Pad, Pad));
	ScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	ActionsListBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ActionsListBox"));
	ActionsScrollBox->AddChild(ActionsListBox);

	// ===== Generate/Delete Buttons Row =====
	UHorizontalBox* GenRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("GenRow"));
	UVerticalBoxSlot* GenRowSlot = RootBox->AddChildToVerticalBox(GenRow);
	GenRowSlot->SetPadding(FMargin(Pad, SectionPad));
	GenRowSlot->SetHorizontalAlignment(HAlign_Fill);

	UButton* GenerateBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("GenerateBtn"));
	GenerateBtn->OnClicked.AddDynamic(this, &UInputStreamlinerWidget::OnGenerateButtonClicked);
	GenerateBtn->SetBackgroundColor(FLinearColor(0.2f, 0.5f, 0.2f, 1.0f));
	UHorizontalBoxSlot* GenBtnSlot = GenRow->AddChildToHorizontalBox(GenerateBtn);
	GenBtnSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	GenBtnSlot->SetPadding(FMargin(0.f, 0.f, Pad, 0.f));

	UTextBlock* GenerateBtnText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("GenerateBtnText"));
	GenerateBtnText->SetText(FText::FromString(TEXT("Generate")));
	FSlateFontInfo GenFont = GenerateBtnText->GetFont();
	GenFont.Size = FontSize;
	GenerateBtnText->SetFont(GenFont);
	GenerateBtn->AddChild(GenerateBtnText);

	UButton* DeleteBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("DeleteBtn"));
	DeleteBtn->OnClicked.AddDynamic(this, &UInputStreamlinerWidget::OnDeleteGeneratedAssetsClicked);
	DeleteBtn->SetBackgroundColor(FLinearColor(0.6f, 0.2f, 0.2f, 1.0f));
	UHorizontalBoxSlot* DelBtnSlot = GenRow->AddChildToHorizontalBox(DeleteBtn);
	DelBtnSlot->SetPadding(FMargin(Pad, 0.f, 0.f, 0.f));

	UTextBlock* DeleteBtnText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DeleteBtnText"));
	DeleteBtnText->SetText(FText::FromString(TEXT("Delete Assets")));
	FSlateFontInfo DelFont = DeleteBtnText->GetFont();
	DelFont.Size = FontSize;
	DeleteBtnText->SetFont(DelFont);
	DeleteBtn->AddChild(DeleteBtnText);

	// ===== Status Text =====
	StatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StatusText"));
	StatusText->SetText(FText::FromString(TEXT("Ready")));
	FSlateFontInfo StatusFont = StatusText->GetFont();
	StatusFont.Size = FontSize;
	StatusText->SetFont(StatusFont);
	UVerticalBoxSlot* StatusSlot = RootBox->AddChildToVerticalBox(StatusText);
	StatusSlot->SetPadding(FMargin(Pad, Pad));

	UE_LOG(LogInputStreamliner, Log, TEXT("UI built successfully"));
}

void UInputStreamlinerWidget::RefreshActionsList()
{
	if (!ActionsListBox || !WidgetTree)
	{
		return;
	}

	const int32 FontSize = 8;
	ActionsListBox->ClearChildren();

	for (int32 i = 0; i < CurrentConfiguration.Actions.Num(); i++)
	{
		const FInputActionDefinition& Action = CurrentConfiguration.Actions[i];

		UHorizontalBox* ActionRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		UVerticalBoxSlot* RowSlot = ActionsListBox->AddChildToVerticalBox(ActionRow);
		RowSlot->SetPadding(FMargin(0.f, 1.f));

		// Action name
		UTextBlock* NameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		NameText->SetText(FText::FromName(Action.ActionName));
		FSlateFontInfo NameFont = NameText->GetFont();
		NameFont.Size = FontSize;
		NameText->SetFont(NameFont);
		UHorizontalBoxSlot* NameSlot = ActionRow->AddChildToHorizontalBox(NameText);
		NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		NameSlot->SetVerticalAlignment(VAlign_Center);

		// Action type
		UTextBlock* TypeText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		FString TypeString;
		switch (Action.ActionType)
		{
		case EInputActionType::Bool: TypeString = TEXT("[B]"); break;
		case EInputActionType::Axis1D: TypeString = TEXT("[1D]"); break;
		case EInputActionType::Axis2D: TypeString = TEXT("[2D]"); break;
		case EInputActionType::Axis3D: TypeString = TEXT("[3D]"); break;
		}
		TypeText->SetText(FText::FromString(TypeString));
		FSlateFontInfo TypeFont = TypeText->GetFont();
		TypeFont.Size = FontSize;
		TypeText->SetFont(TypeFont);
		TypeText->SetColorAndOpacity(FSlateColor(FLinearColor(0.7f, 0.7f, 0.7f)));
		UHorizontalBoxSlot* TypeSlot = ActionRow->AddChildToHorizontalBox(TypeText);
		TypeSlot->SetPadding(FMargin(4.f, 0.f));
		TypeSlot->SetVerticalAlignment(VAlign_Center);
	}

	if (CurrentConfiguration.Actions.Num() == 0)
	{
		UTextBlock* EmptyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		EmptyText->SetText(FText::FromString(TEXT("No actions. Parse or add manually.")));
		FSlateFontInfo EmptyFont = EmptyText->GetFont();
		EmptyFont.Size = FontSize;
		EmptyText->SetFont(EmptyFont);
		EmptyText->SetColorAndOpacity(FSlateColor(FLinearColor(0.5f, 0.5f, 0.5f)));
		ActionsListBox->AddChildToVerticalBox(EmptyText);
	}
}

void UInputStreamlinerWidget::SetStatusText(const FString& Text, FLinearColor Color)
{
	if (StatusText)
	{
		StatusText->SetText(FText::FromString(Text));
		StatusText->SetColorAndOpacity(FSlateColor(Color));
	}
}

// ==================== UI Button Handlers ====================

void UInputStreamlinerWidget::OnParseButtonClicked()
{
	if (!DescriptionInput)
	{
		return;
	}

	FString Description = DescriptionInput->GetText().ToString();
	if (Description.IsEmpty())
	{
		SetStatusText(TEXT("Please enter a description first"), FLinearColor::Yellow);
		return;
	}

	// Update project prefix from input
	if (ProjectPrefixInput)
	{
		CurrentConfiguration.ProjectPrefix = ProjectPrefixInput->GetText().ToString();
	}

	SetStatusText(TEXT("Parsing with AI..."), FLinearColor::White);
	ParseDescription(Description);
}

void UInputStreamlinerWidget::OnAddActionButtonClicked()
{
	// Create a default action
	FInputActionDefinition NewAction;
	NewAction.ActionName = FName(*FString::Printf(TEXT("NewAction_%d"), CurrentConfiguration.Actions.Num()));
	NewAction.ActionType = EInputActionType::Bool;
	NewAction.Category = TEXT("General");
	NewAction.DisplayName = TEXT("New Action");

	AddAction(NewAction);
	SetStatusText(FString::Printf(TEXT("Added action: %s"), *NewAction.ActionName.ToString()), FLinearColor::Green);
}

void UInputStreamlinerWidget::OnGenerateButtonClicked()
{
	// Update project prefix from input
	if (ProjectPrefixInput)
	{
		CurrentConfiguration.ProjectPrefix = ProjectPrefixInput->GetText().ToString();
	}

	SetStatusText(TEXT("Generating assets..."), FLinearColor::White);
	GenerateAssets();
}

void UInputStreamlinerWidget::OnTestConnectionButtonClicked()
{
	if (!LLMParser)
	{
		SetStatusText(TEXT("LLM Parser not initialized"), FLinearColor::Red);
		return;
	}

	SetStatusText(TEXT("Testing LLM connection..."), FLinearColor::White);

	FOnParseComplete Callback;
	Callback.BindDynamic(this, &UInputStreamlinerWidget::HandleConnectionTestComplete);
	LLMParser->CheckConnection(Callback);
}

void UInputStreamlinerWidget::HandleConnectionTestComplete(bool bSuccess, const FString& ErrorMessage)
{
	if (bSuccess)
	{
		SetStatusText(TEXT("LLM connection successful!"), FLinearColor::Green);
	}
	else
	{
		SetStatusText(FString::Printf(TEXT("Connection failed: %s"), *ErrorMessage), FLinearColor::Red);
	}
}

void UInputStreamlinerWidget::OnClearAllButtonClicked()
{
	ClearAllActions();
	SetStatusText(TEXT("Cleared"), FLinearColor::Yellow);
}

void UInputStreamlinerWidget::OnDeleteGeneratedAssetsClicked()
{
	TArray<FString> AssetPaths = PreviewGeneration();

	if (AssetPaths.Num() == 0)
	{
		SetStatusText(TEXT("No assets defined"), FLinearColor::Yellow);
		return;
	}

	int32 DeletedCount = 0;
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	TArray<UObject*> AssetsToDelete;

	for (const FString& AssetPath : AssetPaths)
	{
		// Try to find asset by package path
		FString PackagePath = AssetPath;

		// Get asset name from path
		FString AssetName = FPackageName::GetLongPackageAssetName(PackagePath);

		// Search for the asset
		FARFilter Filter;
		Filter.PackagePaths.Add(FName(*FPackageName::GetLongPackagePath(PackagePath)));
		Filter.bRecursivePaths = false;

		TArray<FAssetData> FoundAssets;
		AssetRegistry.GetAssets(Filter, FoundAssets);

		for (const FAssetData& AssetData : FoundAssets)
		{
			if (AssetData.AssetName.ToString() == AssetName)
			{
				UObject* Asset = AssetData.GetAsset();
				if (Asset)
				{
					AssetsToDelete.Add(Asset);
					UE_LOG(LogInputStreamliner, Log, TEXT("Found asset to delete: %s"), *AssetData.GetObjectPathString());
				}
			}
		}
	}

	if (AssetsToDelete.Num() > 0)
	{
		// Delete all found assets at once
		DeletedCount = ObjectTools::DeleteObjects(AssetsToDelete, true);
	}

	if (DeletedCount > 0)
	{
		SetStatusText(FString::Printf(TEXT("Deleted %d assets"), DeletedCount), FLinearColor::Green);
	}
	else
	{
		SetStatusText(TEXT("No generated assets found"), FLinearColor::Yellow);
	}
}

void UInputStreamlinerWidget::OnModelSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	if (SelectionType == ESelectInfo::Direct)
	{
		return; // Ignore programmatic changes
	}

	CurrentConfiguration.LLMModelName = SelectedItem;

	if (LLMParser)
	{
		LLMParser->SetModel(SelectedItem);
	}

	SetStatusText(FString::Printf(TEXT("Model: %s"), *SelectedItem), FLinearColor::White);
	UE_LOG(LogInputStreamliner, Log, TEXT("Switched to model: %s"), *SelectedItem);
}

void UInputStreamlinerWidget::RefreshModelList()
{
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();

	Request->SetURL(FString::Printf(TEXT("%s:%d/api/tags"),
		*CurrentConfiguration.LLMEndpointURL,
		CurrentConfiguration.LLMEndpointPort));
	Request->SetVerb(TEXT("GET"));
	Request->SetTimeout(5.0f);

	Request->OnProcessRequestComplete().BindUObject(this, &UInputStreamlinerWidget::OnModelListReceived);
	Request->ProcessRequest();
}

void UInputStreamlinerWidget::OnModelListReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	if (!ModelDropdown)
	{
		return;
	}

	if (!bWasSuccessful || !Response.IsValid() || Response->GetResponseCode() != 200)
	{
		UE_LOG(LogInputStreamliner, Warning, TEXT("Failed to fetch model list from Ollama"));
		return;
	}

	// Parse response JSON
	TSharedPtr<FJsonObject> JsonResponse;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());

	if (!FJsonSerializer::Deserialize(Reader, JsonResponse))
	{
		return;
	}

	// Get models array
	const TArray<TSharedPtr<FJsonValue>>* ModelsArray;
	if (!JsonResponse->TryGetArrayField(TEXT("models"), ModelsArray))
	{
		return;
	}

	// Store current selection
	FString CurrentSelection = ModelDropdown->GetSelectedOption();

	// Clear and repopulate
	ModelDropdown->ClearOptions();

	for (const TSharedPtr<FJsonValue>& ModelValue : *ModelsArray)
	{
		TSharedPtr<FJsonObject> ModelObj = ModelValue->AsObject();
		if (ModelObj.IsValid())
		{
			FString ModelName = ModelObj->GetStringField(TEXT("name"));
			if (!ModelName.IsEmpty())
			{
				ModelDropdown->AddOption(ModelName);
			}
		}
	}

	// Restore selection or use config default
	if (ModelDropdown->FindOptionIndex(CurrentSelection) != INDEX_NONE)
	{
		ModelDropdown->SetSelectedOption(CurrentSelection);
	}
	else if (ModelDropdown->FindOptionIndex(CurrentConfiguration.LLMModelName) != INDEX_NONE)
	{
		ModelDropdown->SetSelectedOption(CurrentConfiguration.LLMModelName);
	}
	else if (ModelDropdown->GetOptionCount() > 0)
	{
		ModelDropdown->SetSelectedIndex(0);
	}

	UE_LOG(LogInputStreamliner, Log, TEXT("Loaded %d models from Ollama"), ModelDropdown->GetOptionCount());
}
