// Copyright Epic Games, Inc. All Rights Reserved.

#include "VirtualJoystickWidget.h"
#include "InputStreamlinerRuntimeModule.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedPlayerInput.h"

UVirtualJoystickWidget::UVirtualJoystickWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsFocusable(false);
}

void UVirtualJoystickWidget::ResetToCenter()
{
	CurrentValue = FVector2D::ZeroVector;
	bIsActive = false;
	ActiveTouchIndex = -1;

	OnJoystickValueChanged(CurrentValue);
	OnJoystickDeactivated();
}

FVector2D UVirtualJoystickWidget::GetValueWithDeadZone() const
{
	float Magnitude = CurrentValue.Size();

	if (Magnitude < DeadZone)
	{
		return FVector2D::ZeroVector;
	}

	// Remap from dead zone to 1
	float RemappedMagnitude = (Magnitude - DeadZone) / (1.0f - DeadZone);
	return CurrentValue.GetSafeNormal() * RemappedMagnitude;
}

FReply UVirtualJoystickWidget::NativeOnTouchStarted(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent)
{
	if (ActiveTouchIndex != -1)
	{
		// Already have an active touch
		return FReply::Unhandled();
	}

	ActiveTouchIndex = InGestureEvent.GetPointerIndex();
	bIsActive = true;

	FVector2D LocalPosition = InGeometry.AbsoluteToLocal(InGestureEvent.GetScreenSpacePosition());

	if (bIsFloating)
	{
		// Set center to touch position
		CenterPosition = LocalPosition;
	}
	else
	{
		// Use widget center
		CenterPosition = InGeometry.GetLocalSize() * 0.5f;
	}

	UpdateJoystickPosition(LocalPosition, InGeometry);
	OnJoystickActivated();

	return FReply::Handled().CaptureMouse(GetCachedWidget().ToSharedRef());
}

FReply UVirtualJoystickWidget::NativeOnTouchMoved(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent)
{
	if (InGestureEvent.GetPointerIndex() != ActiveTouchIndex)
	{
		return FReply::Unhandled();
	}

	FVector2D LocalPosition = InGeometry.AbsoluteToLocal(InGestureEvent.GetScreenSpacePosition());
	UpdateJoystickPosition(LocalPosition, InGeometry);

	return FReply::Handled();
}

FReply UVirtualJoystickWidget::NativeOnTouchEnded(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent)
{
	if (InGestureEvent.GetPointerIndex() != ActiveTouchIndex)
	{
		return FReply::Unhandled();
	}

	if (bAutoCenter)
	{
		ResetToCenter();
	}
	else
	{
		bIsActive = false;
		ActiveTouchIndex = -1;
		OnJoystickDeactivated();
	}

	return FReply::Handled().ReleaseMouseCapture();
}

void UVirtualJoystickWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// Continuously inject the input value while active
	if (bIsActive || CurrentValue.SizeSquared() > KINDA_SMALL_NUMBER)
	{
		InjectInputValue(GetValueWithDeadZone());
	}
}

void UVirtualJoystickWidget::UpdateJoystickPosition(const FVector2D& TouchPosition, const FGeometry& Geometry)
{
	// Calculate offset from center
	FVector2D Offset = TouchPosition - CenterPosition;

	// Calculate the maximum offset (half the visual size)
	float MaxOffset = VisualSize * 0.5f;

	// Clamp to maximum range
	float OffsetMagnitude = Offset.Size();
	if (OffsetMagnitude > MaxOffset)
	{
		Offset = Offset.GetSafeNormal() * MaxOffset;
	}

	// Normalize to -1 to 1 range
	CurrentValue = Offset / MaxOffset;

	OnJoystickValueChanged(CurrentValue);
}

void UVirtualJoystickWidget::InjectInputValue(const FVector2D& Value)
{
	if (!LinkedAction)
	{
		return;
	}

	// Get the local player controller
	APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
		return;
	}

	// Get the Enhanced Input Local Player Subsystem
	ULocalPlayer* LocalPlayer = PC->GetLocalPlayer();
	if (!LocalPlayer)
	{
		return;
	}

	UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	if (!Subsystem)
	{
		return;
	}

	// Inject the input value
	Subsystem->InjectInputForAction(LinkedAction, FInputActionValue(Value), {}, {});
}
