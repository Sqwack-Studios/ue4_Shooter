// Fill out your copyright notice in the Description page of Project Settings.


#include "ShooterAnimInstance.h"
#include "ShooterCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"



void UShooterAnimInstance::UpdateAnimationProperties(float DeltaTime)
{

    /* Try to get playercharacter if its null */
    if (ShooterCharacter == nullptr)
    {
        ShooterCharacter = Cast<AShooterCharacter>(TryGetPawnOwner());
    }
    if (ShooterCharacter)
    {
        /* Get the lateral speed of the character from velocity */
        FVector Velocity{ ShooterCharacter->GetVelocity()};
        Velocity.Z = 0;
        Speed = Velocity.Size();

        /* Is the character in the air? */
        bIsInAir = ShooterCharacter->GetCharacterMovement()->IsFalling();

        /* Is the character accelerating (moving)? */

        if (IsMoving())
        {
            bIsAccelerating = true;
            LastMovementOffsetYaw = MovementOffsetYaw;
        }
        else
        {
            bIsAccelerating = false;
            
        }
    

        
        FRotator AimRotation = ShooterCharacter->GetBaseAimRotation();
        FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(ShooterCharacter->GetVelocity());

        // FString RotationMessage = FString::Printf(TEXT("Base Aim Rotation: %f"), AimRotation.Yaw);
        FString MovementMessage = FString::Printf(TEXT("Movement: %f"), MovementRotation.Yaw); 

        MovementOffsetYaw = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation).Yaw;
        
        
        
        FString OffsetMessage = FString::Printf(TEXT("Movement rotation: %f"), MovementOffsetYaw); 

        bAiming = ShooterCharacter->GetAiming();

   
        
        

        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(1, 0.f, FColor::White, OffsetMessage);

        }
        
        
    }
    
    

}

void UShooterAnimInstance::NativeInitializeAnimation()
{

    ShooterCharacter = Cast<AShooterCharacter>(TryGetPawnOwner());   
}
