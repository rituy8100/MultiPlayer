// Fill out your copyright notice in the Description page of Project Settings.


#include "MovingPlatform.h"

AMovingPlatform::AMovingPlatform() {
	PrimaryActorTick.bCanEverTick = true;

	SetMobility(EComponentMobility::Movable);
}

void AMovingPlatform::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority()) {
		SetReplicates(true);
		SetReplicateMovement(true);
	}

	GlobalStartLocation = GetActorLocation();
	//local vector을 global vector로 변환
	GlobalTargetLocation= GetTransform().TransformPosition(TargetLocation);
}

void AMovingPlatform::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (ActiveTriggers > 0) {
		if (HasAuthority()) {
			FVector Location = GetActorLocation();
			//TargetLocation까지의 거리
			float JournyLength = (GlobalTargetLocation - GlobalStartLocation).Size();
			//현재 이동한 거리
			float JourneyTraveled = (Location - GlobalStartLocation).Size();

			//TargetLocation까지 도착했을경우 swap
			if (JournyLength <= JourneyTraveled) {
				FVector Swap = GlobalStartLocation;
				GlobalStartLocation = GlobalTargetLocation;
				GlobalTargetLocation = Swap;
			}
			//방향vector 정규화
			FVector Direction = (GlobalTargetLocation - GlobalStartLocation).GetSafeNormal();
			//targetLocation으로 이동
			Location += Speed * DeltaTime * Direction;

			SetActorLocation(Location);
		}
	}
}

void AMovingPlatform::AddActiveTrigger()
{
	ActiveTriggers++;
}

void AMovingPlatform::RemoveActiveTrigger()
{
	if (ActiveTriggers > 0)
	{
		ActiveTriggers--;
	}
}