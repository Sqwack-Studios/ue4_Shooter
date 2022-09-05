// Fill out your copyright notice in the Description page of Project Settings.


#include "ShooterCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Particles/ParticleSystemComponent.h"
#include "Item.h"
#include "Components/WidgetComponent.h"
#include "Weapon.h"
#include "Components/SphereComponent.h"
#include "Components/BoxComponent.h"


// Sets default values
AShooterCharacter::AShooterCharacter() :

	//Variable initialization's

	//Base rates for turning/looking up
	BaseTurnRate(45.f),
	BaseLookUpRate(45.f),
	//Turn rates for aiming/not aiming
	HipTurnRate(90.f),
	HipLookUpRate(90.f),
	AimingTurnRate(20.f),
	AimingLookUpRate(20.f),
	//Mouse look sensitivity scale factors
	MouseHipTurnRate(1.0f),
	MouseHipLookUpRate(1.0f),
	MouseAimingTurnRate(0.2f),
	MouseAimingLookUpRate(0.2f),
	bAiming(false),
	//Camera FOV values
	CameraDefaultFOV(0.f), //Set in Begin Play
	CameraZoomedFOV(35.f),
	CameraCurrentFOV(0.f),
	ZoomInterpSpeed(20.f),
	//Crosshair spread factors
	CrosshairSpreadMultiplier(0.f),
	CrosshairVelocityFactor(0.f),
	CrosshairInAirFactor(0.f),
	CrosshairAimFactor(0.f),
	CrosshairShootingFactor(0.f),
	//Bullet fire timer variables
	ShootTimeDuration(0.05f),
	bFiringBullet(false),
	//Automatic gun fire variables
	AutomaticFireRate(0.1f),
	bShouldFire(true),
	bFireButtonPressed(false),
	//Item trace variables
	bShouldTraceForItems(false),
	OverlappedItemCount(0),
	//Camera interp location
	CameraInterpDistance(250.f),
	CameraInterpElevation(65.f),
	//Starting ammo amounts
	Starting5_56Ammo {69},
	Starting9mmAmmo {150},

	//Combat variables
	CombatState(ECombatState::ECS_Unoccupied)
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	//CameraBoom creation and setup (pulls in towards the character if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 180.f; //The camera follows at this distance behind the character (3 meters)
	CameraBoom->bUsePawnControlRotation = true; //Rotate the arm based on the controller
	CameraBoom->SocketOffset = FVector(0.f, 50.f, 70.f);

	/* Create a follow camera */
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	/* SpringComponent has a socket which name is contained in a static property */
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); 
	FollowCamera->bUsePawnControlRotation = false; //Camera does not rotate relative to spring arm

	/* Don't rotate when the controller does. Let the controller only affect the camera */
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	/* Configure character movement */
	GetCharacterMovement()->bOrientRotationToMovement = false; //Character moves in the direction of input...
	GetCharacterMovement()->RotationRate = FRotator(0.f, 540.f, 0.f);//...at this rotation rate
	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.2f;

}

// Called when the game starts or when spawned
void AShooterCharacter::BeginPlay()
{
	Super::BeginPlay();
	if(FollowCamera)
	{
		CameraDefaultFOV = GetFollowCamera()->FieldOfView;
		CameraCurrentFOV = CameraDefaultFOV;
	}

	//Spawn the default weapon and attach it to the mesh
	
	EquipWeapon(SpawnDefaultWeapon());

	InitializeAmmoMap();
	
}

// Called every frame
void AShooterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	//Handle Interpolation for zoom when aiming
	CameraInterpZoom(DeltaTime);
	//Change look sensitivity based on aiming
	SetLookRates();
	//Calculate crosshair spread
	CalculateCrosshairSpread(DeltaTime);

	// Check OverlappedItemCount, then trace for items
	TraceForItems();
	
	
	
	
	
}

void AShooterCharacter::TurnAtRate(float Rate)
{

	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());

}

