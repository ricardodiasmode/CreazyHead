// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "opencv2/objdetect.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include "FaceDetector.generated.h"

UCLASS()
class CRAZY_HEAD_DANCE_API AFaceDetector : public AActor
{
	GENERATED_BODY()
private:
	// VideoCapture class for playing video for which faces to be detected
	cv::VideoCapture Vcapture;
	cv::Mat frame;

	// PreDefined trained XML classifiers with facial features
	cv::CascadeClassifier cascade;

private:
	void DetectAndDraw(cv::Mat& img);

public:	
	// Sets default values for this actor's properties
	AFaceDetector();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
