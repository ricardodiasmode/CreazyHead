// Fill out your copyright notice in the Description page of Project Settings.


#include "FaceDetector.h"

// Sets default values
AFaceDetector::AFaceDetector()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Load classifiers from "opencv/data/haarcascades" directory 
	nestedCascade.load("../../ThirdParty/OpenCV/haarcascade_eye_tree_eyeglasses.xml");

	// Change path before execution 
	cascade.load("../../ThirdParty/OpenCV/haarcascade_frontalface_default.xml");

}

// Called when the game starts or when spawned
void AFaceDetector::BeginPlay()
{
	Super::BeginPlay();

    // Start Video..1) 0 for WebCam 2) "Path to Video" for a Local Video
    capture.open(2);
}

// Called every frame
void AFaceDetector::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
    if (capture.isOpened())
    {
        GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, TEXT("a!"));
        // Capture frames from video and detect faces
        capture >> frame;
        if (!frame.empty())
        {
            GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, TEXT("b!"));
            cv::Mat frame1 = frame.clone();
            DetectAndDraw(frame1);
        }
    }
    else
    {
        GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, TEXT("c!"));
    }
}

void AFaceDetector::DetectAndDraw(cv::Mat& img)
{
    std::vector<cv::Rect> faces;
    std::vector<cv::Rect> faces2;
    cv::Mat gray;
    cv::Mat smallImg;
    
    cvtColor(img, gray, cv::COLOR_BGR2GRAY); // Convert to Gray Scale
    double fx = 1 / scale;

    // Resize the Grayscale Image 
    resize(gray, smallImg, cv::Size(), fx, fx, cv::INTER_LINEAR);
    equalizeHist(smallImg, smallImg);

    // Detect faces of different sizes using cascade classifier 
    //cascade.detectMultiScale(smallImg, faces, 1.1,
    //    2, 0 | cv::CASCADE_SCALE_IMAGE, cv::Size(30, 30));

    // Draw circles around the faces
    for (size_t i = 0; i < faces.size(); i++)
    {
        cv::Rect r = faces[i];
        cv::Mat smallImgROI;
        std::vector<cv::Rect> nestedObjects;
        cv::Point center;
        cv::Scalar color = cv::Scalar(255, 0, 0); // Color for Drawing tool
        int radius;

        double aspect_ratio = (double)r.width / r.height;
        if (0.75 < aspect_ratio && aspect_ratio < 1.3)
        {
            center.x = cvRound((r.x + r.width * 0.5) * scale);
            center.y = cvRound((r.y + r.height * 0.5) * scale);
            radius = cvRound((r.width + r.height) * 0.25 * scale);
            circle(img, center, radius, color, 3, 8, 0);
        }
        else
            cv::rectangle(img, cv::Point(cvRound(r.x * scale), cvRound(r.y * scale)),
                cv::Point(cvRound((r.x + r.width - 1) * scale),
                    cvRound((r.y + r.height - 1) * scale)), color, 3, 8, 0);
        if (nestedCascade.empty())
            continue;
        smallImgROI = smallImg(r);

        // Detection of eyes int the input image
        nestedCascade.detectMultiScale(smallImgROI, nestedObjects, 1.1, 2,
            0 | cv::CASCADE_SCALE_IMAGE, cv::Size(30, 30));

        // Draw circles around eyes
        for (size_t j = 0; j < nestedObjects.size(); j++)
        {
            cv::Rect nr = nestedObjects[j];
            center.x = cvRound((r.x + nr.x + nr.width * 0.5) * scale);
            center.y = cvRound((r.y + nr.y + nr.height * 0.5) * scale);
            radius = cvRound((nr.width + nr.height) * 0.25 * scale);
            cv::circle(img, center, radius, color, 3, 8, 0);
        }
    }
    
    // Show Processed Image with detected faces
    cv::imshow("Face Detection", img);
}
