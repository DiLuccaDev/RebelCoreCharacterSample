// Copyright 2026 Michael DiLucca.

#include "RCCharacter.h"

#include "ChargedProjectile.h"
#include "RCCharacterMovementComponent.h"
#include "RCFollowCameraComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "InputActionValue.h"
#include "RCDeathManagerComponent.h"
#include "RCVitalityManagerComponent.h"
#include "RCVitalityObject.h"
#include "Components/CapsuleComponent.h"


ARCCharacter::ARCCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<URCCharacterMovementComponent>(
		ACharacter::CharacterMovementComponentName))
{
	RCCharacterMovementComponent = Cast<URCCharacterMovementComponent>(GetCharacterMovement());

	// Create a configurable camera to follow the player
	FollowCamera = CreateDefaultSubobject<URCFollowCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(RootComponent);
	FollowCamera->bUsePawnControlRotation = false;

	// Create Death Manager component
	DeathManager = CreateDefaultSubobject<URCDeathManagerComponent>(TEXT("DeathManager"));

	// Create Vitality manager component
	VitalityManager = CreateDefaultSubobject<URCVitalityManagerComponent>(TEXT("VitalityManager"));

	// Create the Melee attack component
	MeleeAttackComponent = CreateDefaultSubobject<URCMeleeAttackComponent>(TEXT("RCMeleeAttack"));

	// Disable controller rotation									
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
}

void ARCCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Shield buffer execution
	if (bWantsToShield)
	{
		const float CurrentTime = GetWorld()->GetTimeSeconds();
		if ((LastEquipTime + EquipDelay) < CurrentTime)
		{
			ActivateShield();
		}
	}
	// Ranged attack buffer execution
	if (bWantsToShoot)
	{
		const float CurrentTime = GetWorld()->GetTimeSeconds();
		if ((LastEquipTime + EquipDelay) < CurrentTime)
		{
			RangedAttack();
		}
	}
	// Charged range attack buffer execution
	if (bWantsToReleaseShoot)
	{
		const float CurrentTime = GetWorld()->GetTimeSeconds();
		if ((LastEquipTime + EquipDelay) < CurrentTime)
		{
			ReleaseRangedAttack();
		}
	}
	// Melee attack buffer execution
	if (bWantsToMelee)
	{
		const float CurrentTime = GetWorld()->GetTimeSeconds();
		if ((LastEquipTime + EquipDelay) < CurrentTime)
		{
			MeleeAttack();
		}
	}
	// Charged melee attack buffer execution
	else if (bWantsToReleaseMelee)
	{
		const float CurrentTime = GetWorld()->GetTimeSeconds();
		if ((LastEquipTime + EquipDelay) < CurrentTime)
		{
			ReleaseMeleeAttack();
		}
	}
}

void ARCCharacter::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// Hide pre-existing gun arm that was already on the mesh via bone ID
	if (BoneToRemove != NAME_None)
	{
		GetMesh()->HideBoneByName(BoneToRemove, PBO_None);
	}

	SetupEquippable(EEquippable::EE_Shield);
	SetupEquippable(EEquippable::EE_Melee);
	SetupEquippable(EEquippable::EE_Ranged);
}

void ARCCharacter::BeginPlay()
{
	ShieldEquippableObject->SetVisibility(false);

	//Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<
			UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}

	// Inform movement component of our initial orientation
	RCCharacterMovementComponent->SetFacingRight(GetActorForwardVector().X > 0);

	// Bind Death
	if (DeathManager)
	{
		DeathManager->OnIsDead.AddDynamic(this, &ARCCharacter::OnIsDead);
	}

	// Setup range arm as default
	SwapEquippable(EEquippable::EE_Ranged);

	Super::BeginPlay();
}

