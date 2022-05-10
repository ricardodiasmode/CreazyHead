// Fill out your copyright notice in the Description page of Project Settings.


#include "MainMenuGameMode.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Kismet/KismetStringLibrary.h"

bool AMainMenuGameMode::CheckUserAccount(const FString Account, const FString Password)
{
	if (UserAccount.Contains(Account))
	{
		if (UserAccount[Account] == Password)
			return true;
	}
	return false;
}

bool AMainMenuGameMode::CheckAccountExist(const FString Account)
{
	return UserAccount.Contains(Account);
}


bool AMainMenuGameMode::CheckAccountExpired(const FString Account)
{
	return ExpiredAccounts.Contains(Account);
}

void AMainMenuGameMode::ReadDataFile()
{
	UE_LOG(LogTemp, Warning, TEXT("---- Start to read data file ----"));
	UserAccount.Empty();

	// Loading DB
	FString StringAux;
	UE_LOG(LogTemp, Warning, TEXT("Opening file: %s"), *(FPaths::ProjectDir() + "Datafile.txt"));
	FFileHelper::LoadFileToString(StringAux, *(FPaths::ProjectDir() + "Datafile.txt"));
	UE_LOG(LogTemp, Warning, TEXT("Data file loaded"));

	UE_LOG(LogTemp, Warning, TEXT("Cleaning comments"));
	// Cleaning comments
	int FirstIndex = 0;
	int SecondIndex = 0;
	if (StringAux.Contains("*/**/**/**/*"))
	{
		FirstIndex = StringAux.Find("*/**/**/**/*");
		SecondIndex = StringAux.Find("*/**/**/**/*", ESearchCase::IgnoreCase, ESearchDir::FromStart, FirstIndex+13);
		StringAux.RemoveAt(FirstIndex, SecondIndex + 13 - FirstIndex, true);
	}

	// Debug
	GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, FString::Printf(TEXT("DB Readed: %s"), *StringAux));

	UE_LOG(LogTemp, Warning, TEXT("Storing user accs on variable"));
	// Storing user accs on variable
	TArray<FString> AllLines;
	StringAux.ParseIntoArray(AllLines, TEXT("\n"), false);
	AllLines.RemoveAt(0); // first position is ""

	UE_LOG(LogTemp, Warning, TEXT("Get current system date"));
	// Get current system date
	FDateTime CurrentDate = FDateTime::Now();
	int32 CurrentMin;
	int32 CurrentHour;
	int32 CurrentDay;
	int32 CurrentMonth;
	int32 CurrentYear;

	CurrentDate.GetDate(CurrentYear, CurrentMonth, CurrentDay);
	CurrentHour = CurrentDate.GetHour();
	CurrentMin = CurrentDate.GetMinute();
	UE_LOG(LogTemp, Warning, TEXT("System time: %d mi,%d h,%d d, %d mo, %d y"), CurrentMin, CurrentHour, CurrentDay, CurrentMonth, CurrentYear);

	for (FString CurrentLine : AllLines)
	{
		TArray<FString> CurrentLineParsed;
		CurrentLine.ParseIntoArray(CurrentLineParsed, TEXT(" "), false);
		if (CurrentLineParsed.Num() <= 0)
			continue;

		UE_LOG(LogTemp, Warning, TEXT("Check whether or not the account has expirated..."));
		// Check whether or not the account has expirated
		FString DateString = CurrentLineParsed[2];
		FString DayString;
		DayString.AppendChar(DateString[0]);
		DayString.AppendChar(DateString[1]);
		int UserExpirationDay = UKismetStringLibrary::Conv_StringToInt(DayString);

		FString MonthString;
		MonthString.AppendChar(DateString[3]);
		MonthString.AppendChar(DateString[4]);
		int UserExpirationMonth = UKismetStringLibrary::Conv_StringToInt(MonthString);

		FString YearString;
		YearString.AppendChar(DateString[6]);
		YearString.AppendChar(DateString[7]);
		YearString.AppendChar(DateString[8]);
		YearString.AppendChar(DateString[9]);
		int UserExpirationYear = UKismetStringLibrary::Conv_StringToInt(YearString);

		FString TimeString = CurrentLineParsed[3];

		FString HourString;
		HourString.AppendChar(TimeString[0]);
		HourString.AppendChar(TimeString[1]);
		int UserExpirationHour = UKismetStringLibrary::Conv_StringToInt(HourString);

		FString MinString;
		MinString.AppendChar(TimeString[3]);
		MinString.AppendChar(TimeString[4]);
		int UserExpirationMin = UKismetStringLibrary::Conv_StringToInt(MinString);

		FDateTime UserExpirationDate(UserExpirationYear, UserExpirationMonth, UserExpirationDay,
			UserExpirationHour, UserExpirationMin);

		FTimespan CurrentTimespan = UserExpirationDate - CurrentDate;
		int32 MinLeft = CurrentTimespan.GetMinutes();
		UE_LOG(LogTemp, Warning, TEXT("Readed time: %d mi,%d h,%d d, %d mo, %d y"), UserExpirationMin, UserExpirationHour, UserExpirationDay, UserExpirationMonth, UserExpirationYear);

		UE_LOG(LogTemp, Warning, TEXT("Finally adding the account..."));
		// Finally adding the account to valid user accounts
		if (MinLeft > 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("User account valid..."));
			UserAccount.Add(CurrentLineParsed[0], CurrentLineParsed[1]);
			int UserTimeLeftInMin = CurrentTimespan.GetTotalSeconds()/60;
			UserTimeLeft.Add(CurrentLineParsed[0], UserTimeLeftInMin);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("User account expired..."));
			ExpiredAccounts.Add(CurrentLineParsed[0], CurrentLineParsed[1]);
			UserTimeLeft.Add(CurrentLineParsed[0], 0);
		}
	}
	UE_LOG(LogTemp, Warning, TEXT("---- Finished to read data file ----"));
}
