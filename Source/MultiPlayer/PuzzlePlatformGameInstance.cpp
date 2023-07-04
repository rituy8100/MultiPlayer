// Fill out your copyright notice in the Description page of Project Settings.


#include "PuzzlePlatformGameInstance.h"
#include "Engine/Engine.h"
#include "UObject/ConstructorHelpers.h"
#include "Blueprint/UserWidget.h"
#include "MainMenu.h"
#include "InGameMenu.h"
#include "MenuWidget.h"
#include "OnlineSessionSettings.h"

#include "GameFramework/PlayerController.h"

const static FName SESSION_NAME = TEXT("My Session Games");
const static FName SERVER_NAME_SETTINGS_KEY = TEXT("ServerName");

UPuzzlePlatformGameInstance::UPuzzlePlatformGameInstance(const FObjectInitializer& ObjectInitializer)
{
	UE_LOG(LogTemp, Warning, TEXT("GameInstance Constructor"));
	ConstructorHelpers::FClassFinder<UUserWidget> MenuBPClass(TEXT("/Game/ThirdPerson/Blueprints/UI/WBP_MainMenu"));
	if (!ensure(MenuBPClass.Class != nullptr)) return;

	MenuClass = MenuBPClass.Class;

	ConstructorHelpers::FClassFinder<UUserWidget> InGameMenuBPClass(TEXT("/Game/ThirdPerson/Blueprints/UI/WBP_InGameMenu1"));
	if (!ensure(InGameMenuBPClass.Class != nullptr)) return;

	InGameMenuClass = InGameMenuBPClass.Class;
}

void UPuzzlePlatformGameInstance::Init()
{
	Super::Init();
	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
	if (Subsystem != nullptr) {
		UE_LOG(LogTemp, Warning, TEXT("Found Subsystem %s"),*Subsystem->GetSubsystemName().ToString());
		SessionInterface = Subsystem->GetSessionInterface();
		if (SessionInterface.IsValid()) {
			//binding
			SessionInterface->OnCreateSessionCompleteDelegates.AddUObject(this, &UPuzzlePlatformGameInstance::OnCreateSessionComplete);
			SessionInterface->OnDestroySessionCompleteDelegates.AddUObject(this, &UPuzzlePlatformGameInstance::OnDestroySessionComplete);
			//find session
			SessionInterface->OnFindSessionsCompleteDelegates.AddUObject(this, &UPuzzlePlatformGameInstance::OnFindSessionsComplete);
			//join session complete
			SessionInterface->OnJoinSessionCompleteDelegates.AddUObject(this, &UPuzzlePlatformGameInstance::OnJoinSessionComplete);
		}
	}
	else {
		UE_LOG(LogTemp, Warning, TEXT("Found no Subsystem"));
	}
	
	UE_LOG(LogTemp, Warning, TEXT("GameInstance Init"));
	UE_LOG(LogTemp, Warning, TEXT("Found class %s"), *MenuClass->GetName());
}

void UPuzzlePlatformGameInstance::RefreshServerList()
{
	//TShareRef to find session
	SessionSearch = MakeShareable(new FOnlineSessionSearch());
	if (SessionSearch.IsValid())
	{
		//SessionSearch->bIsLanQuery = true;
		SessionSearch->MaxSearchResults = 100;
		SessionSearch->QuerySettings.Set(SEARCH_PRESENCE, true, EOnlineComparisonOp::Equals);
		UE_LOG(LogTemp, Warning, TEXT("Starting Find Session"));
		SessionInterface->FindSessions(0, SessionSearch.ToSharedRef());
	}
}

void UPuzzlePlatformGameInstance::Host(FString ServerName)
{
	DesiredServerName = ServerName;
	if (SessionInterface.IsValid())
	{
		auto ExistingSession = SessionInterface->GetNamedSession(SESSION_NAME);
		if (ExistingSession != nullptr)
		{
			SessionInterface->DestroySession(SESSION_NAME);
		}
		else
		{
			CreateSession();
		}
	}
}

void UPuzzlePlatformGameInstance::OnDestroySessionComplete(FName SessionName, bool Success)
{
	if (Success) {
		CreateSession();
	}
}