void ARCCharacter::SetupEquippable(EEquippable equip)
{
	if (equip == EEquippable::EE_Ranged)
	{
		if (RangedEquippableMesh)
		{
			RangedEquippable = NewObject<UStaticMeshComponent>(this, TEXT("RangedEquippable"));
			RangedEquippable->RegisterComponent();
			RangedEquippable->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale,
			                                    RangedAttachName);
			RangedEquippable->SetStaticMesh(RangedEquippableMesh);
			RangedEquippable->SetRelativeTransform(FTransform(FRotator(-90, -90, 0)));
			AddInstanceComponent(RangedEquippable);
		}
	}
	if (equip == EEquippable::EE_Melee)
	{
		if (MeleeEquippableMesh)
		{
			RangedEquippable = NewObject<UStaticMeshComponent>(this, TEXT("RangedEquippable"));
			RangedEquippable->RegisterComponent();
			RangedEquippable->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale,
			                                    RangedAttachName);
			RangedEquippable->SetStaticMesh(RangedEquippableMesh);
			RangedEquippable->SetRelativeTransform(FTransform(FRotator(-90, -90, 0)));
			AddInstanceComponent(RangedEquippable);
		}
	}
	if (equip == EEquippable::EE_Melee)
	{
		if (MeleeEquippableMesh)
		{
			MeleeEquippable = NewObject<UStaticMeshComponent>(this, TEXT("MeleeEquippable"));
			MeleeEquippable->RegisterComponent();
			MeleeEquippable->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale,
			                                   MeleeAttachName);
			MeleeEquippable->SetStaticMesh(MeleeEquippableMesh);
			MeleeEquippable->SetRelativeTransform(FTransform(FRotator(90, -90, 0)));
			MeleeEquippable->SetRelativeScale3D(FVector(1.f,1.1f,1.f));
			AddInstanceComponent(MeleeEquippable);
		}
	}
	if (equip == EEquippable::EE_Shield)
	{
		if (ShieldEquippableClass)
		{
			// SpawnParams to ensure it belongs to this actor
			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = this;
			SpawnParams.Instigator = GetInstigator();
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			// Spawn the shield actor
			ShieldEquippableObject = GetWorld()->SpawnActor<ARCPlayerShield>(ShieldEquippableClass, SpawnParams);

			if (ShieldEquippableObject)
			{
				// Attach to mesh socket or bone
				ShieldEquippableObject->AttachToComponent(
					GetMesh(),
					FAttachmentTransformRules::SnapToTargetIncludingScale,
					ShieldAttachName
				);
			}
		}
	}
}

TSubclassOf<ARCProjectile> ARCCharacter::GetProjectileForCharge(int32 ChargeTime)
{
	for (int32 i = ChargeTime; i >= 0; --i)
	{
		for (const FChargedProjectile& Entry : RangedProjectiles)
		{
			if (Entry.ChargeTimeInSeconds == i)
			{
				return Entry.ProjectileClass;
			}
		}
	}

	return nullptr;
}

void ARCCharacter::OnIsDead(URCDeathManagerComponent* DeadManager)
{
	// If we have a death montage for the character, play it. This is dynamic can can be situationally changed.
	if (DeathMontage)
	{
		PlayAnimMontage(DeathMontage, DeathMontagePlayRate);
	}

	// Stop all player input and movement to allow death to play out
	DisableInput(GetLocalViewingPlayerController());
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	GetRCCharacterMovement()->StopActiveMovement();

	// Set a timer based on the animation time to eventually destroy the player model.
	GetWorldTimerManager().SetTimer(DeathMontageTimerHandle, this, &ARCCharacter::OnDeathMontageFinished,
	                                DeathMontage->GetPlayLength() * (1 / DeathMontagePlayRate) -
	                                AdjustedDeathMontageEndTimeReduction, false);
}

void ARCCharacter::OnDeathMontageFinished()
{
	GetMesh()->SetVisibility(false, true);
	
	// Destroy any outstanding models
	// This could potentially be handled better
	if (MeleeEquippable != nullptr)
	{
		MeleeEquippable->DestroyComponent();
		MeleeEquippable = nullptr;
	}

	if (RangedEquippable != nullptr)
	{
		RangedEquippable->DestroyComponent();
		RangedEquippable = nullptr;
	}

	if (AActor* ShieldActor = Cast<AActor>(ShieldEquippableObject))
	{
		ShieldActor->Destroy();
		ShieldEquippableObject = nullptr;
	}
	else if (ShieldEquippableObject != nullptr)
	{
		ShieldEquippableObject = nullptr;
	}

	DeathManager->OnDeathMontageFinished.Broadcast();
}

