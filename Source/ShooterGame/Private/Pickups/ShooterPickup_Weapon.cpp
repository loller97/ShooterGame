// Fill out your copyright notice in the Description page of Project Settings.

#include "ShooterGame.h"
#include "Pickups/ShooterPickup_Weapon.h"
#include "Weapons/ShooterWeapon.h"
#include "Containers/Array.h"
#include "OnlineSubsystemUtils.h"

AShooterPickup_Weapon::AShooterPickup_Weapon(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	
}

void AShooterPickup_Weapon::BeginPlay()
{
	Super::BeginPlay();
	// register on pickup list (server only), don't care about unregistering (in FinishDestroy) - no streaming
	AShooterGameMode* GameMode = GetWorld()->GetAuthGameMode<AShooterGameMode>();
	if (GameMode)
	{
		GameMode->LevelPickups.Add(this);
	}
}

void AShooterPickup_Weapon::SetAmmo(int32 ammo)
{
	Ammo = ammo;
}

void AShooterPickup_Weapon::SetIsRespawnable(bool Respawn)
{
	IsRespawnable = Respawn;
}

bool AShooterPickup_Weapon::IsForWeapon(UClass* WeaponClass)
{
	return WeaponType->IsChildOf(WeaponClass);
}

bool AShooterPickup_Weapon::CanBePickedUp(AShooterCharacter* TestPawn) const
{
	AShooterWeapon* TestWeapon = (TestPawn ? TestPawn->FindWeapon(WeaponType) : NULL);
	if (bIsActive && TestWeapon)
	{
		return TestWeapon->GetCurrentAmmo() < TestWeapon->GetMaxAmmo();
	}

	return false;
}

bool AShooterPickup_Weapon::IsNewWeapon(AShooterCharacter* TestPawn) const
{
	AShooterWeapon* TestWeapon = (TestPawn ? TestPawn->FindWeapon(WeaponType) : NULL);
	if (TestWeapon)
		return false;
	return true;
}

