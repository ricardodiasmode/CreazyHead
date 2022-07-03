// Fill out your copyright notice in the Description page of Project Settings.


#include "FaceDetector.h"
#include "Engine/Texture.h"
#include "ImageUtils.h"
#include "Kismet/KismetMathLibrary.h"
#include <opencv2/imgproc/types_c.h>

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
    cv::resize(frame, source, cv::Size(frame.cols/2, frame.rows/2));

    CurrentRectangles = face_detector.detect_face_rectangles(source);

    // Convert to a 4 channels img
    cv::cvtColor(source, resized, CV_BGR2BGRA);

    if (resized.empty())
        return;

    // We wanna only faces on the img
    RemoveBackground();

    if (FacesAsMat.Num() == 0)
        return;

    FrameAsTexture.Empty();
    for (int i = 0; i < FacesAsMat.Num(); i++)
    {
        if (FacesAsMat[i].empty())
            continue;

        cv::imshow("img", FacesAsMat[i]);

        const int32 SrcWidth = FacesAsMat[i].cols;
        const int32 SrcHeight = FacesAsMat[i].rows;

        // Getting SrcData
        pixelPtr = (uint8_t*)FacesAsMat[i].data;

        // Create the texture
        FrameAsTexture.Add(UTexture2D::CreateTransient(
            SrcWidth,
            SrcHeight,
            PF_B8G8R8A8
        ));

        // Lock the texture so it can be modified
        uint8* MipData = static_cast<uint8*>(FrameAsTexture[i]->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE));

        FMemory::Memcpy(MipData, pixelPtr, SrcWidth * SrcHeight * (4));

        // Unlock the texture
        FrameAsTexture[i]->PlatformData->Mips[0].BulkData.Unlock();
        FrameAsTexture[i]->UpdateResource();
    }
}