bool ARCCharacter::ApplyDamageEvent_Implementation(FRCDamageEvent& damageEvent)
{
	VitalityManager->HandleApplyDamageEvent(damageEvent);

	TArray<URCVitalityObject*> healthVitality;
	VitalityManager->GetVitalityObjectsByTag(HealthTag, healthVitality);
	if (healthVitality.IsValidIndex(0) && !healthVitality.IsEmpty())
	{
		OnHealthChanged.Broadcast(healthVitality[0]->GetCurrentVitality(), healthVitality[0]->GetNormalizedVitality());
		if (healthVitality[0]->GetCurrentVitality() <= 0)
		{
			AActor* SourceActor = Cast<AActor>(damageEvent.Source);
			DeathManager->SetIsDead(true, SourceActor);
		}
		OnInvincibilityStart(damageEvent);
		StartInvincibility();
	}

	return IRCDamageableInterface::ApplyDamageEvent_Implementation(damageEvent);
}

void ARCCharacter::StartInvincibility()
{
	SetCanBeDamaged_Implementation(false);
	GetWorld()->GetTimerManager().SetTimer(
		IFrameTimerHandle,
		this,
		&ARCCharacter::EndInvincibility,
		InvincibilityDuration,
		false
	);
}

void ARCCharacter::EndInvincibility()
{
	SetCanBeDamaged_Implementation(true);
}

void ARCCharacter::OnInvincibilityStart_Implementation(FRCDamageEvent& damageEvent)
{
	ResetJumpState();
}

void ARCCharacter::SetCanBeDamaged_Implementation(bool newValue)
{
	IRCDamageableInterface::SetCanBeDamaged_Implementation(newValue);
	bCanBeDamaged = newValue;
}

bool ARCCharacter::GetCanBeDamaged_Implementation()
{
	return bCanBeDamaged;
}

void ARCCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		/// + MOVEMENT +
		// Jump
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &ARCCharacter::JumpOrDrop);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ARCCharacter::ReleaseJump);
		// Crouch / Drop
		EnhancedInputComponent->BindAction(CrouchDropAction, ETriggerEvent::Triggered, this, &ARCCharacter::CrouchDrop);
		EnhancedInputComponent->BindAction(CrouchDropAction, ETriggerEvent::Completed, this,
		                                   &ARCCharacter::StopCrouchDrop);
		// Move
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ARCCharacter::Move);
		// Dash
		EnhancedInputComponent->BindAction(DashAction, ETriggerEvent::Triggered, this, &ARCCharacter::Dash);

		/// + COMBAT +
		//Ranged
		EnhancedInputComponent->BindAction(RangedAttackAction, ETriggerEvent::Started, this,
		                                   &ARCCharacter::RangedAttack);
		EnhancedInputComponent->BindAction(RangedAttackAction, ETriggerEvent::Completed, this,
		                                   &ARCCharacter::ReleaseRangedAttack);
		//Melee
		EnhancedInputComponent->BindAction(MeleeAttackAction, ETriggerEvent::Started, this,
		                                   &ARCCharacter::MeleeAttack);
		EnhancedInputComponent->BindAction(MeleeAttackAction, ETriggerEvent::Completed, this,
		                                   &ARCCharacter::ReleaseMeleeAttack);
		//Shield
		EnhancedInputComponent->BindAction(ShieldAction, ETriggerEvent::Started, this, &ARCCharacter::ActivateShield);
		EnhancedInputComponent->BindAction(ShieldAction, ETriggerEvent::Completed, this, &ARCCharacter::ReleaseShield);

		//Heal
		EnhancedInputComponent->BindAction(HealAction, ETriggerEvent::Started, this, &ARCCharacter::ActivateHeal);
		EnhancedInputComponent->BindAction(HealAction, ETriggerEvent::Completed, this, &ARCCharacter::ReleaseHeal);

		/// + GAME MISC +
		//Pause
		EnhancedInputComponent->BindAction(PauseAction, ETriggerEvent::Triggered, this, &ARCCharacter::PauseGame);
		//Menu
		EnhancedInputComponent->BindAction(MenuAction, ETriggerEvent::Triggered, this, &ARCCharacter::OpenMenu);
	}
}

