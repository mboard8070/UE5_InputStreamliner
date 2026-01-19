// Copyright Epic Games, Inc. All Rights Reserved.

#include "RebindingSettingsWidget.h"
#include "InputStreamlinerRuntimeModule.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Slider.h"
#include "Components/CheckBox.h"
#include "Components/ScrollBox.h"
#include "Components/Spacer.h"
#include "Components/Border.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBoxSlot.h"
#include "Blueprint/WidgetTree.h"
#include "InputAction.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/GameInstance.h"

// ============== URebindActionRow ==============

void URebindActionRow::NativeConstruct()
{
	Super::NativeConstruct();

	if (!bWidgetsCreated)
	{
		CreateWidgets();
	}

	if (RebindButton)
	{
		RebindButton->OnClicked.AddDynamic(this, &URebindActionRow::OnRebindClicked);
	}

	if (ResetButton)
	{
		ResetButton->OnClicked.AddDynamic(this, &URebindActionRow::OnResetClicked);
	}
}

void URebindActionRow::CreateWidgets()
{
	if (bWidgetsCreated || !WidgetTree)
	{
		return;
	}

	// Create horizontal box as root
	UHorizontalBox* RootBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("RootBox"));
	WidgetTree->RootWidget = RootBox;

	// Action name text
	ActionNameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ActionNameText"));
	ActionNameText->SetText(FText::FromString(TEXT("Action")));
	FSlateFontInfo Font = ActionNameText->GetFont();
	Font.Size = 12;
	ActionNameText->SetFont(Font);
	UHorizontalBoxSlot* NameSlot = RootBox->AddChildToHorizontalBox(ActionNameText);
	NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	NameSlot->SetVerticalAlignment(VAlign_Center);

	// Key binding text
	KeyBindingText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("KeyBindingText"));
	KeyBindingText->SetText(FText::FromString(TEXT("[None]")));
	KeyBindingText->SetFont(Font);
	KeyBindingText->SetColorAndOpacity(FSlateColor(FLinearColor(0.7f, 0.7f, 0.7f)));
	UHorizontalBoxSlot* KeySlot = RootBox->AddChildToHorizontalBox(KeyBindingText);
	KeySlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	KeySlot->SetVerticalAlignment(VAlign_Center);
	KeySlot->SetPadding(FMargin(10.f, 0.f));

	// Rebind button
	RebindButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("RebindButton"));
	RebindButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("RebindButtonText"));
	RebindButtonText->SetText(FText::FromString(TEXT("Rebind")));
	FSlateFontInfo ButtonFont = RebindButtonText->GetFont();
	ButtonFont.Size = 10;
	RebindButtonText->SetFont(ButtonFont);
	RebindButton->AddChild(RebindButtonText);
	UHorizontalBoxSlot* RebindSlot = RootBox->AddChildToHorizontalBox(RebindButton);
	RebindSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	RebindSlot->SetVerticalAlignment(VAlign_Center);
	RebindSlot->SetPadding(FMargin(5.f, 0.f));

	// Reset button
	ResetButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ResetButton"));
	UTextBlock* ResetText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ResetButtonText"));
	ResetText->SetText(FText::FromString(TEXT("Reset")));
	ResetText->SetFont(ButtonFont);
	ResetButton->AddChild(ResetText);
	UHorizontalBoxSlot* ResetSlot = RootBox->AddChildToHorizontalBox(ResetButton);
	ResetSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	ResetSlot->SetVerticalAlignment(VAlign_Center);

	bWidgetsCreated = true;
}

void URebindActionRow::SetupAction(UInputAction* InAction, UInputRebindingManager* InManager)
{
	Action = InAction;
	RebindingManager = InManager;

	if (ActionNameText && Action)
	{
		// Get display name from action
		FString ActionName = Action->GetName();
		// Remove IA_ prefix if present
		if (ActionName.StartsWith(TEXT("IA_")))
		{
			ActionName = ActionName.RightChop(3);
		}
		// Add spaces before capital letters
		FString DisplayName;
		for (int32 i = 0; i < ActionName.Len(); i++)
		{
			if (i > 0 && FChar::IsUpper(ActionName[i]) && !FChar::IsUpper(ActionName[i-1]))
			{
				DisplayName += TEXT(" ");
			}
			DisplayName += ActionName[i];
		}
		ActionNameText->SetText(FText::FromString(DisplayName));
	}

	// Bind to rebind complete event
	if (RebindingManager)
	{
		RebindingManager->OnRebindComplete.AddDynamic(this, &URebindActionRow::OnRebindComplete);
	}

	RefreshKeyDisplay();
}

