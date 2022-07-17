// Fill out your copyright notice in the Description page of Project Settings.


#include "FaceDetector.h"
#include "Engine/Texture.h"
#include "ImageUtils.h"
#include "Kismet/KismetMathLibrary.h"
#include <opencv2/imgproc/types_c.h>
#include "MyGameInstance.h"
#include "Kismet/GameplayStatics.h"

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

    if (Vcapture.open(Cast<UMyGameInstance>(UGameplayStatics::GetGameInstance(GetWorld()))->CameraIndex))
    {
        // Change path before execution 
        std::string CascadePath = std::string(TCHAR_TO_UTF8(*FPaths::ProjectDir())) + "ThirdParty/OpenCV/haarcascades/haarcascade_frontalcatface.xml";
        if (cascade.load(CascadePath))
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
    std::string FirstPath = std::string(TCHAR_TO_UTF8(*FPaths::ProjectDir())) + "ThirdParty/OpenCV/Includes/opencvAssets/deploy.prototxt";
    std::string SecondPath = std::string(TCHAR_TO_UTF8(*FPaths::ProjectDir())) + "ThirdParty/OpenCV/Includes/opencvAssets/res10_300x300_ssd_iter_140000_fp16.caffemodel";
    network_ = cv::dnn::readNetFromCaffe(FirstPath, SecondPath);

    if (network_.empty()) 
    {
        std::string ThirdPath = std::string(TCHAR_TO_UTF8(*FPaths::ProjectDir())) + "ThirdParty/OpenCV/Includes/opencvAssets/deploy.prototxt";
        std::string FourthPath = std::string(TCHAR_TO_UTF8(*FPaths::ProjectDir())) + "ThirdParty/OpenCV/Includes/opencvAssets/res10_300x300_ssd_iter_140000_fp16.caffemodel";
        std::ostringstream ss;
        ss << "Failed to load network with the following settings:\n"
            << "Configuration: " + ThirdPath + "\n"
            << "Binary: " + FourthPath + "\n";
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
    cv::Mat resized;
    cv::resize(frame, resized, cv::Size(frame.cols/3, frame.rows/3));

    CurrentRectangles.clear();
    CurrentRectangles = face_detector.detect_face_rectangles(resized);

    // Convert to a 4 channels img
    cv::cvtColor(resized, resizedWithAlpha, CV_BGR2BGRA);

    if (resizedWithAlpha.empty())
        return;

    // We wanna only faces on the img
    if (WithChromaKey)
        RemoveBackgroundWithChromaKey();
    else
        RemoveBackgroundWithoutChromaKey();

    if (FacesAsMat.Num() == 0)
        return;

    FrameAsTexture.Empty();
    for (int i = 0; i < FacesAsMat.Num(); i++)
    {
        if (FacesAsMat[i].empty())
            continue;

        //cv::imshow("img", FacesAsMat[i]);

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

void AFaceDetector::RemoveBackgroundWithChromaKey()
{
    FacesAsMat.Empty();
    // Reorder rectangles based on it x position
    for (int CurrentRecIdx = 0; CurrentRectangles.size() < CurrentRecIdx + 1 && CurrentRectangles.size() > 0; CurrentRecIdx++)
    {
        if (CurrentRectangles[CurrentRecIdx].x > CurrentRectangles[CurrentRecIdx + 1].x)
        {
            cv::Rect Aux = CurrentRectangles[CurrentRecIdx];
            CurrentRectangles[CurrentRecIdx] = CurrentRectangles[CurrentRecIdx + 1];
            CurrentRectangles[CurrentRecIdx + 1] = Aux;
            CurrentRecIdx = -1;
        }
    }

    // Remove background for each face
    for (cv::Rect CurrentRect : CurrentRectangles)
    {
        // Check whether or not we can adjust detection without get out of image
        int LocalFaceAdjustment = FaceAdjustment;
        if (FaceAdjustment < 0 || FaceAdjustment * 2 + CurrentRect.width + CurrentRect.x  > resizedWithAlpha.cols ||
            FaceAdjustment * 2 + CurrentRect.height + CurrentRect.y > resizedWithAlpha.rows)
            FaceAdjustment = 0;

        // We need to know where is the face on the image
        int XInitialLoc = CurrentRect.x - FaceAdjustment;
        int XSize = CurrentRect.width + FaceAdjustment * 2;
        int YInitialLoc = CurrentRect.y - FaceAdjustment;
        int YSize = CurrentRect.height + FaceAdjustment * 2;

        // Now, every pixel that is in a 1.5x radius from rectangle and is close to the mean that we found,
        // we keep. Every pixel that is out of the 1.5x radius or is not close to the mean we set as alpha 0;

        // First we get the center of the rec
        int XCenter = XInitialLoc + (XSize / 2);
        int YCenter = YInitialLoc + (YSize / 2);
        FVector2D CenterLoc(XCenter, YCenter);
        // Then the dist from center to rec diagonal
        float DiagDist = FVector2D::Distance(FVector2D(XInitialLoc, YInitialLoc), CenterLoc);
        for (int YPix = 0; YPix < resizedWithAlpha.rows; YPix++)
        {
            for (int XPix = 0; XPix < resizedWithAlpha.cols; XPix++)
            {
                FVector2D PixelLoc(XPix, YPix);

                // Whether or not pixel is too far from center
                if (FVector2D::Distance(PixelLoc, CenterLoc) > DiagDist * DistToCenterMultiplier ||
                    abs(XPix - XCenter) > XSize * XDistTolerance)
                {
                    // Set alpha to 0
                    cv::Vec4b& pixelRef = resizedWithAlpha.at<cv::Vec4b>(YPix, XPix);
                    pixelRef[3] = 0;
                }
                else
                {
                    // Whether or not pixel is too close from center
                    if (FVector2D::Distance(PixelLoc, CenterLoc) < DiagDist * CloseToCenterMultiplier &&
                        abs(XPix - XCenter) < XSize * XCloseTolerance)
                    {
                        // Set alpha to 1
                        cv::Vec4b& pixelRef = resizedWithAlpha.at<cv::Vec4b>(YPix, XPix);
                        pixelRef[3] = 255;
                    }
                    else
                    {
                        // If is not far from center, then we check if is close to green color with a px tolerance
                        cv::Vec4b pixel = resizedWithAlpha.at<cv::Vec4b>(YPix, XPix);
                        if (UKismetMathLibrary::NearlyEqual_FloatFloat(pixel[0], 0, PixelTolerance) &&
                            UKismetMathLibrary::NearlyEqual_FloatFloat(pixel[1], 255, PixelTolerance) &&
                            UKismetMathLibrary::NearlyEqual_FloatFloat(pixel[2], 0, PixelTolerance))
                        {
                            // Set alpha to 0
                            cv::Vec4b& pixelRef = resizedWithAlpha.at<cv::Vec4b>(YPix, XPix);
                            pixelRef[3] = 0;
                        }
                        else
                        {
                            // Set alpha to 1
                            cv::Vec4b& pixelRef = resizedWithAlpha.at<cv::Vec4b>(YPix, XPix);
                            pixelRef[3] = 255;
                        }
                    }
                }
            }
        }

        // Creating mat around faces
        uchar* recdata = new uchar[XSize * YSize * 4];
        uchar* resizeddata = resizedWithAlpha.data;
        int i = 0;
        int NumChannels = resizedWithAlpha.channels();
        for (int verticalOffset = YInitialLoc; verticalOffset < YInitialLoc + YSize; verticalOffset++)
        {
            for (int horizontalOffset = XInitialLoc; horizontalOffset < XInitialLoc + XSize; horizontalOffset++)
            {
                // Check whether or not we are getting out of image
                if (((verticalOffset * NumChannels * resizedWithAlpha.cols) + (horizontalOffset * NumChannels) + 3) > resizedWithAlpha.cols * resizedWithAlpha.rows * resizedWithAlpha.channels() ||
                    verticalOffset < 0 || horizontalOffset < 0)
                    continue;
                recdata[i] = resizeddata[(verticalOffset * NumChannels * resizedWithAlpha.cols) + (horizontalOffset * NumChannels)];
                recdata[i + 1] = resizeddata[(verticalOffset * NumChannels * resizedWithAlpha.cols) + (horizontalOffset * NumChannels) + 1];
                recdata[i + 2] = resizeddata[(verticalOffset * NumChannels * resizedWithAlpha.cols) + (horizontalOffset * NumChannels) + 2];
                recdata[i + 3] = resizeddata[(verticalOffset * NumChannels * resizedWithAlpha.cols) + (horizontalOffset * NumChannels) + 3];
                i += 4;
            }
        }

        FacesAsMat.Add(cv::Mat(cv::Size(XSize, YSize), CV_8UC4, recdata));
    }
}

void AFaceDetector::RemoveBackgroundWithoutChromaKey()
{
    FacesAsMat.Empty();
    // Reorder rectangles based on it x position
    for(int CurrentRecIdx = 0; CurrentRectangles.size() < CurrentRecIdx+1 && CurrentRectangles.size() > 0; CurrentRecIdx++)
    {
        if (CurrentRectangles[CurrentRecIdx].x > CurrentRectangles[CurrentRecIdx + 1].x)
        {
            cv::Rect Aux = CurrentRectangles[CurrentRecIdx];
            CurrentRectangles[CurrentRecIdx] = CurrentRectangles[CurrentRecIdx+1];
            CurrentRectangles[CurrentRecIdx + 1] = Aux;
            CurrentRecIdx = -1;
        }
    }

    // Remove background for each face
    for (cv::Rect CurrentRect : CurrentRectangles)
    {
        // Check whether or not we can adjust detection without get out of image
        int LocalFaceAdjustment = 10;
        if (10 < 0 || 10 * 2 + CurrentRect.width + CurrentRect.x  > resizedWithAlpha.cols ||
            10 * 2 + CurrentRect.height + CurrentRect.y > resizedWithAlpha.rows)
            FaceAdjustment = 0;

        // We need to know where is the face on the image
        int XInitialLoc = CurrentRect.x - 10;
        int XSize = CurrentRect.width + 10 *2;
        int YInitialLoc = CurrentRect.y - 10;
        int YSize = CurrentRect.height + 10 * 2;

        TArray<FVector> PixelMeanArray;
        float XMean = 0.0;
        float YMean = 0.0;
        float ZMean = 0.0;
        // This loop walk horizontally inside rectangle
        for (int CurrentHorizontalOffset = XInitialLoc+(XSize* 0.2); CurrentHorizontalOffset <= XInitialLoc+ XSize - (XSize * 0.2); CurrentHorizontalOffset++)
        {
            // This loop walk vertically inside the rectangle
            for (int CurrentVerticalOffset = YInitialLoc + (YSize * 0.2); CurrentVerticalOffset <= YInitialLoc+YSize - (YSize * 0.2); CurrentVerticalOffset += resizedWithAlpha.cols)
            {
                // Current pixel collor
                cv::Vec4b bgrPixel = resizedWithAlpha.at<cv::Vec4b>(CurrentHorizontalOffset, CurrentVerticalOffset);

                // Adding pixel color to array
                XMean += bgrPixel[0];
                YMean += bgrPixel[1];
                ZMean += bgrPixel[2];
                PixelMeanArray.Add(FVector(bgrPixel[0], bgrPixel[1], bgrPixel[2]));
            }
        }

        // Actually getting the mean
        XMean = XMean / PixelMeanArray.Num(); // B
        YMean = YMean / PixelMeanArray.Num(); // G
        ZMean = ZMean / PixelMeanArray.Num(); // R

        // Getting standart deviation
        float XSum = 0;
        float YSum = 0;
        float ZSum = 0;
        for (FVector CurrentPix : PixelMeanArray)
        {
            XSum += pow(CurrentPix.X - XMean, 2);
            YSum += pow(CurrentPix.Y - YMean, 2);
            ZSum += pow(CurrentPix.Z - ZMean, 2);
        }
        float Xsd = sqrt(XSum / PixelMeanArray.Num());
        float Ysd = sqrt(YSum / PixelMeanArray.Num());
        float Zsd = sqrt(ZSum / PixelMeanArray.Num());

        // Now we wanna a new mean that does not consider points that are too far from sd
        float AdjustedXMean = XMean;
        float AdjustedYMean = YMean;
        float AdjustedZMean = ZMean;
        if (IncreasePrecision)
        {
            AdjustedXMean = 0.0;
            AdjustedYMean = 0.0;
            AdjustedZMean = 0.0;
            for (FVector CurrentPix : PixelMeanArray)
            {
                if (abs(CurrentPix.X - XMean) > (Xsd / 1.0))
                    continue;
                if (abs(CurrentPix.Y - YMean) > (Ysd / 1.0))
                    continue;
                if (abs(CurrentPix.Z - ZMean) > (Zsd / 1.0))
                    continue;
                AdjustedXMean += CurrentPix.X;
                AdjustedYMean += CurrentPix.Y;
                AdjustedZMean += CurrentPix.Z;
            }
            // Actually getting the mean
            AdjustedXMean = AdjustedXMean / PixelMeanArray.Num(); // B
            AdjustedYMean = AdjustedYMean / PixelMeanArray.Num(); // G
            AdjustedZMean = AdjustedZMean / PixelMeanArray.Num(); // R
        }

        // Now, every pixel that is in a 1.5x radius from rectangle and is close to the mean that we found,
        // we keep. Every pixel that is out of the 1.5x radius or is not close to the mean we set as alpha 0;

        // First we get the center of the rec
        int XCenter = XInitialLoc + (XSize/2);
        int YCenter = YInitialLoc + (YSize/2);
        FVector2D CenterLoc(XCenter, YCenter);
        // Then the dist from center to rec diagonal
        float DiagDist = FVector2D::Distance(FVector2D(XInitialLoc, YInitialLoc), CenterLoc);
        for (int YPix = 0; YPix < resizedWithAlpha.rows; YPix++)
        {
            for (int XPix = 0; XPix < resizedWithAlpha.cols ; XPix++)
            {
                FVector2D PixelLoc(XPix, YPix);

                // Whether or not pixel is too far from center
                if (FVector2D::Distance(PixelLoc, CenterLoc) > DiagDist * 0.75 ||
                    abs(XPix - XCenter) > XSize * 0.8)
                {
                    // Set alpha to 0
                    cv::Vec4b& pixelRef = resizedWithAlpha.at<cv::Vec4b>(YPix, XPix);
                    pixelRef[3] = 0;
                }
                else
                {
                    // Whether or not pixel is too close from center
                    if (FVector2D::Distance(PixelLoc, CenterLoc) < DiagDist * 0.7 &&
                        abs(XPix - XCenter) < XSize * 0.45)
                    {
                        // Set alpha to 1
                        cv::Vec4b& pixelRef = resizedWithAlpha.at<cv::Vec4b>(YPix, XPix);
                        pixelRef[3] = 255;
                    }
                    else
                    {
                        // If is not far from center, then we check if is close to mean with a px tolerance
                        cv::Vec4b pixel = resizedWithAlpha.at<cv::Vec4b>(YPix, XPix);
                        if (UKismetMathLibrary::NearlyEqual_FloatFloat(pixel[0], AdjustedXMean, 150) &&
                            UKismetMathLibrary::NearlyEqual_FloatFloat(pixel[1], AdjustedYMean, 150) &&
                            UKismetMathLibrary::NearlyEqual_FloatFloat(pixel[2], AdjustedZMean, 150))
                        {
                            // Set alpha to 1
                            cv::Vec4b& pixelRef = resizedWithAlpha.at<cv::Vec4b>(YPix, XPix);
                            pixelRef[3] = 255;
                        }
                        else
                        {
                            // Set alpha to 0
                            cv::Vec4b& pixelRef = resizedWithAlpha.at<cv::Vec4b>(YPix, XPix);
                            pixelRef[3] = 0;
                        }
                    }
                }
            }
        }

        // Creating mat around faces
        uchar* recdata = new uchar[XSize * YSize * 4];
        uchar* resizeddata = resizedWithAlpha.data;
        int i = 0;
        int NumChannels = resizedWithAlpha.channels();
        for (int verticalOffset = YInitialLoc; verticalOffset < YInitialLoc + YSize; verticalOffset++)
        {
            for (int horizontalOffset = XInitialLoc; horizontalOffset < XInitialLoc + XSize; horizontalOffset++)
            {
                // Check whether or not we are getting out of image
                if (((verticalOffset * NumChannels * resizedWithAlpha.cols) + (horizontalOffset * NumChannels) + 3) > resizedWithAlpha.cols * resizedWithAlpha.rows * resizedWithAlpha.channels() ||
                    verticalOffset < 0 || horizontalOffset < 0)
                    continue;
                recdata[i] = resizeddata[(verticalOffset * NumChannels * resizedWithAlpha.cols) + (horizontalOffset * NumChannels)];
                recdata[i + 1] = resizeddata[(verticalOffset * NumChannels * resizedWithAlpha.cols) + (horizontalOffset * NumChannels) + 1];
                recdata[i + 2] = resizeddata[(verticalOffset * NumChannels * resizedWithAlpha.cols) + (horizontalOffset * NumChannels) + 2];
                recdata[i + 3] = resizeddata[(verticalOffset * NumChannels * resizedWithAlpha.cols) + (horizontalOffset * NumChannels) + 3];
                i += 4;
            }
        }

        FacesAsMat.Add(cv::Mat(cv::Size(XSize, YSize), CV_8UC4, recdata));
    }
}