void AShooterPickup_Weapon::GivePickupTo(class AShooterCharacter* Pawn)
{
	AShooterWeapon* Weapon = (Pawn ? Pawn->FindWeapon(WeaponType) : NULL);
	if (Weapon)
	{
		Weapon->GiveAmmo(Ammo);

		// Fire event for collected ammo
		if (Pawn)
		{
			const UWorld* World = GetWorld();
			const IOnlineEventsPtr Events = Online::GetEventsInterface(World);
			const IOnlineIdentityPtr Identity = Online::GetIdentityInterface(World);

			if (Events.IsValid() && Identity.IsValid())
			{
				AShooterPlayerController* PC = Cast<AShooterPlayerController>(Pawn->Controller);
				if (PC)
				{
					ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(PC->Player);

					if (LocalPlayer)
					{
						const int32 UserIndex = LocalPlayer->GetControllerId();
						TSharedPtr<const FUniqueNetId> UniqueID = Identity->GetUniquePlayerId(UserIndex);
						if (UniqueID.IsValid())
						{
							FVector Location = Pawn->GetActorLocation();

							FOnlineEventParms Params;

							Params.Add(TEXT("SectionId"), FVariantData((int32)0)); // unused
							Params.Add(TEXT("GameplayModeId"), FVariantData((int32)1)); // @todo determine game mode (ffa v tdm)
							Params.Add(TEXT("DifficultyLevelId"), FVariantData((int32)0)); // unused

							Params.Add(TEXT("ItemId"), FVariantData((int32)Weapon->GetAmmoType() + 1)); // @todo come up with a better way to determine item id, currently health is 0 and ammo counts from 1
							Params.Add(TEXT("AcquisitionMethodId"), FVariantData((int32)0)); // unused
							Params.Add(TEXT("LocationX"), FVariantData(Location.X));
							Params.Add(TEXT("LocationY"), FVariantData(Location.Y));
							Params.Add(TEXT("LocationZ"), FVariantData(Location.Z));
							Params.Add(TEXT("ItemQty"), FVariantData((int32)Ammo));

							Events->TriggerEvent(*UniqueID, TEXT("CollectPowerup"), Params);
						}
					}
				}
			}
		}
	}
	else
	{
		FActorSpawnParameters SpawnInfo;
		SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AShooterWeapon* NewWeapon = GetWorld()->SpawnActor<AShooterWeapon>(WeaponType, SpawnInfo);
		if (NewWeapon)
		{
			NewWeapon->SetAmmo(Ammo);
			Pawn->AddWeapon(NewWeapon);
			if (Pawn)
			{
				const UWorld* World = GetWorld();
				const IOnlineEventsPtr Events = Online::GetEventsInterface(World);
				const IOnlineIdentityPtr Identity = Online::GetIdentityInterface(World);

				if (Events.IsValid() && Identity.IsValid())
				{
					AShooterPlayerController* PC = Cast<AShooterPlayerController>(Pawn->Controller);
					if (PC)
					{
						ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(PC->Player);

						if (LocalPlayer)
						{
							const int32 UserIndex = LocalPlayer->GetControllerId();
							TSharedPtr<const FUniqueNetId> UniqueID = Identity->GetUniquePlayerId(UserIndex);
							if (UniqueID.IsValid())
							{
								FVector Location = Pawn->GetActorLocation();

								FOnlineEventParms Params;

								Params.Add(TEXT("SectionId"), FVariantData((int32)0)); // unused
								Params.Add(TEXT("GameplayModeId"), FVariantData((int32)1)); // @todo determine game mode (ffa v tdm)
								Params.Add(TEXT("DifficultyLevelId"), FVariantData((int32)0)); // unused

								Params.Add(TEXT("ItemId"), FVariantData((int32)Weapon->GetAmmoType() + 1 + Weapon->GetHowManyAmmoTypes())); // @todo come up with a better way to determine item id, currently health is 0 and ammo counts from 1
								Params.Add(TEXT("AcquisitionMethodId"), FVariantData((int32)0)); // unused
								Params.Add(TEXT("LocationX"), FVariantData(Location.X));
								Params.Add(TEXT("LocationY"), FVariantData(Location.Y));
								Params.Add(TEXT("LocationZ"), FVariantData(Location.Z));

								Events->TriggerEvent(*UniqueID, TEXT("CollectPowerup"), Params);
							}
						}
					}
				}
			}
		}
	}
}

void AShooterPickup_Weapon::PickupOnTouch(AShooterCharacter* Pawn)
{
	if (bIsActive && Pawn && Pawn->IsAlive() && !IsPendingKill())
	{
		if (!IsNewWeapon(Pawn))			//Check if the pickup is a new weapon or one already in inventory
		{
			if (CanBePickedUp(Pawn))
			{
				GivePickupTo(Pawn);
				PickedUpBy = Pawn;

				if (!IsPendingKill())
				{
					if (IsRespawnable)
					{
						bIsActive = false;
						OnPickedUp();

						if (RespawnTime > 0.0f)
						{
							GetWorldTimerManager().SetTimer(TimerHandle_RespawnPickup, this, &AShooterPickup_Weapon::RespawnPickup, RespawnTime, false);
						}
					}
					else
					{
						OnPickedUp();
						Destroy();
					}
				}
			}
		}
		else
		{
			GivePickupTo(Pawn);
			PickedUpBy = Pawn;
			if (!IsPendingKill())
			{
				if (IsRespawnable)
				{
					bIsActive = false;
					OnPickedUp();

					if (RespawnTime > 0.0f)
					{
						GetWorldTimerManager().SetTimer(TimerHandle_RespawnPickup, this, &AShooterPickup_Weapon::RespawnPickup, RespawnTime, false);
					}
				}
				else
				{
					OnPickedUp();
					Destroy();
				}
			}
		}
	}
}