void URebindActionRow::RefreshKeyDisplay()
{
	if (!KeyBindingText || !Action || !RebindingManager)
	{
		return;
	}

	TArray<FKey> Bindings = RebindingManager->GetBindingsForAction(Action);
	if (Bindings.Num() > 0 && Bindings[0].IsValid())
	{
		KeyBindingText->SetText(FText::FromString(Bindings[0].GetDisplayName().ToString()));
		KeyBindingText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	}
	else
	{
		KeyBindingText->SetText(FText::FromString(TEXT("[None]")));
		KeyBindingText->SetColorAndOpacity(FSlateColor(FLinearColor(0.5f, 0.5f, 0.5f)));
	}
}

void URebindActionRow::SetRebindingState(bool bIsRebinding)
{
	if (RebindButtonText)
	{
		if (bIsRebinding)
		{
			RebindButtonText->SetText(FText::FromString(TEXT("Press Key...")));
			RebindButtonText->SetColorAndOpacity(FSlateColor(FLinearColor::Yellow));
		}
		else
		{
			RebindButtonText->SetText(FText::FromString(TEXT("Rebind")));
			RebindButtonText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		}
	}

	if (RebindButton)
	{
		RebindButton->SetIsEnabled(!bIsRebinding);
	}

	if (ResetButton)
	{
		ResetButton->SetIsEnabled(!bIsRebinding);
	}
}

void URebindActionRow::OnRebindClicked()
{
	if (RebindingManager && Action)
	{
		SetRebindingState(true);
		RebindingManager->StartRebinding(Action);
	}
}

void URebindActionRow::OnResetClicked()
{
	if (RebindingManager && Action)
	{
		RebindingManager->ResetToDefault(Action);
		RefreshKeyDisplay();
	}
}

void URebindActionRow::OnRebindComplete(UInputAction* CompletedAction, FKey NewKey)
{
	if (CompletedAction == Action)
	{
		SetRebindingState(false);
		RefreshKeyDisplay();
	}
}

// ============== URebindingSettingsWidget ==============

void URebindingSettingsWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!bWidgetsCreated)
	{
		CreateWidgets();
	}

	// Get rebinding manager
	CachedManager = GetRebindingManager();
	if (CachedManager)
	{
		CachedManager->OnRebindComplete.AddDynamic(this, &URebindingSettingsWidget::OnRebindComplete);
		CachedManager->OnAnyKeyPressed.AddDynamic(this, &URebindingSettingsWidget::OnAnyKeyPressed);
		CachedManager->OnBindingConflict.AddDynamic(this, &URebindingSettingsWidget::OnBindingConflict);

		// Initialize sensitivity sliders
		if (MouseSensitivitySlider)
		{
			MouseSensitivitySlider->SetValue(CachedManager->GetMouseSensitivity() / 5.0f);
			MouseSensitivitySlider->OnValueChanged.AddDynamic(this, &URebindingSettingsWidget::OnMouseSensitivityChanged);
		}

		if (GamepadSensitivitySlider)
		{
			GamepadSensitivitySlider->SetValue(CachedManager->GetGamepadSensitivity() / 5.0f);
			GamepadSensitivitySlider->OnValueChanged.AddDynamic(this, &URebindingSettingsWidget::OnGamepadSensitivityChanged);
		}

		if (InvertYCheckBox)
		{
			InvertYCheckBox->SetIsChecked(CachedManager->GetInvertY());
			InvertYCheckBox->OnCheckStateChanged.AddDynamic(this, &URebindingSettingsWidget::OnInvertYChanged);
		}
	}

	// Bind buttons
	if (ResetAllButton)
	{
		ResetAllButton->OnClicked.AddDynamic(this, &URebindingSettingsWidget::OnResetAllClicked);
	}

	if (SaveButton)
	{
		SaveButton->OnClicked.AddDynamic(this, &URebindingSettingsWidget::OnSaveClicked);
	}

	if (CancelButton)
	{
		CancelButton->OnClicked.AddDynamic(this, &URebindingSettingsWidget::OnCancelClicked);
	}
}

