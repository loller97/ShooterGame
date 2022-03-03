// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapons/ShooterWeapon.h"
#include "GameFramework/DamageType.h" // for UDamageType::StaticClass()
#include "ShooterWeapon_SnowBall.generated.h"


USTRUCT()
struct FSnowBallWeaponData
{
	GENERATED_USTRUCT_BODY()

		/** projectile class */
		UPROPERTY(EditDefaultsOnly, Category = Projectile)
		TSubclassOf<class AShooterProjectile> ProjectileClass;

	/** life time */
	UPROPERTY(EditDefaultsOnly, Category = Projectile)
		float ProjectileLife;

	/** damage at impact point */
	UPROPERTY(EditDefaultsOnly, Category = WeaponStat)
		int32 ExplosionDamage;

	/** radius of damage */
	UPROPERTY(EditDefaultsOnly, Category = WeaponStat)
		float ExplosionRadius;

	/** type of damage */
	UPROPERTY(EditDefaultsOnly, Category = WeaponStat)
		TSubclassOf<UDamageType> DamageType;

	/** defaults */
	FSnowBallWeaponData()
	{
		ProjectileClass = NULL;
		ProjectileLife = 10.0f;
		ExplosionDamage = 10;
		ExplosionRadius = 200.0f;
		DamageType = UDamageType::StaticClass();
	}
};

UCLASS(Abstract)
class SHOOTERGAME_API AShooterWeapon_SnowBall : public AShooterWeapon
{
	GENERATED_UCLASS_BODY()

		/** apply config on projectile */
		void ApplyWeaponConfig(FSnowBallWeaponData& Data);

protected:

	virtual EAmmoType GetAmmoType() const override
	{
		return EAmmoType::ESnowBall;
	}

	/** weapon config */
	UPROPERTY(EditDefaultsOnly, Category = Config)
		FSnowBallWeaponData ProjectileConfig;

	//////////////////////////////////////////////////////////////////////////
	// Weapon usage

	/** [local] weapon specific fire implementation */
	virtual void FireWeapon() override;

	/** spawn projectile on server */
	UFUNCTION(reliable, server, WithValidation)
		void ServerFireProjectile(FVector Origin, FVector_NetQuantizeNormal ShootDir);
};
