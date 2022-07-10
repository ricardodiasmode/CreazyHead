// Fill out your copyright notice in the Description page of Project Settings.


#include "ShowFaceActor.h"
#include "Kismet/GameplayStatics.h"
#include "MyGameInstance.h"

// Sets default values
AShowFaceActor::AShowFaceActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AShowFaceActor::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AShowFaceActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (Cast<UMyGameInstance>(UGameplayStatics::GetGameInstance(GetWorld()))->CameraIndex != CameraIndex)
	{
		Vcapture.open(Cast<UMyGameInstance>(UGameplayStatics::GetGameInstance(GetWorld()))->CameraIndex);
		CameraIndex = Cast<UMyGameInstance>(UGameplayStatics::GetGameInstance(GetWorld()))->CameraIndex;
	}
	if(Vcapture.isOpened())
	{
		cv::Mat frame;
		Vcapture >> frame;
		cv::Mat resized;
		cv::resize(frame, resized, cv::Size(frame.cols / 2, frame.rows / 2));
		cv::imshow("img", resized);
	}
}

