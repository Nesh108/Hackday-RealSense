#include "image.h"


void manipulate_pixels(const PXCCapture::Sample *sample)
{
	// Request image
	PXCImage::ImageData image;
	sample->color->AcquireAccess(PXCImage::ACCESS_READ_WRITE, PXCImage::PIXEL_FORMAT_RGB24, &image);
	PXCImage::ImageInfo info = sample->color->QueryInfo();

	// Manipulate image
	for (int y = 0; y < info.height; ++y) {
		for (int x = 0; x < info.width; ++x) {
			Color color = get_pixel(image, info, x, y);
			color.r *= 2;
			set_pixel(image, info, x, y, color);
		}
	}

	// Release image
	sample->color->ReleaseAccess(&image);
}