void AFaceDetector::RemoveBackground()
{
    FacesAsMat.Empty();
    // Reorder rectangles based on it x position
    bool TryAgain = CurrentRectangles.size() > 1;
    int CurrentRecIdx = 0;
    while (true)
    {
        if (CurrentRecIdx+1 >= CurrentRectangles.size())
            break;

        if (CurrentRectangles[CurrentRecIdx].x > CurrentRectangles[CurrentRecIdx + 1].x)
        {
            cv::Rect Aux = CurrentRectangles[CurrentRecIdx];
            CurrentRectangles[CurrentRecIdx] = CurrentRectangles[CurrentRecIdx+1];
            CurrentRectangles[CurrentRecIdx + 1] = Aux;
            CurrentRecIdx = -1;
        }

        CurrentRecIdx++;
    }

    // Remove background for each face
    for (cv::Rect CurrentRect : CurrentRectangles)
    {
        // Check whether or not we can adjust detection without get out of image
        int LocalFaceAdjustment = FaceAdjustment;
        if (FaceAdjustment < 0 || FaceAdjustment * 2 + CurrentRect.width + CurrentRect.x  > resized.cols ||
            FaceAdjustment * 2 + CurrentRect.height + CurrentRect.y > resized.rows)
            FaceAdjustment = 0;

        // We need to know where is the face on the image
        int XInitialLoc = CurrentRect.x - FaceAdjustment;
        int XSize = CurrentRect.width + FaceAdjustment*2;
        int YInitialLoc = CurrentRect.y - FaceAdjustment;
        int YSize = CurrentRect.height + FaceAdjustment * 2;

        TArray<FVector> PixelMeanArray;
        // This loop walk horizontally inside rectangle
        for (int CurrentHorizontalOffset = XInitialLoc+(XSize* RecSizeReduction); CurrentHorizontalOffset <= XInitialLoc+ XSize - (XSize * RecSizeReduction); CurrentHorizontalOffset++)
        {
            // This loop walk vertically inside the rectangle
            for (int CurrentVerticalOffset = YInitialLoc + (YSize * RecSizeReduction); CurrentVerticalOffset <= YInitialLoc+YSize - (YSize * RecSizeReduction); CurrentVerticalOffset += resized.cols)
            {
                // Current pixel collor
                cv::Vec4b bgrPixel = resized.at<cv::Vec4b>(CurrentHorizontalOffset, CurrentVerticalOffset);

                // Adding pixel color to array
                PixelMeanArray.Add(FVector(bgrPixel[0], bgrPixel[1], bgrPixel[2]));
            }
        }

        // Actually getting the mean and standard deviation
        float XMean = 0.0;
        float YMean = 0.0;
        float ZMean = 0.0;
        for (FVector CurrentPix : PixelMeanArray)
        {
            XMean += CurrentPix.X;
            YMean += CurrentPix.Y;
            ZMean += CurrentPix.Z;
        }
        // Actually getting the mean
        XMean = XMean / PixelMeanArray.Num(); // B
        YMean = YMean / PixelMeanArray.Num(); // G
        ZMean = ZMean / PixelMeanArray.Num(); // R

        float XSum = 0;
        float YSum = 0;
        float ZSum = 0;
        for (FVector CurrentPix : PixelMeanArray)
        {
            XSum += pow(CurrentPix.X - XMean, 2);
            YSum += pow(CurrentPix.Y - YMean, 2);
            ZSum += pow(CurrentPix.Z - ZMean, 2);
        }
        // Actually getting the sd
        float Xsd = sqrt(XSum / PixelMeanArray.Num());
        float Ysd = sqrt(YSum / PixelMeanArray.Num());
        float Zsd = sqrt(ZSum / PixelMeanArray.Num());

        // Now we wanna a new mean that does not consider points that are too far from sd
        float AdjustedXMean = 0.0;
        float AdjustedYMean = 0.0;
        float AdjustedZMean = 0.0;
        for (FVector CurrentPix : PixelMeanArray)
        {
            if (abs(CurrentPix.X - XMean) > (Xsd/ MeanPrecisionMultiplier))
                continue;
            if (abs(CurrentPix.Y - YMean) > (Ysd/ MeanPrecisionMultiplier))
                continue;
            if (abs(CurrentPix.Z - ZMean) > (Zsd/ MeanPrecisionMultiplier))
                continue;
            AdjustedXMean += CurrentPix.X;
            AdjustedYMean += CurrentPix.Y;
            AdjustedZMean += CurrentPix.Z;
        }
        // Actually getting the mean
        AdjustedXMean = AdjustedXMean / PixelMeanArray.Num(); // B
        AdjustedYMean = AdjustedYMean / PixelMeanArray.Num(); // G
        AdjustedZMean = AdjustedZMean / PixelMeanArray.Num(); // R

        // Now, every pixel that is in a 1.5x radius from rectangle and is close to the mean that we found,
        // we keep. Every pixel that is out of the 1.5x radius or is not close to the mean we set as alpha 0;

        // First we get the center of the rec
        int XCenter = XInitialLoc + (XSize/2);
        int YCenter = YInitialLoc + (YSize/2);
        FVector2D CenterLoc(XCenter, YCenter);
        // Then the dist from center to rec diagonal
        float DiagDist = FVector2D::Distance(FVector2D(XInitialLoc, YInitialLoc), CenterLoc);
        for (int YPix = 0; YPix < resized.rows; YPix++)
        {
            for (int XPix = 0; XPix < resized.cols ; XPix++)
            {
                FVector2D PixelLoc(XPix, YPix);

                // Whether or not pixel is too far from center
                if (FVector2D::Distance(PixelLoc, CenterLoc) > DiagDist * DistToCenterMultiplier ||
                    abs(XPix - XCenter) > XSize * XDistTolerance)
                {
                    // Set alpha to 0
                    cv::Vec4b& pixelRef = resized.at<cv::Vec4b>(YPix, XPix);
                    pixelRef[3] = 0;
                }
                else
                {
                    // Whether or not pixel is too close from center
                    if (FVector2D::Distance(PixelLoc, CenterLoc) < DiagDist * CloseToCenterMultiplier &&
                        abs(XPix - XCenter) < XSize * XCloseTolerance)
                    {
                        // Set alpha to 1
                        cv::Vec4b& pixelRef = resized.at<cv::Vec4b>(YPix, XPix);
                        pixelRef[3] = 255;
                    }
                    else
                    {
                        // If is not far from center, then we check if is close to mean with a px tolerance
                        cv::Vec4b pixel = resized.at<cv::Vec4b>(YPix, XPix);
                        if (UKismetMathLibrary::NearlyEqual_FloatFloat(pixel[0], AdjustedXMean, PixelTolerance) &&
                            UKismetMathLibrary::NearlyEqual_FloatFloat(pixel[1], AdjustedYMean, PixelTolerance) &&
                            UKismetMathLibrary::NearlyEqual_FloatFloat(pixel[2], AdjustedZMean, PixelTolerance))
                        {
                            // Set alpha to 1
                            cv::Vec4b& pixelRef = resized.at<cv::Vec4b>(YPix, XPix);
                            pixelRef[3] = 255;
                        }
                        else
                        {
                            // Set alpha to 0
                            cv::Vec4b& pixelRef = resized.at<cv::Vec4b>(YPix, XPix);
                            pixelRef[3] = 0;
                        }
                    }
                }
            }
        }

        // Creating mat around faces
        uchar* recdata = new uchar[XSize * YSize * 4];
        uchar* resizeddata = resized.data;
        int i = 0;
        int NumChannels = resized.channels();
        for (int verticalOffset = YInitialLoc; verticalOffset < YInitialLoc + YSize; verticalOffset++)
        {
            for (int horizontalOffset = XInitialLoc; horizontalOffset < XInitialLoc + XSize; horizontalOffset++)
            {
                // Check whether or not we are getting out of image
                if (((verticalOffset * NumChannels * resized.cols) + (horizontalOffset * NumChannels) + 3) > resized.cols * resized.rows * resized.channels() ||
                    verticalOffset < 0 || horizontalOffset < 0)
                    continue;
                recdata[i] = resizeddata[(verticalOffset * NumChannels * resized.cols) + (horizontalOffset * NumChannels)];
                recdata[i + 1] = resizeddata[(verticalOffset * NumChannels * resized.cols) + (horizontalOffset * NumChannels) + 1];
                recdata[i + 2] = resizeddata[(verticalOffset * NumChannels * resized.cols) + (horizontalOffset * NumChannels) + 2];
                recdata[i + 3] = resizeddata[(verticalOffset * NumChannels * resized.cols) + (horizontalOffset * NumChannels) + 3];
                i += 4;
            }
        }

        FacesAsMat.Add(cv::Mat(cv::Size(XSize, YSize), CV_8UC4, recdata));
    }
}
