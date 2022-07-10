// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "opencv2/objdetect.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/dnn/dnn.hpp"
#include "opencv2/opencv.hpp"
#include "ShowFaceActor.generated.h"

UCLASS()
class CRAZY_HEAD_DANCE_API AShowFaceActor : public AActor
{
	GENERATED_BODY()

private:
	// VideoCapture class for playing video for which faces to be detected
	cv::VideoCapture Vcapture;

	int CameraIndex = -1;
public:	
	// Sets default values for this actor's properties
	AShowFaceActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