void AShooterCharacter::LookUpRate(float Rate)
{
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void AShooterCharacter::Turn(float Value)
{
	float TurnScaleFactor {};
	if (bAiming)
	{
		TurnScaleFactor = MouseAimingTurnRate;
	}
	else
	{
		TurnScaleFactor = MouseHipTurnRate;
	}
	AddControllerYawInput( Value * TurnScaleFactor);
}

void AShooterCharacter::LookUp(float Value)
{
	float LookUpScaleFactor {};
	if (bAiming)
	{
		LookUpScaleFactor = MouseAimingLookUpRate;
	}
	else
	{
		LookUpScaleFactor = MouseHipLookUpRate;
	}
	AddControllerPitchInput( Value * LookUpScaleFactor);
}

void AShooterCharacter::MoveForward(float Value)
{
	if ((Controller != nullptr) && (Value != 0.0f))
	{
		/* Find out which way is forward */
		const FRotator Rotation {Controller->GetControlRotation()};
		const FRotator YawRotation{0, Rotation.Yaw, 0};

		const FVector Direction { FRotationMatrix{YawRotation}.GetUnitAxis(EAxis::X)};
		AddMovementInput(Direction, Value);
	}	 
	
}

void AShooterCharacter::MoveRight(float Value)
{
	if ((Controller != nullptr) && (Value != 0.0f))
	{
		/* Find out which way is right */
		const FRotator Rotation {Controller->GetControlRotation()};
		const FRotator YawRotation{0, Rotation.Yaw, 0};

		const FVector Direction { FRotationMatrix{YawRotation}.GetUnitAxis(EAxis::Y)};
		AddMovementInput(Direction, Value);
	}
}

 /* TODO:Shooting through walls, possible solution:
	1: Perfom a line trace following arm/weapon bones for their lenght looking for wall clipping...
	...if wall is clipping the weapon, we use wall line trace result hit to perform real shooting line trace...
	...with an offset to always hit the wall. Sound/effects handling remains intact.
  */
void AShooterCharacter::FireWeapon()
{
	if (EquippedWeapon == nullptr) return;
	if (CombatState != ECombatState::ECS_Unoccupied) return;
	if (!WeaponHasAmmo()) return;

	//Play fire sound
	PlayFireSound();
	//Send bullet: performing line trace etc
	SendBullet();
	//Play Hip fire montage
	PlayGunFireMontage();
	//Start bullet fire timer for crosshairs
	StartCrosshairBulletFire();
	//Substract 1 from the Weapon's Ammo
	EquippedWeapon->DecrementAmmo();

	StartFireTimer();
	
}


// Called to bind functionality to input
void AShooterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	check(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &AShooterCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AShooterCharacter::MoveRight);
	PlayerInputComponent->BindAxis("TurnRate", this, &AShooterCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AShooterCharacter::LookUpRate);	

	/* We dont need to scale Turn and Look inputs with the mouse because it depends of the mouse delta position.
	The faster you move the mouse, the bigger the delta, more movement. We directly call Yaw and Pitch inputs. */
	PlayerInputComponent->BindAxis("Turn", this, &AShooterCharacter::Turn);
	PlayerInputComponent->BindAxis("LookUp", this, &AShooterCharacter::LookUp);	

	/* We could override Jump functions to implement our own functionality while keeping the standard functionality
	by calling super. */
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAction("FireButton", IE_Pressed, this, &AShooterCharacter::FireButtonPressed);
	PlayerInputComponent->BindAction("FireButton", IE_Released, this, &AShooterCharacter::FireButtonReleased);

	PlayerInputComponent->BindAction("AimingButton", IE_Pressed, this, &AShooterCharacter::AimingButtonPressed);
	PlayerInputComponent->BindAction("AimingButton", IE_Released, this, &AShooterCharacter::AimingButtonReleased);

	PlayerInputComponent->BindAction("Select", IE_Pressed, this, &AShooterCharacter::SelectButtonPressed);
	PlayerInputComponent->BindAction("Select", IE_Released, this, &AShooterCharacter::SelectButtonReleased);

	PlayerInputComponent->BindAction("ReloadButton", IE_Pressed, this, &AShooterCharacter::ReloadButtonPressed);
}

