// Fill out your copyright notice in the Description page of Project Settings.


#include "FaceDetector.h"
#include "Engine/Texture.h"
#include "ImageUtils.h"

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
            GetWorldTimerManager().SetTimer(UnusedTimer, this, &AFaceDetector::DetectAndDraw, 0.05f, true, 1.f);
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
        
    AuxFaceDetector face_detector;

    CurrentRectangles = face_detector.detect_face_rectangles(frame);
    cv::Scalar color(0, 105, 205);
    int frame_thickness = 4;
    for (const auto& r : CurrentRectangles)
        cv::rectangle(frame, r, color, frame_thickness);
    if(CurrentRectangles.size() > 0)
        ConvertMatToOpenCV();
}

AuxFaceDetector::AuxFaceDetector() : confidence_threshold_(0.5), input_image_height_(300), input_image_width_(300),
scale_factor_(1.0), mean_values_({ 104., 177.0, 123.0 }) 
{

    network_ = cv::dnn::readNetFromCaffe("C:/Program Files/Epic Games/UnrealProjects/CreazyHead/ThirdParty/OpenCV/Includes/opencvAssets/deploy.prototxt", "C:/Program Files/Epic Games/UnrealProjects/CreazyHead/ThirdParty/OpenCV/Includes/opencvAssets/res10_300x300_ssd_iter_140000_fp16.caffemodel");

    if (network_.empty()) 
    {
        std::ostringstream ss;
        ss << "Failed to load network with the following settings:\n"
            << "Configuration: " + std::string("C:/Program Files/Epic Games/UnrealProjects/CreazyHead/ThirdParty/OpenCV/Includes/opencvAssets/deploy.prototxt") + "\n"
            << "Binary: " + std::string("C:/Program Files/Epic Games/UnrealProjects/CreazyHead/ThirdParty/OpenCV/Includes/opencvAssets/res10_300x300_ssd_iter_140000_fp16.caffemodel") + "\n";
        throw std::invalid_argument(ss.str());
    }
}

std::vector<cv::Rect> AuxFaceDetector::detect_face_rectangles(const cv::Mat& frame) 
{
    cv::Mat input_blob = cv::dnn::blobFromImage(frame, scale_factor_, cv::Size(input_image_width_, input_image_height_),
        mean_values_, false, false);
    network_.setInput(input_blob, "data");
    cv::Mat detection = network_.forward("detection_out");
    cv::Mat detection_matrix(detection.size[2], detection.size[3], CV_32F, detection.ptr<float>());

    std::vector<cv::Rect> faces;

    for (int i = 0; i < detection_matrix.rows; i++) 
    {
        float confidence = detection_matrix.at<float>(i, 2);

        if (confidence < confidence_threshold_) {
            continue;
        }
        int x_left_bottom = static_cast<int>(detection_matrix.at<float>(i, 3) * frame.cols);
        int y_left_bottom = static_cast<int>(detection_matrix.at<float>(i, 4) * frame.rows);
        int x_right_top = static_cast<int>(detection_matrix.at<float>(i, 5) * frame.cols);
        int y_right_top = static_cast<int>(detection_matrix.at<float>(i, 6) * frame.rows);

        faces.emplace_back(x_left_bottom, y_left_bottom, (x_right_top - x_left_bottom), (y_right_top - y_left_bottom));
    }

    return faces;
}

void AFaceDetector::ConvertMatToOpenCV()
{
    const int32 SrcWidth = CurrentRectangles[0].width;
    const int32 SrcHeight = CurrentRectangles[0].height;
    const bool UseAlpha = true;
    // Create the texture
    FrameAsTexture = UTexture2D::CreateTransient(
        SrcWidth,
        SrcHeight,
        PF_B8G8R8A8
    );

    // Getting SrcData
    uint8_t* pixelPtr = (uint8_t*)frame.data;
    int cn = frame.channels();
    cv::Scalar_<uint8_t> bgrPixel;
    TArray<FColor*> ImageColor;

    for (int i = 0; i < frame.rows; i++)
    {
        for (int j = 0; j < frame.cols; j++)
        {
            // Getting pixel rgb values
            bgrPixel.val[0] = pixelPtr[i * frame.cols * cn + j * cn + 0]; // B
            bgrPixel.val[1] = pixelPtr[i * frame.cols * cn + j * cn + 1]; // G
            bgrPixel.val[2] = pixelPtr[i * frame.cols * cn + j * cn + 2]; // R
            uint8 ImageR = bgrPixel.val[2];
            uint8 ImageG = bgrPixel.val[1];
            uint8 ImageB = bgrPixel.val[0];

            // Storing RGB values
            ImageColor.Add(new FColor(ImageR, ImageG, ImageB, 1));
        }
    }

    // Lock the texture so it can be modified
    uint8* MipData = static_cast<uint8*>(FrameAsTexture->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE));

    // Create base mip.
    uint8* DestPtr = NULL;
    const FColor* SrcPtr = NULL;
    for (int32 y = 0; y < SrcHeight; y++)
    {
        DestPtr = &MipData[(SrcHeight - 1 - y) * SrcWidth * sizeof(FColor)];
        SrcPtr = const_cast<FColor*>(ImageColor[(SrcHeight - 1 - y) * SrcWidth]);
        for (int32 x = 0; x < SrcWidth; x++)
        {
            *DestPtr++ = SrcPtr->B;
            *DestPtr++ = SrcPtr->G;
            *DestPtr++ = SrcPtr->R;
            if (UseAlpha)
            {
                *DestPtr++ = SrcPtr->A;
            }
            else
            {
                *DestPtr++ = 0xFF;
            }
            SrcPtr++;
        }
    }

    // Unlock the texture
    FrameAsTexture->PlatformData->Mips[0].BulkData.Unlock();
    FrameAsTexture->UpdateResource();
}
