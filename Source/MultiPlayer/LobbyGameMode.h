// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MultiPlayerGameMode.h"
#include "LobbyGameMode.generated.h"

/**
 * 
 */
UCLASS()
class MULTIPLAYER_API ALobbyGameMode : public AMultiPlayerGameMode
{
	GENERATED_BODY()
	
public:

	virtual void PostLogin(APlayerController* NewPlayer) override;

	virtual void Logout(AController* Exiting) override;

private:
	uint32 NumberOfPlayers = 0;

	void StartGame();

	FTimerHandle GameStartTimer;
};