bool AShooterCharacter::GetBeamEndLocation(const FVector &MuzzleSocketLocation, FVector &OutBeamLocation)
{

	FHitResult CrosshairHitResult;
	bool bCrosshairHit = TraceUnderCrosshairs(CrosshairHitResult, OutBeamLocation);

	if(bCrosshairHit)
	{
		//Tentative beam location - still need to trace from gun
		OutBeamLocation = CrosshairHitResult.Location;
	}
	else //no crosshair trace hit
	{
		//OutBeamLocation is the End location for the line trace
	}

	//Perfrom trace from gun barrel
	//Perform a second trace, this time from the gun barrel

	FHitResult WeaponTraceHit;
	const FVector WeaponTraceStart { MuzzleSocketLocation };
	const FVector WeaponTraceEnd { OutBeamLocation + (OutBeamLocation - MuzzleSocketLocation)* 50'000.f};
	GetWorld()->LineTraceSingleByChannel(
		WeaponTraceHit,
		WeaponTraceStart,
		WeaponTraceEnd,
		ECollisionChannel::ECC_Visibility);
			
	if (WeaponTraceHit.bBlockingHit)
	{
		OutBeamLocation = WeaponTraceHit.Location;
		//Spawn impact particles after updating BeamEnd
		if (ImpactParticles)
		{
			UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, OutBeamLocation);
		}

		return true;
	}
	return false;

}


void AShooterCharacter::AimingButtonPressed()
{
	bAiming = true;
}

void AShooterCharacter::AimingButtonReleased()
{
	bAiming = false;
}

void AShooterCharacter::CameraInterpZoom(float DeltaTime)
{
	//SetCurrentCamera FOV
	if (bAiming)
	{
		//Interpolate to zoomed FOV
		CameraCurrentFOV = FMath::FInterpTo(CameraCurrentFOV, CameraZoomedFOV, DeltaTime, ZoomInterpSpeed);
		
	}
	else
	{
		//Interpolate to default FOV
		CameraCurrentFOV = FMath::FInterpTo(CameraCurrentFOV, CameraDefaultFOV, DeltaTime, ZoomInterpSpeed);
		
	}
	GetFollowCamera()->SetFieldOfView(CameraCurrentFOV);
}

void AShooterCharacter::SetLookRates()
{
	if (bAiming)
	{
		BaseTurnRate = AimingTurnRate;
		BaseLookUpRate = AimingLookUpRate;
	}
	else
	{
		BaseTurnRate = HipTurnRate;
		BaseLookUpRate = HipLookUpRate;
	}
}

void AShooterCharacter::CalculateCrosshairSpread(float DeltaTime)
{
	FVector2D WalkSpeedRange{ 0.f, 600.f};
	FVector2D VelocityMultiplierRange {0.f, 1.f};
	FVector Velocity{ GetVelocity() };
	Velocity.Z = 0.1f;

	CrosshairVelocityFactor = FMath::GetMappedRangeValueClamped(WalkSpeedRange, VelocityMultiplierRange, Velocity.Size());

	//Calculate crosshair in air factor
	if (GetCharacterMovement()->IsFalling())//is in air?
	{
		//Spread the crosshairs slowly while in air
		CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 2.25f, DeltaTime, 2.25f);
		
	}
	else //Character is on the ground
	{
		//Shrink the crosshairs fast while on the ground
		CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 0.f, DeltaTime, 30.f);
	}

	//Calculate crosshair aim factor
	if (bAiming)//Are we aiming?
	{
		CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.6f, DeltaTime, 30.f);
	}
	else//Not aiming
	{
		
		CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.f, DeltaTime, 30.f);
	}
	
	//True 0.05 seconds after firing
	if (bFiringBullet)
	{
		CrosshairShootingFactor = FMath::FInterpTo(CrosshairShootingFactor, 0.3f, DeltaTime, 60.f);
	}
	else
	{
		CrosshairShootingFactor = FMath::FInterpTo(CrosshairShootingFactor, 0.f, DeltaTime, 60.f);
	}
	
	
	CrosshairSpreadMultiplier = 
		0.5f + 
		CrosshairVelocityFactor + 
		CrosshairInAirFactor -
		CrosshairAimFactor +
		CrosshairShootingFactor;
}

float AShooterCharacter::GetCrosshairSpreadMultiplier() const
{
	return CrosshairSpreadMultiplier;
}


void AShooterCharacter::StartCrosshairBulletFire()
{
	bFiringBullet = true;
	
	GetWorldTimerManager().SetTimer(CrosshairShootTimer, this, &AShooterCharacter::FinishCrosshairBulletFire, ShootTimeDuration);
}

void AShooterCharacter::FinishCrosshairBulletFire()
{
	bFiringBullet = false;
}

void AShooterCharacter::FireButtonPressed()
{
	bFireButtonPressed = true;
	FireWeapon();

	
}