void URebindingSettingsWidget::NativeDestruct()
{
	// Unbind delegates
	if (CachedManager)
	{
		CachedManager->OnRebindComplete.RemoveDynamic(this, &URebindingSettingsWidget::OnRebindComplete);
		CachedManager->OnAnyKeyPressed.RemoveDynamic(this, &URebindingSettingsWidget::OnAnyKeyPressed);
		CachedManager->OnBindingConflict.RemoveDynamic(this, &URebindingSettingsWidget::OnBindingConflict);
	}

	Super::NativeDestruct();
}

void URebindingSettingsWidget::CreateWidgets()
{
	if (bWidgetsCreated || !WidgetTree)
	{
		return;
	}

	const int32 FontSize = 10;

	// Create root vertical box
	UVerticalBox* RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RootBox"));
	WidgetTree->RootWidget = RootBox;

	// Title
	UTextBlock* TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
	TitleText->SetText(FText::FromString(TEXT("Input Settings")));
	FSlateFontInfo TitleFont = TitleText->GetFont();
	TitleFont.Size = 16;
	TitleText->SetFont(TitleFont);
	UVerticalBoxSlot* TitleSlot = RootBox->AddChildToVerticalBox(TitleText);
	TitleSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 10.f));

	// Key Bindings section header
	UTextBlock* BindingsHeader = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("BindingsHeader"));
	BindingsHeader->SetText(FText::FromString(TEXT("Key Bindings")));
	FSlateFontInfo HeaderFont = BindingsHeader->GetFont();
	HeaderFont.Size = 12;
	BindingsHeader->SetFont(HeaderFont);
	RootBox->AddChildToVerticalBox(BindingsHeader);

	// Scroll box for action rows
	ActionsScrollBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("ActionsScrollBox"));
	UVerticalBoxSlot* ScrollSlot = RootBox->AddChildToVerticalBox(ActionsScrollBox);
	ScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	ScrollSlot->SetPadding(FMargin(0.f, 5.f));

	// Actions container inside scroll box
	ActionsContainer = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ActionsContainer"));
	ActionsScrollBox->AddChild(ActionsContainer);

	// Sensitivity section
	UTextBlock* SensitivityHeader = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SensitivityHeader"));
	SensitivityHeader->SetText(FText::FromString(TEXT("Sensitivity")));
	SensitivityHeader->SetFont(HeaderFont);
	UVerticalBoxSlot* SensHeaderSlot = RootBox->AddChildToVerticalBox(SensitivityHeader);
	SensHeaderSlot->SetPadding(FMargin(0.f, 10.f, 0.f, 5.f));

	// Mouse sensitivity row
	UHorizontalBox* MouseSensRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("MouseSensRow"));
	RootBox->AddChildToVerticalBox(MouseSensRow);

	UTextBlock* MouseLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("MouseLabel"));
	MouseLabel->SetText(FText::FromString(TEXT("Mouse")));
	FSlateFontInfo LabelFont = MouseLabel->GetFont();
	LabelFont.Size = FontSize;
	MouseLabel->SetFont(LabelFont);
	UHorizontalBoxSlot* MouseLabelSlot = MouseSensRow->AddChildToHorizontalBox(MouseLabel);
	MouseLabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	MouseLabelSlot->SetVerticalAlignment(VAlign_Center);

	MouseSensitivitySlider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass(), TEXT("MouseSensitivitySlider"));
	MouseSensitivitySlider->SetValue(0.2f); // 1.0 / 5.0
	UHorizontalBoxSlot* MouseSliderSlot = MouseSensRow->AddChildToHorizontalBox(MouseSensitivitySlider);
	MouseSliderSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	MouseSliderSlot->SetVerticalAlignment(VAlign_Center);
	MouseSliderSlot->SetPadding(FMargin(10.f, 0.f));

	MouseSensitivityText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("MouseSensitivityText"));
	MouseSensitivityText->SetText(FText::FromString(TEXT("1.0")));
	MouseSensitivityText->SetFont(LabelFont);
	UHorizontalBoxSlot* MouseTextSlot = MouseSensRow->AddChildToHorizontalBox(MouseSensitivityText);
	MouseTextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	MouseTextSlot->SetVerticalAlignment(VAlign_Center);

	// Gamepad sensitivity row
	UHorizontalBox* GamepadSensRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("GamepadSensRow"));
	UVerticalBoxSlot* GamepadRowSlot = RootBox->AddChildToVerticalBox(GamepadSensRow);
	GamepadRowSlot->SetPadding(FMargin(0.f, 5.f, 0.f, 0.f));

	UTextBlock* GamepadLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("GamepadLabel"));
	GamepadLabel->SetText(FText::FromString(TEXT("Gamepad")));
	GamepadLabel->SetFont(LabelFont);
	UHorizontalBoxSlot* GamepadLabelSlot = GamepadSensRow->AddChildToHorizontalBox(GamepadLabel);
	GamepadLabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	GamepadLabelSlot->SetVerticalAlignment(VAlign_Center);

	GamepadSensitivitySlider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass(), TEXT("GamepadSensitivitySlider"));
	GamepadSensitivitySlider->SetValue(0.2f);
	UHorizontalBoxSlot* GamepadSliderSlot = GamepadSensRow->AddChildToHorizontalBox(GamepadSensitivitySlider);
	GamepadSliderSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	GamepadSliderSlot->SetVerticalAlignment(VAlign_Center);
	GamepadSliderSlot->SetPadding(FMargin(10.f, 0.f));

	GamepadSensitivityText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("GamepadSensitivityText"));
	GamepadSensitivityText->SetText(FText::FromString(TEXT("1.0")));
	GamepadSensitivityText->SetFont(LabelFont);
	UHorizontalBoxSlot* GamepadTextSlot = GamepadSensRow->AddChildToHorizontalBox(GamepadSensitivityText);
	GamepadTextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	GamepadTextSlot->SetVerticalAlignment(VAlign_Center);

	// Invert Y row
	UHorizontalBox* InvertYRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("InvertYRow"));
	UVerticalBoxSlot* InvertRowSlot = RootBox->AddChildToVerticalBox(InvertYRow);
	InvertRowSlot->SetPadding(FMargin(0.f, 5.f, 0.f, 0.f));

	UTextBlock* InvertLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("InvertLabel"));
	InvertLabel->SetText(FText::FromString(TEXT("Invert Y Axis")));
	InvertLabel->SetFont(LabelFont);
	UHorizontalBoxSlot* InvertLabelSlot = InvertYRow->AddChildToHorizontalBox(InvertLabel);
	InvertLabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	InvertLabelSlot->SetVerticalAlignment(VAlign_Center);

	InvertYCheckBox = WidgetTree->ConstructWidget<UCheckBox>(UCheckBox::StaticClass(), TEXT("InvertYCheckBox"));
	UHorizontalBoxSlot* InvertCheckSlot = InvertYRow->AddChildToHorizontalBox(InvertYCheckBox);
	InvertCheckSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	InvertCheckSlot->SetVerticalAlignment(VAlign_Center);

	// Status text
	StatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StatusText"));
	StatusText->SetText(FText::FromString(TEXT("")));
	StatusText->SetFont(LabelFont);
	StatusText->SetColorAndOpacity(FSlateColor(FLinearColor::Yellow));
	UVerticalBoxSlot* StatusSlot = RootBox->AddChildToVerticalBox(StatusText);
	StatusSlot->SetPadding(FMargin(0.f, 10.f, 0.f, 0.f));

	// Button row
	UHorizontalBox* ButtonRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ButtonRow"));
	UVerticalBoxSlot* ButtonRowSlot = RootBox->AddChildToVerticalBox(ButtonRow);
	ButtonRowSlot->SetPadding(FMargin(0.f, 10.f, 0.f, 0.f));
	ButtonRowSlot->SetHorizontalAlignment(HAlign_Right);

	// Reset All button
	ResetAllButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ResetAllButton"));
	UTextBlock* ResetAllText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ResetAllText"));
	ResetAllText->SetText(FText::FromString(TEXT("Reset All")));
	ResetAllText->SetFont(LabelFont);
	ResetAllButton->AddChild(ResetAllText);
	UHorizontalBoxSlot* ResetAllSlot = ButtonRow->AddChildToHorizontalBox(ResetAllButton);
	ResetAllSlot->SetPadding(FMargin(0.f, 0.f, 5.f, 0.f));

	// Save button
	SaveButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("SaveButton"));
	UTextBlock* SaveText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SaveText"));
	SaveText->SetText(FText::FromString(TEXT("Save")));
	SaveText->SetFont(LabelFont);
	SaveButton->AddChild(SaveText);
	UHorizontalBoxSlot* SaveSlot = ButtonRow->AddChildToHorizontalBox(SaveButton);
	SaveSlot->SetPadding(FMargin(0.f, 0.f, 5.f, 0.f));

	// Cancel button
	CancelButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CancelButton"));
	UTextBlock* CancelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CancelText"));
	CancelText->SetText(FText::FromString(TEXT("Cancel")));
	CancelText->SetFont(LabelFont);
	CancelButton->AddChild(CancelText);
	ButtonRow->AddChildToHorizontalBox(CancelButton);

	bWidgetsCreated = true;
}