void ARCCharacter::Move(const FInputActionValue& Value)
{
	float MovementValue = Value.Get<float>();

	if (FMath::Abs(MovementValue) > 1.f)
	{
		MovementValue /= 50.f;
	}

	if (FMath::Abs(MovementValue) < .7f)
	{
		MovementValue = 0.001f * DirectionFacing;
	}
	else
	{
		DirectionFacing = FMath::Sign(MovementValue);
	}

	RCCharacterMovementComponent->AddInputVector(FVector::ForwardVector * MovementValue);
}

void ARCCharacter::JumpOrDrop_Implementation()
{
	if (bIsCrouched) // attempt to drop through platform, if any
	{
		RCCharacterMovementComponent->WantsToPlatformDrop = true;
	}
	else
	{
		Jump();
	}
}

void ARCCharacter::CrouchDrop_Implementation()
{
	if (CanCrouch())
	{
		Crouch();
	}
}

void ARCCharacter::StopCrouchDrop_Implementation()
{
	UnCrouch();
}

bool ARCCharacter::CanDash_Implementation() const
{
	// Default implementation: can dash out of any action
	// Note that dash cooldown and max dashes per jump are handled
	//   in the character movement component 
	return true;
}

void ARCCharacter::Dash_Implementation()
{
	// Normal dash logic
	RCCharacterMovementComponent->WantsToDash = true;
}

void ARCCharacter::ReleaseJump_Implementation()
{
	StopJumping();
}

void ARCCharacter::RangedAttack_Implementation()
{
	// Get time, then see if we are buffering the input.
	const float CurrentTime = GetWorld()->GetTimeSeconds();
	if (!(LastEquipTime + EquipDelay < CurrentTime)
		&& CurrentEquippable != EEquippable::EE_Ranged)
	{
		bWantsToShoot = true;
		return;
	}

	// Remove the buffer
	bWantsToShoot = false;

	// Inform BP / Anim
	RCCharacterMovementComponent->IsShooting = true;
	if (SwapEquippable(EEquippable::EE_Ranged))
	{
		RCCharacterMovementComponent->bIsShielding = false;
	}

	RangedAttackChargeStartTime = CurrentTime;
	PlayTriggerRangedSuccessVFX();
	PlayFireRangedSuccessVFX();
	FireRangedAttack();
}

void ARCCharacter::ReleaseRangedAttack_Implementation()
{
	const float CurrentTime = GetWorld()->GetTimeSeconds();
	if (!(LastEquipTime + EquipDelay < CurrentTime)
		&& CurrentEquippable != EEquippable::EE_Ranged)
	{
		bWantsToReleaseShoot = true;
		bWantsToReleaseMelee = false;
		return;
	}

	if (RangedAttackChargeStartTime == MAX_FLT) return;

	// Cache the current time input was released then compare to ranged attack delay.
	if (!(LastRangedAttackTime + RangedAttackDelay < CurrentTime))
	{
		bWantsToReleaseShoot = true;
		return;
	}

	if (CurrentEquippable != EEquippable::EE_Ranged) return;

	bWantsToShoot = false;
	bWantsToReleaseShoot = false;

	// Calculate how long the player has been charging their attack clamped to a max of 10.
	int32 ChargeTime = FMath::Clamp(FMath::FloorToInt32(GetWorld()->GetTimeSeconds() - RangedAttackChargeStartTime), 0,
	                                10);
	if (ChargeTime >= 1)
	{
		FireRangedAttack();
		LastRangedAttackTime = CurrentTime;
		PlayFireRangedSuccessVFX();
	}
	
	RangedAttackChargeStartTime = MAX_FLT;
	PlayReleaseRangedSuccessVFX();
	LastEquipTime = CurrentTime;
}

