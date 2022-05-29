// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "opencv2/objdetect.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/dnn/dnn.hpp"
#include "opencv2/opencv.hpp"
#include "FaceDetector.generated.h"

class UTexture2D;

UCLASS()
class CRAZY_HEAD_DANCE_API AFaceDetector : public AActor
{
	GENERATED_BODY()
private:
	// VideoCapture class for playing video for which faces to be detected
	cv::VideoCapture Vcapture;
	cv::Mat frame;
    //std::vector<cv::Rect> CurrentRectangles;

	// PreDefined trained XML classifiers with facial features
	cv::CascadeClassifier cascade;

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
        UTexture2D* FrameAsTexture;

private:
	void DetectAndDraw();
    void ConvertMatToOpenCV();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
    // Sets default values for this actor's properties
    AFaceDetector();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

};

class AuxFaceDetector 
{
public:
    explicit AuxFaceDetector();

    /// Detect faces in an image frame
    /// \param frame Image to detect faces in
    /// \return Vector of detected faces
    std::vector<cv::Rect> detect_face_rectangles(const cv::Mat& frame);

private:
    /// Face detection network
    cv::dnn::Net network_;
    /// Input image width
    const int input_image_width_;
    /// Input image height
    const int input_image_height_;
    /// Scale factor when creating image blob
    const double scale_factor_;
    /// Mean normalization values network was trained with
    const cv::Scalar mean_values_;
    /// Face detection confidence threshold
    const float confidence_threshold_;

};