UInputRebindingManager* URebindingSettingsWidget::GetRebindingManager() const
{
	UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(GetWorld());
	if (GameInstance)
	{
		return GameInstance->GetSubsystem<UInputRebindingManager>();
	}
	return nullptr;
}

void URebindingSettingsWidget::RegisterAction(UInputAction* Action, const TArray<FKey>& DefaultBindings)
{
	if (!Action)
	{
		return;
	}

	UInputRebindingManager* Manager = GetRebindingManager();
	if (Manager)
	{
		Manager->RegisterAction(Action, DefaultBindings);
	}

	// Create row widget
	URebindActionRow* Row = CreateActionRow(Action);
	if (Row)
	{
		ActionRows.Add(Action, Row);
	}
}

void URebindingSettingsWidget::RegisterActions(const TMap<UInputAction*, FKey>& ActionsAndDefaults)
{
	for (const auto& Pair : ActionsAndDefaults)
	{
		TArray<FKey> Bindings;
		Bindings.Add(Pair.Value);
		RegisterAction(Pair.Key, Bindings);
	}
}

void URebindingSettingsWidget::SetMappingContext(UInputMappingContext* Context)
{
	UInputRebindingManager* Manager = GetRebindingManager();
	if (Manager)
	{
		Manager->SetMappingContext(Context);
	}
}

