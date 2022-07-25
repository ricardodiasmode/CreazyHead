// Fill out your copyright notice in the Description page of Project Settings.


#include "FaceDetector.h"
#include "Engine/Texture.h"
#include "ImageUtils.h"
#include "Kismet/KismetMathLibrary.h"
#include <opencv2/imgproc/types_c.h>
#include "MyGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "opencv2/highgui/highgui_c.h"

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

void AFaceDetector::on_trackbar(int, void*)
{
    red_l = cv::getTrackbarPos("Red Low", "Image Result1");
    red_h = cv::getTrackbarPos("Red High", "Image Result1");

    green_l = cv::getTrackbarPos("green Low", "Image Result1");
    green_h = cv::getTrackbarPos("green High", "Image Result1");

    blue_l = cv::getTrackbarPos("blue Low", "Image Result1");
    blue_h = cv::getTrackbarPos("blue High", "Image Result1");

}

void AFaceDetector::DetectAndDraw()
{
    // Capture frames from video and detect faces
    Vcapture >> frame;
    if (frame.empty())
        return;

    /// Create Windows
    cv::namedWindow("Image Result1", 1);

    cv::createTrackbar("Red Low", "Image Result1", 0, 255, on_trackbar);
    cv::createTrackbar("Red High", "Image Result1", 0, 255, on_trackbar);

    cv::createTrackbar("green Low", "Image Result1", 0, 255, on_trackbar);
    cv::createTrackbar("green High", "Image Result1", 0, 255, on_trackbar);

    cv::createTrackbar("blue Low", "Image Result1", 0, 255, on_trackbar);
    cv::createTrackbar("blue High", "Image Result1", 0, 255, on_trackbar);

    cvSetTrackbarPos("Red Low", "Image Result1", 92);
    cvSetTrackbarPos("Red High", "Image Result1", 242);

    cvSetTrackbarPos("green Low", "Image Result1", 0);
    cvSetTrackbarPos("green High", "Image Result1", 255);

    cvSetTrackbarPos("blue Low", "Image Result1", 0);
    cvSetTrackbarPos("blue High", "Image Result1", 121);

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

    cv::Mat resizedWithoutBackground = resized;
    if (WithChromaKey)
        chromakey(resized, &resizedWithoutBackground);

    // Convert to a 4 channels img
    cv::cvtColor(resizedWithoutBackground, resizedWithAlpha, CV_BGR2BGRA);

    if (resizedWithAlpha.empty())
        return;

    // We wanna only faces on the img
    if (!WithChromaKey)
        RemoveBackgroundWithoutChromaKey();

    if (FacesAsMat.Num() == 0)
        return;

    FrameAsTexture.Empty();
    for (int i = 0; i < FacesAsMat.Num(); i++)
    {
        if (FacesAsMat[i].empty())
            continue;

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

void AFaceDetector::chromakey(const cv::Mat in, cv::Mat* dst)
{
    // Create the destination matrix
    *dst = cv::Mat(in.rows, in.cols, CV_8UC4);

    for (int y = 0; y < in.rows; y++) {
        for (int x = 0; x < in.cols; x++) {

            dst->at<cv::Vec4b>(y, x)[0] = in.at<cv::Vec4b>(y, x)[0];
            dst->at<cv::Vec4b>(y, x)[1] = in.at<cv::Vec4b>(y, x)[1];
            dst->at<cv::Vec4b>(y, x)[2] = in.at<cv::Vec4b>(y, x)[2];
            if (in.at<cv::Vec4b>(y, x)[0] >= red_l && 
                in.at<cv::Vec4b>(y, x)[0] <= red_h && 
                in.at<cv::Vec4b>(y, x)[1] >= green_l && 
                in.at<cv::Vec4b>(y, x)[1] <= green_h &&
                in.at<cv::Vec4b>(y, x)[2] >= blue_l &&
                in.at<cv::Vec4b>(y, x)[2] <= blue_h)
            {
                dst->at<cv::Vec4b>(y, x)[3] = 0;
            }
            else {
                dst->at<cv::Vec4b>(y, x)[3] = 255;
            }

        }
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
                    {// If is not far from center, then we set alpha
                        cv::Vec4b& pixelRef = resizedWithAlpha.at<cv::Vec4b>(YPix, XPix);
                        double a1 = 1.5;
                        double a2 = .5;
                        double aux = a2 * atof(reinterpret_cast<char*>(pixelRef[0])); // Blue adjust
                        double aux2 = atof(reinterpret_cast<char*>(pixelRef[1])); // Green
                        double aux3 = aux2 - aux; // Vlahos formula
                        pixelRef[3] = 255 - a1 * aux3;
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
        if (FaceAdjustmentNoCk < 0 || FaceAdjustmentNoCk * 2 + CurrentRect.width + CurrentRect.x  > resizedWithAlpha.cols ||
            FaceAdjustmentNoCk * 2 + CurrentRect.height + CurrentRect.y > resizedWithAlpha.rows)
            FaceAdjustment = 0;

        // We need to know where is the face on the image
        int XInitialLoc = CurrentRect.x - 10;
        int XSize = CurrentRect.width + 10 *2;
        int YInitialLoc = CurrentRect.y - 10;
        int YSize = CurrentRect.height + 10 * 2;

        // Now, every pixel that is in a 1.5x radius from rectangle and is close to the mean that we found,
        // we keep. Every pixel that is out of the 1.5x radius or is not close to the mean we set as alpha 0;

        // First we get the center of the rec
        int XCenter = XInitialLoc + (XSize/2);
        int YCenter = YInitialLoc + (YSize/2);
        FVector2D CenterLoc(XCenter, YCenter);
        cv::Vec4b leftpixel = resizedWithAlpha.at<cv::Vec4b>(XCenter-(XSize/2), YCenter + (YSize /4));
        cv::Vec4b rightpixel = resizedWithAlpha.at<cv::Vec4b>(XCenter + (XSize / 2), YCenter + (YSize / 4));
        cv::Vec4b abovepixel = resizedWithAlpha.at<cv::Vec4b>(XCenter, YCenter - (YSize / 2));
        FVector PixelMean(((leftpixel[0]+rightpixel[0]+ abovepixel[0])/3),
            ((leftpixel[1] + rightpixel[1] + abovepixel[1]) / 3),
            ((leftpixel[2] + rightpixel[2] + abovepixel[2]) / 3));
        // Then the dist from center to rec diagonal
        float DiagDist = FVector2D::Distance(FVector2D(XInitialLoc, YInitialLoc), CenterLoc);
        for (int YPix = 0; YPix < resizedWithAlpha.rows; YPix++)
        {
            for (int XPix = 0; XPix < resizedWithAlpha.cols ; XPix++)
            {
                FVector2D PixelLoc(XPix, YPix);

                // Whether or not pixel is too far from center
                if (FVector2D::Distance(PixelLoc, CenterLoc) > DiagDist * DistToCenterMultiplierNoCk ||
                    abs(XPix - XCenter) > XSize * XDistToleranceNoCk)
                {
                    // Set alpha to 0
                    cv::Vec4b& pixelRef = resizedWithAlpha.at<cv::Vec4b>(YPix, XPix);
                    pixelRef[3] = 0;
                }
                else
                {
                    // Whether or not pixel is too close from center
                    if (FVector2D::Distance(PixelLoc, CenterLoc) < DiagDist * CloseToCenterMultiplierNoCk &&
                        abs(XPix - XCenter) < XSize * XCloseToleranceNoCk)
                    {
                        // Set alpha to 1
                        cv::Vec4b& pixelRef = resizedWithAlpha.at<cv::Vec4b>(YPix, XPix);
                        pixelRef[3] = 255;
                    }
                    else
                    {
                        // If is not far from center, then we check if is close to mean with a px tolerance
                        cv::Vec4b pixel = resizedWithAlpha.at<cv::Vec4b>(YPix, XPix);
                        if (UKismetMathLibrary::NearlyEqual_FloatFloat(pixel[0], PixelMean[0], PixelToleranceNoCk) &&
                            UKismetMathLibrary::NearlyEqual_FloatFloat(pixel[1], PixelMean[1], PixelToleranceNoCk) &&
                            UKismetMathLibrary::NearlyEqual_FloatFloat(pixel[2], PixelMean[2], PixelToleranceNoCk))
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