void AShooterCharacter::FireButtonReleased()
{
	bFireButtonPressed = false;
}

void AShooterCharacter::StartFireTimer()
{
	CombatState = ECombatState::ECS_FireTimerInProgress;
	GetWorldTimerManager().SetTimer(AutoFireTimer, this, &AShooterCharacter::AutoFireReset, AutomaticFireRate);
	
	
}

void AShooterCharacter::AutoFireReset()
{
	CombatState = ECombatState::ECS_Unoccupied;

	if (WeaponHasAmmo())
	{
		if (bFireButtonPressed)
		{
				FireWeapon();
		}
	}
	else
	{
		//Reload weapon
		ReloadWeapon();
	}
	
	
}

bool AShooterCharacter::TraceUnderCrosshairs(FHitResult &OutHitResult, FVector &OutHitLocation)
{

	//Get Viewport Size
	FVector2D ViewportSize;
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}

	/*Get screen space location of crosshairs  */
	FVector2D CrosshairLocation(ViewportSize.X / 2.f, ViewportSize.Y / 2.f);
	// CrosshairLocation.Y -= 50.f;
	FVector CrosshairWorldPosition;
	FVector CrosshairWorldDirection;

	/*Get world position and direction of crosshair */
	bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(UGameplayStatics::GetPlayerController(this, 0), 
	CrosshairLocation,
	CrosshairWorldPosition,
	CrosshairWorldDirection);

	if (bScreenToWorld)
	{
		//Trace from Crosshair world location outward
		const FVector Start{CrosshairWorldPosition};
		const FVector End{Start + CrosshairWorldDirection * 50'000.f};
		OutHitLocation = End;
		GetWorld()->LineTraceSingleByChannel(
			OutHitResult,
			Start,
			End,
			ECollisionChannel::ECC_Visibility);
		
		if (OutHitResult.bBlockingHit)
		{
			OutHitLocation = OutHitResult.Location;
			return true;
		}
		
	}
	return false;
}

void AShooterCharacter::IncrementOverlappedItemCount(int8 Amount)
{
	if (OverlappedItemCount + Amount <= 0)
	{
		OverlappedItemCount = 0;
		bShouldTraceForItems = false;
	}
	else
	{
		OverlappedItemCount += Amount;
		bShouldTraceForItems = true;
	}
	
}

void AShooterCharacter::TraceForItems()
{
	if (bShouldTraceForItems)
	{
		FHitResult ItemTraceResult;
		FVector HitLocation;
		TraceUnderCrosshairs(ItemTraceResult, HitLocation);
		if (ItemTraceResult.bBlockingHit)
		{
			TraceHitItem = Cast<AItem>(ItemTraceResult.Actor);
			if (TraceHitItem && TraceHitItem->GetPickupWidget())
			{	
			
				//Show Item's PickUp Widget

				TraceHitItem->GetPickupWidget()->SetVisibility(true);

				//Store a reference to TraceHitItem for next frame
				
			}

			if (TraceHitItemLastFrame)
			{
				if (TraceHitItem != TraceHitItemLastFrame)
				{	//We are hitting a different AItem this frame from last frame 
					//Or AItem is null.
					TraceHitItemLastFrame->GetPickupWidget()->SetVisibility(false);
				}
				
			}

			TraceHitItemLastFrame = TraceHitItem;
			
		}
	}
	else if(TraceHitItemLastFrame)
	{
		//No longer overlapping any items, last frame item should not show widget
		TraceHitItemLastFrame->GetPickupWidget()->SetVisibility(false);
	}
}

AWeapon* AShooterCharacter::SpawnDefaultWeapon()
{
	if (DefaultWeaponClass)
	{
		return GetWorld()->SpawnActor<AWeapon>(DefaultWeaponClass);
	}
	
	return nullptr;
}

void AShooterCharacter::EquipWeapon(AWeapon* WeaponToEquip)
{

	if (WeaponToEquip)
	{

		const USkeletalMeshSocket* HandSocket = GetMesh()->GetSocketByName(FName("RightHandSocket"));

		if (HandSocket)
		{
			//Attach the weapon to the hand socket RightHandSocket
			HandSocket->AttachActor(WeaponToEquip, GetMesh());
		}
		
		//Set EquippedWeapon to the newly spawned weapon
		EquippedWeapon = WeaponToEquip;
		EquippedWeapon->SetItemState(EItemState::EIS_Equipped);
	}

}

