// Copyright Epic Games, Inc. All Rights Reserved.

#include "ShooterGame.h"
#include "Weapons/ShooterProjectile.h"
#include "Particles/ParticleSystemComponent.h"
#include "Effects/ShooterExplosionEffect.h"

AShooterProjectile::AShooterProjectile(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	CollisionComp = ObjectInitializer.CreateDefaultSubobject<USphereComponent>(this, TEXT("SphereComp"));
	CollisionComp->InitSphereRadius(5.0f);
	CollisionComp->AlwaysLoadOnClient = true;
	CollisionComp->AlwaysLoadOnServer = true;
	CollisionComp->bTraceComplexOnMove = true;
	CollisionComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionComp->SetCollisionObjectType(COLLISION_PROJECTILE);
	CollisionComp->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionComp->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	CollisionComp->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	CollisionComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	RootComponent = CollisionComp;

	ParticleComp = ObjectInitializer.CreateDefaultSubobject<UParticleSystemComponent>(this, TEXT("ParticleComp"));
	ParticleComp->bAutoActivate = false;
	ParticleComp->bAutoDestroy = false;
	ParticleComp->SetupAttachment(RootComponent);

	Mesh = ObjectInitializer.CreateDefaultSubobject<UStaticMeshComponent>(this, TEXT("MeshComp"));
	Mesh->SetupAttachment(RootComponent);

	MovementComp = ObjectInitializer.CreateDefaultSubobject<UProjectileMovementComponent>(this, TEXT("ProjectileComp"));
	MovementComp->UpdatedComponent = CollisionComp;
	MovementComp->InitialSpeed = 2000.0f;
	MovementComp->MaxSpeed = 2000.0f;
	MovementComp->bRotationFollowsVelocity = true;
	MovementComp->ProjectileGravityScale = 0.f;

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;
	SetRemoteRoleForBackwardsCompat(ROLE_SimulatedProxy);
	bReplicates = true;
	SetReplicatingMovement(true);
}

void AShooterProjectile::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	CollisionComp->MoveIgnoreActors.Add(GetInstigator());

	CollisionComp->OnComponentHit.AddDynamic(this, &AShooterProjectile::OnHit);


	if (FreezeProjectile)
	{
		AShooterWeapon_SnowBall* OwnerWeapon = Cast<AShooterWeapon_SnowBall>(GetOwner());
		if (OwnerWeapon)
		{
			OwnerWeapon->ApplyWeaponConfig(WeaponConfigS);
		}
		SetLifeSpan(WeaponConfigS.ProjectileLife);
	}
	else
	{
		AShooterWeapon_Projectile* OwnerWeapon = Cast<AShooterWeapon_Projectile>(GetOwner());
		if (OwnerWeapon)
		{
			OwnerWeapon->ApplyWeaponConfig(WeaponConfig);
		}
		SetLifeSpan(WeaponConfig.ProjectileLife);
	}



	MyController = GetInstigatorController();
}

void AShooterProjectile::InitVelocity(FVector& ShootDirection)
{
	if (MovementComp)
	{
		MovementComp->Velocity = ShootDirection * MovementComp->InitialSpeed;
	}
}


void AShooterProjectile::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& HitResult)
{
	if (GetLocalRole() == ROLE_Authority && !bExploded)
	{
		Explode(HitResult);
		DisableAndDestroy();
	}
}

void AShooterProjectile::Explode(const FHitResult& Impact)
{
	if (ParticleComp)
	{
		ParticleComp->Deactivate();
	}

	// effects and damage origin shouldn't be placed inside mesh at impact point
	const FVector NudgedImpactLocation = Impact.ImpactPoint + Impact.ImpactNormal * 10.0f;

	if (FreezeProjectile)
	{
		if (WeaponConfigS.ExplosionDamage > 0 && WeaponConfigS.ExplosionRadius > 0 && WeaponConfigS.DamageType)
		{
			UGameplayStatics::ApplyRadialDamage(this, WeaponConfigS.ExplosionDamage, NudgedImpactLocation, WeaponConfigS.ExplosionRadius, WeaponConfigS.DamageType, TArray<AActor*>(), this, MyController.Get());
		}

		FreezeActors(this, NudgedImpactLocation, WeaponConfigS.ExplosionRadius, TArray<AActor*>(), this, MyController.Get());

	}
	else
	{
		if (WeaponConfig.ExplosionDamage > 0 && WeaponConfig.ExplosionRadius > 0 && WeaponConfig.DamageType)
		{
			UGameplayStatics::ApplyRadialDamage(this, WeaponConfig.ExplosionDamage, NudgedImpactLocation, WeaponConfig.ExplosionRadius, WeaponConfig.DamageType, TArray<AActor*>(), this, MyController.Get());
		}
	}


	if (ExplosionTemplate)
	{
		FTransform const SpawnTransform(Impact.ImpactNormal.Rotation(), NudgedImpactLocation);
		AShooterExplosionEffect* const EffectActor = GetWorld()->SpawnActorDeferred<AShooterExplosionEffect>(ExplosionTemplate, SpawnTransform);
		if (EffectActor)
		{
			EffectActor->SurfaceHit = Impact;
			UGameplayStatics::FinishSpawningActor(EffectActor, SpawnTransform);
		}
	}

	bExploded = true;
}