URebindActionRow* URebindingSettingsWidget::CreateActionRow(UInputAction* Action)
{
	if (!Action || !ActionsContainer)
	{
		return nullptr;
	}

	// Create the row widget
	TSubclassOf<URebindActionRow> RowClass = ActionRowClass;
	if (!RowClass)
	{
		RowClass = URebindActionRow::StaticClass();
	}
	URebindActionRow* Row = CreateWidget<URebindActionRow>(this, RowClass);
	if (Row)
	{
		Row->SetupAction(Action, GetRebindingManager());
		ActionsContainer->AddChildToVerticalBox(Row);
	}

	return Row;
}

void URebindingSettingsWidget::RefreshAllBindings()
{
	for (const auto& Pair : ActionRows)
	{
		if (Pair.Value)
		{
			Pair.Value->RefreshKeyDisplay();
		}
	}
}

void URebindingSettingsWidget::ResetAllToDefaults()
{
	UInputRebindingManager* Manager = GetRebindingManager();
	if (Manager)
	{
		Manager->ResetAllToDefaults();
		RefreshAllBindings();

		// Reset sliders
		if (MouseSensitivitySlider)
		{
			MouseSensitivitySlider->SetValue(0.2f);
		}
		if (GamepadSensitivitySlider)
		{
			GamepadSensitivitySlider->SetValue(0.2f);
		}
		if (InvertYCheckBox)
		{
			InvertYCheckBox->SetIsChecked(false);
		}

		SetStatus(TEXT("All settings reset to defaults"));
	}
}