void ARCCharacter::FireRangedAttack()
{
	// We are eligible for fire, let's define params.
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.Instigator = this;

	// Set the location and rotation where we want to spawn the projectile.
	const FVector ActorLocation = GetActorLocation();
	FVector SpawnLocation = ActorLocation + FVector(0, 0, GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
	float SpawnX = FMath::Sign(GetActorForwardVector().X) * (GetCapsuleComponent()->GetScaledCapsuleRadius() *
		ProjectileSpawnDistanceMultiplierMod);
	SpawnLocation.X += SpawnX;
	SpawnLocation += ProjectileSpawnLocationModifier;
	const FRotator SpawnRotation = FRotator(FMath::Sign(GetActorForwardVector().X) * 90 - 90, 0, 0);

	if (RangedProjectiles.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("The HERO ranged projectile array is empty! Assign a value to the array."));
		return;
	}

	// Calculate how long the player has been charging their attack clamped to a max of 10.
	int32 ChargeTime = FMath::Clamp(FMath::FloorToInt32(GetWorld()->GetTimeSeconds() - RangedAttackChargeStartTime), 0,
	                                10);

	// Spawn the projectile based on what is set in the inspector.
	ARCProjectile* Projectile = GetWorld()->SpawnActor<ARCProjectile>(
		GetProjectileForCharge(ChargeTime), SpawnLocation,
		SpawnRotation, SpawnParameters);

	if (!Projectile)
	{
		UE_LOG(LogTemp, Warning, TEXT("Projectile failed to spawn!"));
		return;
	}

	// Add an impulse to the spawned projectile's root primitive component. (MUST HAVE PHYSICS ENABLED)
	FVector ProjectileImpulse = FVector(FMath::Sign(GetActorForwardVector().X) * ProjectileImpulseForce, 0, 0);
	UPrimitiveComponent* root = Cast<UPrimitiveComponent>(Projectile->GetRootComponent());

	// Charge attack logic
	root->AddImpulse((GetVelocity().X * FVector::ForwardVector) + ProjectileImpulse);
}

void ARCCharacter::MeleeAttack_Implementation()
{
	// Get time, then see if we are buffering the input.
	const float CurrentTime = GetWorld()->GetTimeSeconds();
	if ((LastEquipTime + EquipDelay) > CurrentTime
		&& CurrentEquippable != EEquippable::EE_Melee)
	{
		bWantsToMelee = true;
		return;
	}
	bWantsToMelee = false;

	if (RangedAttackChargeStartTime > 0 &&
		RangedAttackChargeStartTime < MAX_FLT &&
		CurrentEquippable == EEquippable::EE_Ranged)
	{
		PlayReleaseRangedFailureVFX();
	}

	// Inform BP / Anim
	RCCharacterMovementComponent->IsMeleeing = true;
	if (SwapEquippable(EEquippable::EE_Melee))
	{
		RCCharacterMovementComponent->bIsShielding = false;
	}

	if (LastMeleeAttackTime + MeleeAttackDelay > CurrentTime) return;	// Melee cooldown
	PlayMeleeSuccessVFX();
	LastMeleeAttackTime = CurrentTime;
	LastEquipTime = CurrentTime;
}

void ARCCharacter::ReleaseMeleeAttack_Implementation()
{
	//UE_LOG(LogTemp, Log, TEXT("Melee Release Attempted"));
	// Cache the current time input was released then compare to melee attack delay.
	float CurrentTime = GetWorld()->GetTimeSeconds();
	if ((LastEquipTime + EquipDelay) > CurrentTime
		&& CurrentEquippable != EEquippable::EE_Melee)
	{
		bWantsToReleaseShoot = false;
		bWantsToReleaseMelee = true;
		return;
	}
	if (RangedAttackChargeStartTime > 0 && RangedAttackChargeStartTime < MAX_FLT) return; //
	if (LastMeleeAttackTime + MeleeAttackDelay > CurrentTime) return;	// Melee cooldown
	if (CurrentEquippable != EEquippable::EE_Melee) return; //

	bWantsToMelee = false;
	bWantsToReleaseMelee = false;
	RCCharacterMovementComponent->IsMeleeing = false;
}

