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

    if (Vcapture.isOpened())
    {
        // Change path before execution 
        if (cascade.load("C:/Program Files/Epic Games/UnrealProjects/CreazyHead/ThirdParty/OpenCV/haarcascades/haarcascade_frontalcatface.xml"))
        {
            GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, TEXT("DB Faces loaded!"));
            UE_LOG(LogTemp, Warning, TEXT("DB Faces loaded!"));
            FTimerHandle UnusedTimer;
            GetWorldTimerManager().SetTimer(UnusedTimer, this, &AFaceDetector::DetectAndDraw, 0.05f, true, 0.f);
        }
    }
}

// Called every frame
void AFaceDetector::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AFaceDetector::DetectAndDraw()
{
    // Capture frames from video and detect faces
    Vcapture >> frame;
        
    // Whether or not captured something
    if (frame.empty())
        return;
    cv::Mat img = frame.clone();

    // Whether or not there is a image
    if (img.empty())
        return;

    std::vector<cv::Rect> faces;

    cascade.detectMultiScale(img, faces);
    imshow("Image", img);

    // Whether or not there is a face
    if(faces.size() == 0)
        return;

    UE_LOG(LogTemp, Warning, TEXT("%d"), faces.size());
    for (size_t i = 0; i < faces.size(); i++)
    {
        int x1 = faces[i].x;
        int y1 = faces[i].y;
        int x2 = faces[i].x + faces[i].width;
        int y2 = faces[i].y + faces[i].height;
    }
}
