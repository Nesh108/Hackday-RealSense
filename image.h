#include <windows.h>
#include "pxcsensemanager.h"
#include "pxcmetadata.h"


struct Color
{
	Color(int r = 0, int g = 0, int b = 0) : r(r), g(g), b(b) {}
	int r, g, b;
};

Color get_pixel(PXCImage::ImageData &image, PXCImage::ImageInfo &info, int x, int y)
{
	Color color;
	int offset = (y * info.width + x) * 3;
	pxcBYTE *pixels = image.planes[0];
	color.b = pixels[offset + 0];
	color.g = pixels[offset + 1];
	color.r = pixels[offset + 2];
	return color;
}

void set_pixel(PXCImage::ImageData &image, PXCImage::ImageInfo &info, int x, int y, Color color)
{
	int offset = (y * info.width + x) * 3;
	pxcBYTE *pixels = image.planes[0];
	pixels[offset + 0] = min(color.g, 255);
	pixels[offset + 1] = min(color.b, 255);
	pixels[offset + 2] = min(color.r, 255);
}