void ARCCharacter::ActivateShield_Implementation()
{
	// Shield Buffer for when the player holds down the input
	//	but equip delays won't allow for the ability to fire yet.
	float CurrentTime = GetWorld()->GetTimeSeconds();
	if ((LastEquipTime + EquipDelay) > CurrentTime)
	{
		bWantsToShield = true;
		return;
	}

	// Activate shield
	bWantsToShield = false; // Clear buffer
	RangedAttackChargeStartTime = MAX_FLT;
	RCCharacterMovementComponent->bIsShielding = SwapEquippable(EEquippable::EE_Shield);
	PlayReleaseRangedFailureVFX();
	PlayShieldSuccessVFX();
}

void ARCCharacter::ReleaseShield_Implementation()
{
	bWantsToShield = false;
	if (CurrentEquippable != EEquippable::EE_Shield) return;
	RCCharacterMovementComponent->bIsShielding = false;
	SwapEquippable(LastEquippable, true);
}

void ARCCharacter::PauseGame_Implementation()
{
	UE_LOG(LogTemp, Display, TEXT("PauseGame_Implementation"));
}

void ARCCharacter::OpenMenu_Implementation()
{
	UE_LOG(LogTemp, Display, TEXT("OpenMenu_Implementation"));
}

void ARCCharacter::ActivateHeal_Implementation()
{
	UE_LOG(LogTemp, Display, TEXT("ActivateHeal_Implementation"));
	bIsHealing = true;
	PlayShieldSuccessVFX();
}

void ARCCharacter::ReleaseHeal_Implementation()
{
	UE_LOG(LogTemp, Display, TEXT("ReleaseHeal_Implementation"));
	bIsHealing = false;
	PlayHealFinishVFX();
}

bool ARCCharacter::SwapEquippable_Implementation(EEquippable ToEquip, bool IsForced)
{
	if (CurrentEquippable == ToEquip) { return true; }

	if (!IsForced)
	{
		float CurrentTime = GetWorld()->GetTimeSeconds();
		// The player is trying to swap too fast.
		if (!(LastEquipTime + EquipDelay < CurrentTime))
		{
			return false;
		}
		LastEquipTime = CurrentTime;
	}

	// Save current weapon as LastEquippable ONLY if we're switching to shield or none
	if ((ToEquip == EEquippable::EE_Shield || ToEquip == EEquippable::EE_None) &&
		(CurrentEquippable == EEquippable::EE_Melee || CurrentEquippable == EEquippable::EE_Ranged))
	{
		// Only save it if we're not already saving the same value
		if (LastEquippable != CurrentEquippable)
		{
			LastEquippable = CurrentEquippable;
		}
	}

	// If the player interrupted the charge shot with shield we want to fail the charge shot.
	if (CurrentEquippable == EEquippable::EE_Ranged && ToEquip == EEquippable::EE_Shield)
	{
		PlayReleaseRangedFailureVFX();
	}

	switch (ToEquip)
	{
	case EEquippable::EE_Ranged:
		if (RangedEquippable) RangedEquippable->SetVisibility(true);
		else SetupEquippable(ToEquip);

		if (MeleeEquippable) MeleeEquippable->SetVisibility(false);
		if (ShieldEquippableObject) ShieldEquippableObject->SetVisibility(false);

		break;

	case EEquippable::EE_Melee:
		if (MeleeEquippable) MeleeEquippable->SetVisibility(true);
		else SetupEquippable(ToEquip);

		if (RangedEquippable) RangedEquippable->SetVisibility(false);
		if (ShieldEquippableObject) ShieldEquippableObject->SetVisibility(false);

		break;

	case EEquippable::EE_Shield:
		if (ShieldEquippableObject) ShieldEquippableObject->SetVisibility(true);
		else SetupEquippable(ToEquip);

		if (RangedEquippable) RangedEquippable->SetVisibility(false);
		if (MeleeEquippable) MeleeEquippable->SetVisibility(false);
		break;

	case EEquippable::EE_None:
		if (RangedEquippable) RangedEquippable->SetVisibility(false);
		if (MeleeEquippable) MeleeEquippable->SetVisibility(false);
		if (ShieldEquippableObject) ShieldEquippableObject->SetVisibility(false);
		break;

	default:
		break;
	}

	CurrentEquippable = ToEquip;
	return true;
}

