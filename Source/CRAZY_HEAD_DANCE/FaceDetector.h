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

UCLASS()
class CRAZY_HEAD_DANCE_API AFaceDetector : public AActor
{
	GENERATED_BODY()
private:
	// VideoCapture class for playing video for which faces to be detected
	cv::VideoCapture Vcapture;
	cv::Mat frame;
    cv::Mat resizedWithAlpha;
    TArray<cv::Mat> FacesAsMat;
    std::vector<cv::Rect> CurrentRectangles;

    // Hold img data
    uint8_t* pixelPtr;

	// PreDefined trained XML classifiers with facial features
	cv::CascadeClassifier cascade;
    AuxFaceDetector face_detector;

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
        TArray<UTexture2D*> FrameAsTexture;
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
        int PixelTolerance = 100;
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
        float CloseToCenterMultiplier = 0.5f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
        float XCloseTolerance = 0.6f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
        float DistToCenterMultiplier = 1.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
        float XDistTolerance = 0.6f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
        float YDistTolerance = 0.6f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
        int FaceAdjustment = 10;


    UPROPERTY(EditAnywhere, BlueprintReadWrite)
        int PixelToleranceNoCk = 150;
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
        float CloseToCenterMultiplierNoCk = 0.7f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
        float XCloseToleranceNoCk = 0.45f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
        float DistToCenterMultiplierNoCk = 0.75f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
        float XDistToleranceNoCk = 0.8f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
        float MeanPrecisionMultiplierNoCk = 1.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
        float RecSizeReductionNoCk = 0.2f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
        int FaceAdjustmentNoCk = 10;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
        bool IncreasePrecision = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
        bool WithChromaKey = true;

private:
	void DetectAndDraw();
    void ConvertMatToOpenCV();
    void RemoveBackgroundWithoutChromaKey();
    void RemoveBackgroundWithChromaKey();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
    // Sets default values for this actor's properties
    AFaceDetector();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
