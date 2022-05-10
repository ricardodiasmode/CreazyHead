// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "MainMenuGameMode.generated.h"

/**
 * 
 */
UCLASS()
class CRAZY_HEAD_DANCE_API AMainMenuGameMode : public AGameMode
{
	GENERATED_BODY()
	
private:
	TMap<FString, FString> UserAccount;
	TMap<FString, int> UserTimeLeft;
	TMap<FString, FString> ExpiredAccounts;

public:
	// Return true if password matches account
	UFUNCTION(BlueprintCallable)
		bool CheckUserAccount(const FString Account, const FString Password);
	UFUNCTION(BlueprintCallable)
		bool CheckAccountExist(const FString Account);
	UFUNCTION(BlueprintCallable)
		bool CheckAccountExpired(const FString Account);
	UFUNCTION(BlueprintCallable)
		void ReadDataFile();
	UFUNCTION(BlueprintCallable)
		int GetUserTimeLeft(const FString Account) { return UserTimeLeft[Account]; }
};
