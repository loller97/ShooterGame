// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pickups/ShooterPickup.h"
#include "Weapons/ShooterWeapon.h"
#include "Weapons/ShooterWeapon_Instant.h"
#include "Weapons/ShooterWeapon_Projectile.h"
#include "ShooterPickup_Weapon.generated.h"

class AShooterCharacter;
class AShooterWeapon;

UCLASS(Abstract, Blueprintable)
class AShooterPickup_Weapon : public AShooterPickup
{
	GENERATED_UCLASS_BODY()
	
	virtual bool CanBePickedUp(AShooterCharacter* TestPawn) const override;
	bool IsNewWeapon(AShooterCharacter* TestPawn) const;

	bool IsForWeapon(UClass* WeaponClass);

protected:

	virtual void BeginPlay() override;

	/** how much ammo does it give? */
	UPROPERTY(EditAnywhere, Category = Pickup)
		int Ammo = 100;

	//sets if the pickup is respawnable
	UPROPERTY(EditDefaultsOnly, Category = Pickup)
		bool IsRespawnable;

	/** which weapon gets ammo? */
	UPROPERTY(EditDefaultsOnly, Category = Pickup)
		TSubclassOf<AShooterWeapon> WeaponType;

	/** give pickup */
	virtual void GivePickupTo(AShooterCharacter* Pawn) override;



	/** handle touches */
	virtual void PickupOnTouch(class AShooterCharacter* Pawn) override;

public:
	//sets the ammo you get from this pickup
	void SetAmmo(int32 ammo);

	void SetIsRespawnable(bool Respawn);

};