void AShooterProjectile::FreezeActors(const UObject* WorldContextObject, const FVector& Origin, float DamageRadius, const TArray<AActor*>& IgnoreActors, AActor* DamageCauser, AController* InstigatedByController)
{
	FCollisionQueryParams SphereParams(SCENE_QUERY_STAT(ApplyRadialDamage), false, DamageCauser);

	SphereParams.AddIgnoredActors(IgnoreActors);

	// query scene to see what we hit
	TArray<FOverlapResult> Overlaps;
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		World->OverlapMultiByObjectType(Overlaps, Origin, FQuat::Identity, FCollisionObjectQueryParams(FCollisionObjectQueryParams::InitType::AllDynamicObjects), FCollisionShape::MakeSphere(DamageRadius), SphereParams);
	}

	// collate into per-actor list of hit components
	TMap<AActor*, TArray<FHitResult> > OverlapComponentMap;
	for (int32 Idx = 0; Idx < Overlaps.Num(); ++Idx)
	{
		FOverlapResult const& Overlap = Overlaps[Idx];
		AActor* const OverlapActor = Overlap.GetActor();

		if (OverlapActor &&
			OverlapActor->CanBeDamaged() &&
			OverlapActor != DamageCauser &&
			Overlap.Component.IsValid())
		{
			FHitResult Hit;

			TArray<FHitResult>& HitList = OverlapComponentMap.FindOrAdd(OverlapActor);
			HitList.Add(Hit);
		}
	}

	if (OverlapComponentMap.Num() > 0)
	{

		// call damage function on each affected actors
		for (TMap<AActor*, TArray<FHitResult> >::TIterator It(OverlapComponentMap); It; ++It)
		{
			AActor* const Victim = It.Key();
			TArray<FHitResult> const& ComponentHits = It.Value();
			AShooterCharacter* VictimShooter = Cast<AShooterCharacter>(Victim);
			if (VictimShooter)
			{
				VictimShooter->Freezing();
			}

		}
	}
}

void AShooterProjectile::DisableAndDestroy()
{
	UAudioComponent* ProjAudioComp = FindComponentByClass<UAudioComponent>();
	if (ProjAudioComp && ProjAudioComp->IsPlaying())
	{
		ProjAudioComp->FadeOut(0.1f, 0.f);
	}

	MovementComp->StopMovementImmediately();

	// give clients some time to show explosion
	Mesh->SetActive(false);
	SetLifeSpan(2.0f);
}

///CODE_SNIPPET_START: AActor::GetActorLocation AActor::GetActorRotation
void AShooterProjectile::OnRep_Exploded()
{
	FVector ProjDirection = GetActorForwardVector();

	const FVector StartTrace = GetActorLocation() - ProjDirection * 200;
	const FVector EndTrace = GetActorLocation() + ProjDirection * 150;
	FHitResult Impact;

	if (!GetWorld()->LineTraceSingleByChannel(Impact, StartTrace, EndTrace, COLLISION_PROJECTILE, FCollisionQueryParams(SCENE_QUERY_STAT(ProjClient), true, GetInstigator())))
	{
		// failsafe
		Impact.ImpactPoint = GetActorLocation();
		Impact.ImpactNormal = -ProjDirection;
	}

	Explode(Impact);
}
///CODE_SNIPPET_END

void AShooterProjectile::PostNetReceiveVelocity(const FVector& NewVelocity)
{
	if (MovementComp)
	{
		MovementComp->Velocity = NewVelocity;
	}
}

void AShooterProjectile::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AShooterProjectile, bExploded);
}