void URebindingSettingsWidget::SaveBindings()
{
	UInputRebindingManager* Manager = GetRebindingManager();
	if (Manager)
	{
		if (Manager->SaveBindings())
		{
			SetStatus(TEXT("Settings saved"));
		}
		else
		{
			SetStatus(TEXT("Failed to save settings"));
		}
	}
}

void URebindingSettingsWidget::SetStatus(const FString& Message)
{
	if (StatusText)
	{
		StatusText->SetText(FText::FromString(Message));
	}
}

void URebindingSettingsWidget::OnRebindComplete(UInputAction* Action, FKey NewKey)
{
	CurrentlyRebindingAction = nullptr;
	SetStatus(FString::Printf(TEXT("Bound %s to %s"), *Action->GetName(), *NewKey.GetDisplayName().ToString()));
	OnBindingChanged.Broadcast(Action, NewKey);
}

void URebindingSettingsWidget::OnAnyKeyPressed(FKey Key)
{
	// Could add visual feedback here
}

void URebindingSettingsWidget::OnBindingConflict(UInputAction* ExistingAction, FKey ConflictingKey)
{
	FString ActionName = ExistingAction ? ExistingAction->GetName() : TEXT("Unknown");
	SetStatus(FString::Printf(TEXT("Conflict: %s already uses %s"), *ActionName, *ConflictingKey.GetDisplayName().ToString()));

	// Reset the rebinding state on the current row
	if (CurrentlyRebindingAction)
	{
		TObjectPtr<URebindActionRow>* RowPtr = ActionRows.Find(CurrentlyRebindingAction);
		if (RowPtr && *RowPtr)
		{
			(*RowPtr)->SetRebindingState(false);
		}
	}
}

void URebindingSettingsWidget::OnMouseSensitivityChanged(float Value)
{
	float Sensitivity = Value * 5.0f; // 0-5 range
	Sensitivity = FMath::Max(0.1f, Sensitivity);

	UInputRebindingManager* Manager = GetRebindingManager();
	if (Manager)
	{
		Manager->SetMouseSensitivity(Sensitivity);
	}

	if (MouseSensitivityText)
	{
		MouseSensitivityText->SetText(FText::FromString(FString::Printf(TEXT("%.1f"), Sensitivity)));
	}
}

void URebindingSettingsWidget::OnGamepadSensitivityChanged(float Value)
{
	float Sensitivity = Value * 5.0f;
	Sensitivity = FMath::Max(0.1f, Sensitivity);

	UInputRebindingManager* Manager = GetRebindingManager();
	if (Manager)
	{
		Manager->SetGamepadSensitivity(Sensitivity);
	}

	if (GamepadSensitivityText)
	{
		GamepadSensitivityText->SetText(FText::FromString(FString::Printf(TEXT("%.1f"), Sensitivity)));
	}
}

void URebindingSettingsWidget::OnInvertYChanged(bool bIsChecked)
{
	UInputRebindingManager* Manager = GetRebindingManager();
	if (Manager)
	{
		Manager->SetInvertY(bIsChecked);
	}
}

void URebindingSettingsWidget::OnResetAllClicked()
{
	ResetAllToDefaults();
}

void URebindingSettingsWidget::OnSaveClicked()
{
	SaveBindings();
}

void URebindingSettingsWidget::OnCancelClicked()
{
	// Reload saved bindings to discard changes
	UInputRebindingManager* Manager = GetRebindingManager();
	if (Manager)
	{
		Manager->LoadBindings();
		RefreshAllBindings();

		// Refresh sensitivity values
		if (MouseSensitivitySlider)
		{
			MouseSensitivitySlider->SetValue(Manager->GetMouseSensitivity() / 5.0f);
		}
		if (GamepadSensitivitySlider)
		{
			GamepadSensitivitySlider->SetValue(Manager->GetGamepadSensitivity() / 5.0f);
		}
		if (InvertYCheckBox)
		{
			InvertYCheckBox->SetIsChecked(Manager->GetInvertY());
		}
	}

	SetStatus(TEXT("Changes cancelled"));
}
