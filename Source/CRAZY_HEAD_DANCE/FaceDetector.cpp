// Fill out your copyright notice in the Description page of Project Settings.


#include "FaceDetector.h"
#include "Engine/Texture.h"
#include "ImageUtils.h"
#include "Kismet/KismetMathLibrary.h"

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
    if (frame.empty())
        return;

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
    cv::Mat source;
    cv::resize(frame, source, cv::Size(frame.cols/3, frame.rows/3));
    cv::imshow("img", source);

    CurrentRectangles = face_detector.detect_face_rectangles(source);

    // Convert to a 4 channels img
    resized = cv::Mat(source.size(), CV_MAKE_TYPE(source.depth(), 4));
    int from_to[] = { 0,0, 1,1, 2,2, 2,3 };
    cv::mixChannels(&source, 1, &resized, 1, from_to, 4);

    const int32 SrcWidth = resized.cols;
    const int32 SrcHeight = resized.rows;

    // Create the texture
    FrameAsTexture = UTexture2D::CreateTransient(
        SrcWidth,
        SrcHeight,
        PF_B8G8R8A8 
    );

    // We wanna only faces on the img
    //RemoveBackground(); // THIS IS CRASHING

    // Getting SrcData
    pixelPtr = (uint8_t*)resized.data;

    // Lock the texture so it can be modified
    uint8* MipData = static_cast<uint8*>(FrameAsTexture->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE));

    FMemory::Memcpy(MipData, pixelPtr, SrcWidth * SrcHeight * (4));

    // Unlock the texture
    FrameAsTexture->PlatformData->Mips[0].BulkData.Unlock();
    FrameAsTexture->UpdateResource();
}

void AFaceDetector::RemoveBackground()
{
    if (resized.empty())
        return;
    // Remove background for each face
    for (cv::Rect CurrentRect : CurrentRectangles)
    {
        // We need to know where is the face on the image
        int XInitialLoc = CurrentRect.x;
        int XSize = CurrentRect.width;
        int YInitialLoc = CurrentRect.y;
        int YSize = CurrentRect.height;

        TArray<FVector> PixelMeanArray;
        // Now, inside the rectangle we get the mean of colors
        // Note that top left pos of the rectangle is pixelPtr[XInitialLoc*YInitialLoc]
        // This loop walk horizontally inside rectangle
        for (int CurrentHorizontalOffset = XInitialLoc; CurrentHorizontalOffset <= XInitialLoc+ XSize; CurrentHorizontalOffset++)
        {
            // This loop walk vertically inside the rectangle
            for (int CurrentVerticalOffset = YInitialLoc; CurrentVerticalOffset <= YInitialLoc+YSize ; CurrentVerticalOffset += resized.cols)
            {
                // Current pixel collor
                cv::Vec4b bgrPixel = resized.at<cv::Vec4b>(CurrentHorizontalOffset, CurrentVerticalOffset);

                // Adding pixel color to array
                PixelMeanArray.Add(FVector(bgrPixel[0], bgrPixel[1], bgrPixel[2]));
            }
        }

        // Actually getting the mean
        float XMean = 0.0;
        float YMean = 0.0;
        float ZMean = 0.0;
        for (FVector CurrentPix : PixelMeanArray)
        {
            XMean += CurrentPix.X;
            YMean += CurrentPix.Y;
            ZMean += CurrentPix.Z;
        }
        XMean = XMean / PixelMeanArray.Num(); // B
        YMean = YMean / PixelMeanArray.Num(); // G
        ZMean = ZMean / PixelMeanArray.Num(); // R

        // Now, every pixel that is in a 1.5x radius from rectangle and is close to the mean that we found,
        // we keep. Every pixel that is out of the 1.5x radius or is not close to the mean we set as alpha 0;

        // First we get the center of the rec
        int XCenter = XInitialLoc + (XInitialLoc + CurrentRect.x) / 2;
        int YCenter = YInitialLoc + (YInitialLoc + CurrentRect.y) / 2;
        FVector2D CenterLoc(XCenter, YCenter);
        // Then the dist from center to rec diagonal
        float DiagDist = FVector2D::Distance(FVector2D(XInitialLoc, YInitialLoc), CenterLoc);
        for (int XPix =0;XPix < resized.cols;XPix++)
        {
            for (int YPix =0;YPix < resized.rows;YPix++)
            {
                FVector2D PixelLoc(XPix, YPix);

                // Whether or not pixel is too far from center
                if (FVector2D::Distance(PixelLoc, CenterLoc) > DiagDist * 1.5)
                {
                    // Set alpha to 0
                    cv::Vec4b& pixelRef = resized.at<cv::Vec4b>(XPix, YPix);
                    pixelRef[3] = 0;
                }
                else
                {
                    // If is not far from center, then we check if is close to mean with a 10px tolerance
                    cv::Vec4b pixel = resized.at<cv::Vec4b>(XPix, YPix);
                    if (!(UKismetMathLibrary::NearlyEqual_FloatFloat(pixel[0], XMean, 10) &&
                        UKismetMathLibrary::NearlyEqual_FloatFloat(pixel[1], YMean, 10) &&
                        UKismetMathLibrary::NearlyEqual_FloatFloat(pixel[2], ZMean, 10)))
                    {
                        cv::Vec4b& pixelRef = resized.at<cv::Vec4b>(XPix, YPix);
                        pixelRef[3] = 0;
                    }
                }
            }
        }
    }
}
