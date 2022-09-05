// Fill out your copyright notice in the Description page of Project Settings.


#include "ShooterPlayerController.h"
#include "Blueprint/UserWidget.h"


AShooterPlayerController::AShooterPlayerController() 

{
	PrimaryActorTick.bCanEverTick = true;
}

void AShooterPlayerController::BeginPlay()
{
	Super::BeginPlay();
	UE_LOG(LogTemp, Display, TEXT("cipote0"));
	//Check our HUDOverlayClass TSubclassOf variable was set from Blueprints
	if (HUDOverlayClass)
	{
		UE_LOG(LogTemp, Display, TEXT("cipote1"));
		HUDOverlay = CreateWidget<UUserWidget>(this, HUDOverlayClass);
		if (HUDOverlay)
		{
			UE_LOG(LogTemp, Display, TEXT("cipote2"));
			HUDOverlay->AddToViewport();
			HUDOverlay->SetVisibility(ESlateVisibility::Visible);
		}
	}

}