void AShooterCharacter::DropWeapon()
{
	if(EquippedWeapon)
	{
		FDetachmentTransformRules DetachmentTransformRules(EDetachmentRule::KeepWorld, true);
		EquippedWeapon->GetItemMesh()->DetachFromComponent(DetachmentTransformRules);

		EquippedWeapon->SetItemState(EItemState::EIS_Falling);
		EquippedWeapon->ThrowWeapon();
		
	}
}

void AShooterCharacter::SelectButtonPressed()
{	
	if (TraceHitItem)
	{
		TraceHitItem->StartItemCurve(this);
	}
	
	
}

void AShooterCharacter::SelectButtonReleased()
{

}

void AShooterCharacter::SwapWeapon(AWeapon* WeaponToSwap)
{
	DropWeapon();
	EquipWeapon(WeaponToSwap);
	TraceHitItem = nullptr;
	TraceHitItemLastFrame = nullptr;
}

FVector AShooterCharacter::GetCameraInterpLocation()
{
	const FVector CameraWorldLocation { FollowCamera->GetComponentLocation() };
	const FVector CameraForward { FollowCamera->GetForwardVector() };
	// Desired = CameraWorldLocation + A * Forward + B * Up

	return CameraWorldLocation + CameraInterpDistance * CameraForward + CameraInterpElevation * (FVector {FVector::UpVector});
}

void AShooterCharacter::GetPickupItem(AItem *Item)
{
	auto Weapon = Cast<AWeapon>(Item);
	if (Weapon)
	{
		SwapWeapon(Weapon);
	}
	
}

void AShooterCharacter::InitializeAmmoMap()
{
	AmmoMap.Add(EAmmoType::EAT_5_56mm, Starting5_56Ammo);
	AmmoMap.Add(EAmmoType::EAT_9mm, Starting9mmAmmo);
}

bool AShooterCharacter::WeaponHasAmmo()
{
	if (EquippedWeapon == nullptr) return false;
	
	return EquippedWeapon->GetAmmo() > 0;
}

void AShooterCharacter::PlayFireSound()
{
	if (FireSound)
	{
		UGameplayStatics::PlaySound2D(this, FireSound);
	}
}

void AShooterCharacter::SendBullet()
{
	const USkeletalMeshSocket* BarrelSocket = EquippedWeapon->GetItemMesh()->GetSocketByName("BarrelSocket");
	if (BarrelSocket)
	{
		const FTransform SocketTransform = BarrelSocket->GetSocketTransform(EquippedWeapon->GetItemMesh());

		if (MuzzleFlash)
		{

			UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), MuzzleFlash, SocketTransform);

		}

		FVector BeamEnd;
		bool bBeamEnd = GetBeamEndLocation(SocketTransform.GetLocation(), BeamEnd);
		if (bBeamEnd)
		{
			if (BeamParticles)
			{
				UParticleSystemComponent* Beam = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), BeamParticles, SocketTransform);
				if (Beam)
				{
					Beam->SetVectorParameter(FName("Target"), BeamEnd);
				}
			}
		}
	}
}

void AShooterCharacter::PlayGunFireMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && HipFireMontage)
	{
		AnimInstance->Montage_Play(HipFireMontage);
		AnimInstance->Montage_JumpToSection(FName("StartFire"));
	}
}

void AShooterCharacter::ReloadButtonPressed()
{
	ReloadWeapon();
}

void AShooterCharacter::ReloadWeapon()
{
	if (CombatState != ECombatState::ECS_Unoccupied) return;

	//Do we have ammo of the correct type?
	//TODO: Create bool CarryingAmmo()
	if (true) //replace with CarryingAmmo()
	{
		//TODO: Create an enum for Weapon Type
		//TODO: switch on EquippedWeapon->WeaponType
		FName MontageSection{ TEXT("Reload SMG") };

		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if (ReloadMontage && AnimInstance)
		{
			AnimInstance->Montage_Play(ReloadMontage);
			AnimInstance->Montage_JumpToSection(MontageSection);
		}
	}

	
	
}

void AShooterCharacter::FinishReloading()
{
	//TODO: Update AmmoMap

	CombatState = ECombatState::ECS_Unoccupied;
}
