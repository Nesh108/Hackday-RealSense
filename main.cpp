/*******************************************************************************

INTEL CORPORATION PROPRIETARY INFORMATION
This software is supplied under the terms of a license agreement or nondisclosure
agreement with Intel Corporation and may not be copied or disclosed except in
accordance with the terms of that agreement
Copyright(c) 2011-2014 Intel Corporation. All Rights Reserved.

*******************************************************************************/
#include <windows.h>
#include "pxcsensemanager.h"
#include "pxcmetadata.h"
#include "util_cmdline.h"
#include "util_render.h"
#include <conio.h>
#include <iostream>
#include <array>
#include "tracking.h"

#include "markersFinder.h"

int wmain(int argc, WCHAR* argv[]) {
    /* Creates an instance of the PXCSenseManager */
    PXCSenseManager *pp = PXCSenseManager::CreateInstance();
    if (!pp) {
        wprintf_s(L"Unable to create the SenseManager\n");
        return 3;
    }

    // Optional steps to send feedback to Intel Corporation to understand how often each SDK sample is used.
    PXCMetadata * md = pp->QuerySession()->QueryInstance<PXCMetadata>();
    if (md) {
        pxcCHAR sample_name[] = L"Camera Viewer";
        md->AttachBuffer(PXCSessionService::FEEDBACK_SAMPLE_INFO, (pxcBYTE*)sample_name, sizeof(sample_name));
    }

    /* Collects command line arguments */
    UtilCmdLine cmdl(pp->QuerySession());
    if (!cmdl.Parse(L"-listio-nframes-sdname-csize-dsize-isize-file-record-noRender-mirror",argc,argv)) return 3;

    /* Sets file recording or playback */
    PXCCaptureManager *cm=pp->QueryCaptureManager();
    cm->SetFileName(cmdl.m_recordedFile, cmdl.m_bRecord);
    if (cmdl.m_sdname) cm->FilterByDeviceInfo(cmdl.m_sdname,0,0);

	// Create stream renders
    UtilRender renderc(L"Color"), renderd(L"Depth"), renderi(L"IR");
    pxcStatus sts;
    do {
        /* Apply command line arguments */
        if (cmdl.m_csize.size()>0) {
            pp->EnableStream(PXCCapture::STREAM_TYPE_COLOR, cmdl.m_csize.front().first.width, cmdl.m_csize.front().first.height, (pxcF32)cmdl.m_csize.front().second);
        }
        if (cmdl.m_dsize.size()>0) {
            pp->EnableStream(PXCCapture::STREAM_TYPE_DEPTH, cmdl.m_dsize.front().first.width, cmdl.m_dsize.front().first.height, (pxcF32)cmdl.m_dsize.front().second);
        }
        if (cmdl.m_isize.size()>0) {
            pp->EnableStream(PXCCapture::STREAM_TYPE_IR,    cmdl.m_isize.front().first.width, cmdl.m_isize.front().first.height, (pxcF32)cmdl.m_isize.front().second);
        }
        if (cmdl.m_csize.size()==0 && cmdl.m_dsize.size()==0 && cmdl.m_isize.size()==0) {
            PXCVideoModule::DataDesc desc={};
            if (cm->QueryCapture()) {
                cm->QueryCapture()->QueryDeviceInfo(0, &desc.deviceInfo);
            } else {
                desc.deviceInfo.streams = PXCCapture::STREAM_TYPE_COLOR | PXCCapture::STREAM_TYPE_DEPTH;
            }
            pp->EnableStreams(&desc);
        }

        /* Initializes the pipeline */
        sts = pp->Init();

        if (sts<PXC_STATUS_NO_ERROR) {
            wprintf_s(L"Failed to locate any video stream(s)\n");
            break;
        }
        if ( cmdl.m_bMirror ) {
            pp->QueryCaptureManager()->QueryDevice()->SetMirrorMode( PXCCapture::Device::MirrorMode::MIRROR_MODE_HORIZONTAL );
        } else {
            pp->QueryCaptureManager()->QueryDevice()->SetMirrorMode( PXCCapture::Device::MirrorMode::MIRROR_MODE_DISABLED );
        }

        /* Stream Data */
        for (int nframes=0;nframes<cmdl.m_nframes;nframes++) {
            /* Waits until new frame is available and locks it for application processing */
            sts=pp->AcquireFrame(false);

            if (sts<PXC_STATUS_NO_ERROR) {
                if (sts==PXC_STATUS_STREAM_CONFIG_CHANGED) {
                    wprintf_s(L"Stream configuration was changed, re-initilizing\n");
                    pp->Close();
                }
                break;
            }

			const PXCCapture::Sample *sample = pp->QuerySample();

			PXCImage::ImageData data;
				PXCImage::ImageInfo dataInfo;

				// OPENCV Stuff
				if (sample->color->AcquireAccess(PXCImage::ACCESS_READ, PXCImage::PixelFormat::PIXEL_FORMAT_RGB24, &data) >= PXC_STATUS_NO_ERROR)
				{
					dataInfo = sample->color->QueryInfo();
					
					IplImage* colorimg = cvCreateImageHeader(cvSize(dataInfo.width, dataInfo.height), 8, 3);
					cvSetData(colorimg, (uchar*)data.planes[0], dataInfo.width * 3);
					cv::Mat image(colorimg);
					Mat dest;
					vector<Point2f> square_centers = findSquares(image, dest, 0.04);
					
					Point text_pos;
					text_pos.x = 40;
					text_pos.y = 40;
					char fc[200];

					sprintf(fc, "Found: %d squares.", square_centers.size());
					putText(dest, fc, text_pos, FONT_HERSHEY_PLAIN, 3,
						Scalar::all(255), 2, CV_AA);

					imshow("OpenCV", dest);
					waitKey(10);
					sample->color->ReleaseAccess(&data);
				}
				////////

			manipulate_pixels(sample);

            /* Render streams, unless -noRender is selected */
            if (cmdl.m_bNoRender == false) {
                if (sample) {
                    if (sample->depth && !renderd.RenderFrame(sample->depth)) break;
                    if (sample->color && !renderc.RenderFrame(sample->color)) break;
                    if (sample->ir    && !renderi.RenderFrame(sample->ir))    break;
                }
            }

			/* Releases lock so pipeline can process next frame */
            pp->ReleaseFrame();

            if( _kbhit() ) { // Break loop
                int c = _getch() & 255;
                if( c == 27 || c == 'q' || c == 'Q') break; // ESC|q|Q for Exit
            }
        }

    } while (sts == PXC_STATUS_STREAM_CONFIG_CHANGED);

    wprintf_s(L"Exiting\n");

    // Clean Up
    pp->Release();
    return 0;
}
