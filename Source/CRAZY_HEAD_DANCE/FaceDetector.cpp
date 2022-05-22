// Fill out your copyright notice in the Description page of Project Settings.


#include "FaceDetector.h"

// Sets default values
AFaceDetector::AFaceDetector()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AFaceDetector::BeginPlay()
{
	Super::BeginPlay();

    // Start Video..1) 0 for WebCam 2) "Path to Video" for a Local Video
    Vcapture.open(2);

    // Change path before execution 
    if (cascade.load("C:\\Program Files\\Epic Games\\UnrealProjects\\CreazyHead\\ThirdParty\\OpenCV/haarcascade_frontalface_default.xml"))
    {
        GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, TEXT("cool!"));
    }
    else
    {
        GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, TEXT("not cool!"));
    }
}

// Called every frame
void AFaceDetector::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
    if (Vcapture.isOpened())
    {
        // Capture frames from video and detect faces
        Vcapture >> frame;
        if (!frame.empty())
        {
            cv::Mat frame1 = frame.clone();
            DetectAndDraw(frame1);
        }
    }
}

void AFaceDetector::DetectAndDraw(cv::Mat& img)
{
    std::vector<cv::Rect> faces;
    cascade.detectMultiScale(img, faces);
    for (size_t i = 0; i < faces.size(); i++)
    {
        int x1 = faces[i].x;
        int y1 = faces[i].y;
        int x2 = faces[i].x + faces[i].width;
        int y2 = faces[i].y + faces[i].height;
        UE_LOG(LogTemp, Warning, TEXT("%d"), x1);
    }
    faces.clear();
}
