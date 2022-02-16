// Copyright Epic Games, Inc. All Rights Reserved.

/**
 * Movement component meant for use with Pawns.
 */

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ShooterCharacterMovement.generated.h"

#define CHARACTERNETWORKING_API DLLEXPORT

UCLASS(BlueprintType)
class CHARACTERNETWORKING_API UShooterCharacterMovement : public UCharacterMovementComponent
{
	GENERATED_BODY()

	friend class FSavedMove_ShooterCharacter;

#pragma region Defaults
private:
	// The ground speed when running
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "My Character Movement|Grounded", Meta = (AllowPrivateAccess = "true"))
		float RunSpeed = 300.0f;
	// The ground speed when sprinting
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "My Character Movement|Grounded", Meta = (AllowPrivateAccess = "true"))
		float SprintSpeed = 800.0f;
	// The acceleration when running
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "My Character Movement|Grounded", Meta = (AllowPrivateAccess = "true"))
		float RunAcceleration = 2000.0f;
	// The acceleration when sprinting
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "My Character Movement|Grounded", Meta = (AllowPrivateAccess = "true"))
		float SprintAcceleration = 2000.0f;
	// The amount of vertical room between the two line traces when checking if the character is still on the wall
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "My Character Movement|Wall Running", Meta = (AllowPrivateAccess = "true"))
		float LineTraceVerticalTolerance = 50.0f;
	// The player's velocity while wall running
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "My Character Movement|Wall Running", Meta = (AllowPrivateAccess = "true"))
		float WallRunSpeed = 625.0f;
#pragma endregion

public:
	// Sets sprinting to either enabled or disabled
	UFUNCTION(BlueprintCallable, Category = "Character Movement")
		void SetSprinting(bool sprinting, bool sprintingToggle);

#pragma region Overrides

protected:
	virtual void BeginPlay() override;
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;

public:
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	//update the client flags from the compressed custom ones
	virtual void UpdateFromCompressedFlags(uint8 Flags) override;
	//Handle the MovementMode change
	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode) override;
	//Set custom physics movement, useful to make them correcly applied on the network
	virtual void PhysCustom(float deltaTime, int32 Iterations) override;
	//Physic movement for the Wallrun
	void PhysWallRunning(float deltaTime, int32 Iterations);
	//Get the max speed depending on the current movement
	virtual float GetMaxSpeed() const override;
	//Get the max acceleration depending on the current movement
	virtual float GetMaxAcceleration() const override;
	//Handle the land
	virtual void ProcessLanded(const FHitResult& Hit, float remainingTime, int32 Iterations) override;
	//Function to handle the client prediction
	virtual FNetworkPredictionData_Client* GetPredictionData_Client() const override;

#pragma endregion

#pragma region Compressed Flags
private:
	uint8 WantsToSprint : 1;
	uint8 WallRunKeysDown : 1;
#pragma endregion

#pragma region Private Variables
	// True if the sprint key is down
	bool SprintKeyDown = false;
	bool SprintKeyDownToggle = false;
	// The direction the character is currently wall running in
	FVector WallRunDirection;
	// The side of the wall the player is running on.
	EWallRunSide WallRunSide;
#pragma endregion

#pragma region WallRun
public:
	// Requests that the character begins wall running. Will return false if the required keys are not being pressed
	UFUNCTION(BlueprintCallable, Category = "Custom Character Movement")
		bool BeginWallRun();
	// Ends the character's wall run
	UFUNCTION(BlueprintCallable, Category = "Custom Character Movement")
		void EndWallRun();
	// Returns true if the required wall run keys are currently down
	bool AreRequiredWallRunKeysDown() const;
	// Returns true if the player is next to a wall that can be wall ran
	bool IsNextToWall(float vertical_tolerance = 0.0f);
	// Finds the wall run direction and side based on the specified surface normal
	void FindWallRunDirectionAndSide(const FVector& surface_normal, FVector& direction, EWallRunSide& side) const;
	// Helper function that returns true if the specified surface normal can be wall ran on
	bool CanSurfaceBeWallRan(const FVector& surface_normal) const;
	// Returns true if the movement mode is custom and matches the provided custom movement mode
	bool IsCustomMovementMode(uint8 custom_movement_mode) const;

private:
	// Called when the owning actor hits something (to begin the wall run)
	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
#pragma endregion
};

class FNetworkPredictionData_Client_ShooterCharacter : public FNetworkPredictionData_Client_Character
{
public:
	typedef FNetworkPredictionData_Client_Character Super;

	// Constructor
	FNetworkPredictionData_Client_ShooterCharacter(const UCharacterMovementComponent& ClientMovement);

	//brief Allocates a new copy of our custom saved move
	virtual FSavedMovePtr AllocateNewMove() override;
};

class FSavedMove_ShooterCharacter : public FSavedMove_Character
{
public:

	typedef FSavedMove_Character Super;

	// Resets all saved variables.
	virtual void Clear() override;
	// Store input commands in the compressed flags.
	virtual uint8 GetCompressedFlags() const override;
	// This is used to check whether or not two moves can be combined into one.
	// Basically you just check to make sure that the saved variables are the same.
	virtual bool CanCombineWith(const FSavedMovePtr& NewMovePtr, ACharacter* Character, float MaxDelta) const override;
	// Sets up the move before sending it to the server. 
	virtual void SetMoveFor(ACharacter* Character, float InDeltaTime, FVector const& NewAccel, class FNetworkPredictionData_Client_Character& ClientData) override;
	// Sets variables on character movement component before making a predictive correction.
	virtual void PrepMoveFor(class ACharacter* Character) override;

private:
	uint8 SavedWantsToSprint : 1;
	uint8 SavedWallRunKeysDown : 1;
};

/** Custom movement modes for Characters. */
UENUM(BlueprintType)
enum ECustomMovementMode
{
	CMOVE_WallRunning   UMETA(DisplayName = "WallRunning"),
	CMOVE_MAX			UMETA(Hidden),
};

//Enum for the direction of the wall the character want to wallrun
UENUM(BlueprintType)
enum class EWallRunSide : uint8 {
	left UMETA(DisplayName = "LEFT"),
	right UMETA(DisplayName = "RIGHT"),
};

//Enum for the reason the character end the wallrun, I'll see to use them When reimplementing the walljump
UENUM(BlueprintType)
enum class EWallRunEndReason : uint8 {
	fallOff UMETA(DisplayName = "FALL OFF WALL"),
	jumpedOff UMETA(DisplayName = "JUMPED OFF WALL"),
};