void UPuzzlePlatformGameInstance::CreateSession()
{
	if (SessionInterface.IsValid()) {
		FOnlineSessionSettings SessionSettings;
		if (IOnlineSubsystem::Get()->GetSubsystemName() == "NULL")
		{
			SessionSettings.bIsLANMatch = true;
		}
		else
		{
			SessionSettings.bIsLANMatch = false;
		}
		SessionSettings.NumPublicConnections = 5;
		SessionSettings.bShouldAdvertise = true;
		SessionSettings.bUsesPresence = true;
		SessionSettings.Set(SERVER_NAME_SETTINGS_KEY, DesiredServerName, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

		SessionInterface->CreateSession(0, SESSION_NAME, SessionSettings);
	}
}

void UPuzzlePlatformGameInstance::OnCreateSessionComplete(FName SessionName, bool Success)
{
	if (!Success)
	{
		UE_LOG(LogTemp, Warning, TEXT("Could not create session"));
		return;
	}

	if (Menu != nullptr)
	{
		Menu->Teardown();
	}
	UEngine* Engine = GetEngine();
	if (!ensure(Engine != nullptr)) return;

	Engine->AddOnScreenDebugMessage(0, 2, FColor::Green, TEXT("Hosting"));
	UWorld* World = GetWorld();
	if (!ensure(World != nullptr)) return;

	World->ServerTravel("/Game/ThirdPerson/Maps/Lobby?listen");
}

void UPuzzlePlatformGameInstance::Join(uint32 Index)
{
	if (!SessionInterface.IsValid()) return;

	if (Menu != nullptr)
	{
		Menu->Teardown();
	}

	SessionInterface->JoinSession(0, SESSION_NAME, SessionSearch->SearchResults[Index]);

	
}

void UPuzzlePlatformGameInstance::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	if (!SessionInterface.IsValid()) return;

	FString Address;
	if (!SessionInterface->GetResolvedConnectString(SessionName, Address)) {
		UE_LOG(LogTemp, Warning, TEXT("Could not get connect string"));
		return;
	}
	
	UEngine* Engine = GetEngine();
	if (!ensure(Engine != nullptr)) return;

	Engine->AddOnScreenDebugMessage(0, 5, FColor::Green, FString::Printf(TEXT("Joining %s"), *Address));
	APlayerController* PlayerController = GetFirstLocalPlayerController();
	if (!ensure(PlayerController != nullptr)) return;

	PlayerController->ClientTravel(Address, ETravelType::TRAVEL_Absolute);
}

void UPuzzlePlatformGameInstance::LoadMenuWidget()
{
	if (!ensure(MenuClass != nullptr)) return;

	//UUserWidget* Menu = CreateWidget(GetWorld(), MenuClass);
	Menu = CreateWidget<UMainMenu>(this, MenuClass);
	if (!ensure(Menu != nullptr)) return;
	UE_LOG(LogTemp, Warning, TEXT("Widget Init"));
	Menu->Setup();

	Menu->SetMenuInterface(this);
}

void UPuzzlePlatformGameInstance::InGameLoadMenu()
{
	if (!ensure(InGameMenuClass != nullptr)) return;

	UInGameMenu* InMenu = CreateWidget<UInGameMenu>(this, InGameMenuClass);
	if (!ensure(InMenu != nullptr)) return;

	InMenu->Setup();

	InMenu->SetMenuInterface(this);
}

void UPuzzlePlatformGameInstance::LoadMainMenu()
{
	APlayerController* PlayerController = GetFirstLocalPlayerController();
	if (!ensure(PlayerController != nullptr)) return;

	PlayerController->ClientTravel("/Game/ThirdPerson/Blueprints/UI/MainMenuLevel", ETravelType::TRAVEL_Absolute);
}

void UPuzzlePlatformGameInstance::OnFindSessionsComplete(bool Success)
{
	if (Success && SessionSearch.IsValid() && Menu != nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("Finished Find Session"));
		TArray<FServerData> ServerNames;
		for (const FOnlineSessionSearchResult& SearchResult : SessionSearch->SearchResults)
		{
			UE_LOG(LogTemp, Warning, TEXT("Found session names: %s"), *SearchResult.GetSessionIdStr());
			FServerData Data;
			Data.MaxPlayers = SearchResult.Session.SessionSettings.NumPublicConnections;
			Data.CurrentPlayers = Data.MaxPlayers - SearchResult.Session.NumOpenPublicConnections;
			Data.HostUsername = SearchResult.Session.OwningUserName;
			
			FString ServerName;
			if (SearchResult.Session.SessionSettings.Get(SERVER_NAME_SETTINGS_KEY, ServerName))
			{
				Data.Name = ServerName;
			}
			else
			{
				Data.Name = "Could not find name.";
			}

			ServerNames.Add(Data);
		}
		Menu->SetServerList(ServerNames);
	}
}

void UPuzzlePlatformGameInstance::StartSession()
{
	if (SessionInterface.IsValid())
	{
		SessionInterface->StartSession(SESSION_NAME);
	}
}