bool ARCCharacter::CanCrouch() const
{
	if (RCCharacterMovementComponent->IsFalling()) { return false; }
	if (RCCharacterMovementComponent->IsWallsliding) { return false; }
	if (RCCharacterMovementComponent->IsDashing) { return false; }

	return Super::CanCrouch();
}

void ARCCharacter::StopJumping()
{
	IsJumpStale = false;
	Super::StopJumping();
}

void ARCCharacter::ResetJumpState()
{
	// Override default class to allow jumping to reset with wallsliding properly. 
	bPressedJump = false;
	JumpKeyHoldTime = 0.0f;
	JumpForceTimeRemaining = 0.0f;
}

void ARCCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	JumpCurrentCount = 0;
	JumpCurrentCountPreJump = 0;
}

void ARCCharacter::CheckJumpInput(float DeltaTime)
{
	// Override default class to make it so the player needs to press jump multiple times for multiple jumps.
	JumpCurrentCountPreJump = JumpCurrentCount;

	if (RCCharacterMovementComponent)
	{
		if (bPressedJump && (IsJumpProvidingForce() || !IsJumpStale))
		{
			IsJumpStale = true;

			if (GetWorld()->GetTimeSeconds() - RCCharacterMovementComponent->GetLastDropThroughPlatformTime() >
				DropThroughPlatformJumpLockout)
			{
				if (bInCoyoteTime && RCCharacterMovementComponent->IsFalling())
				{
					if (JumpCoyoteTimerHandle.IsValid())
					{
						// Clear the timer, which stops it from executing
						GetWorld()->GetTimerManager().ClearTimer(JumpCoyoteTimerHandle);

						// Invalidate the handle after clearing the timer
						JumpCoyoteTimerHandle.Invalidate();
					}

					// Set the current movement mode to Walking to trigger a JumpCurrentCount0 jump.
					RCCharacterMovementComponent->SetMovementMode(MOVE_Walking);
				}

				const bool bDidJump = RCCharacterMovementComponent->DoJump(bClientUpdating);
				if (bDidJump)
				{
					// Transition from not (actively) jumping to jumping.
					if (!bWasJumping)
					{
						JumpCurrentCount++;
						JumpForceTimeRemaining = GetJumpMaxHoldTime();
						bInCoyoteTime = false;
						OnJumped();
					}
				}

				bWasJumping = bDidJump;
			}
		}
	}
}

void ARCCharacter::OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode)
{
	// When the movement mode changes we want to see if the player simply ran off the ledge in-case they may be trying to jump off, coyote style.
	Super::OnMovementModeChanged(PrevMovementMode, PreviousCustomMode);

	if (PrevMovementMode == MOVE_Walking &&
		RCCharacterMovementComponent->MovementMode == MOVE_Falling &&
		!bWasJumping)
	{
		bInCoyoteTime = true;

		// Capture 'this' as a weak pointer to avoid a strong reference cycle.
		TWeakObjectPtr<ARCCharacter> WeakThis(this);

		// Use a lambda to define the timer's logic.
		FTimerDelegate TimerDelegate;
		TimerDelegate.BindLambda([WeakThis]()
		{
			// Check if the object is still valid before accessing it.
			if (WeakThis.IsValid())
			{
				if (WeakThis->RCCharacterMovementComponent->IsFalling() && WeakThis->JumpCurrentCount <= 0)
				{
					WeakThis->JumpCurrentCount++;
					WeakThis->bInCoyoteTime = false;
				}
			}
		});

		// Set the timer using the FTimerDelegate.
		GetWorld()->GetTimerManager().SetTimer(
			JumpCoyoteTimerHandle,
			TimerDelegate,
			JumpCoyoteTime,
			false
		);
	}
}

FCollisionQueryParams ARCCharacter::GetIgnoreSelfParams() const
{
	FCollisionQueryParams Params;
	TArray<AActor*> SelfChildren;

	GetAllChildActors(SelfChildren);
	Params.AddIgnoredActors(SelfChildren);
	Params.AddIgnoredActor(this);

	return Params;
}
