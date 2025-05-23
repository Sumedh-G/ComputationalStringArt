#include "options.h"
#include "images.h"
#include "filters.h"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define min(a, b) ((a < b) ? (a) : (b))

NormImage* NormalizeRawImage(RawImage* inputImage)
{
  #if DEBUG
    fprintf(stderr, "[INFO] Normalizing image!\n");
  #endif

  NormImage* outputImage;
  outputImage = LoadNormImage(inputImage->width, inputImage->height, inputImage->numColorChannels);
  if (!outputImage) {
    #if DEBUG
      fprintf(stderr, "[ERROR] <%s:%u> Failed to load norm image.\n", __FILE__, __LINE__);
    #endif
    return NULL;
  }

  for (int i=0; i < inputImage->width * inputImage->height * inputImage->numColorChannels; ++i) {
    outputImage->data[i] = ((double)inputImage->data[i]) / 255.0;
  }

  #if DEBUG
    fprintf(stderr, "[INFO] Finished normalization!\n");
  #endif

  return outputImage;
}

NormImage* RgbToPrintable(RawImage* inputImage)
{
  #if DEBUG
    fprintf(stderr, "[INFO] Converting image to printable colors!\n");
  #endif

  if (inputImage->numColorChannels != 3) {
    #if DEBUG
      fprintf(stderr, "[ERROR] <%s:%u> Tried to convert non-rgb image into printable colors.\n", __FILE__, __LINE__);
    #endif
    return NULL;
  }
  NormImage* outputImage;
  unsigned long int monoChromeBufferWidth = inputImage->width * inputImage->height;

  outputImage = LoadNormImage(inputImage->width, inputImage->height, 5);
  if (!outputImage) {
    #if DEBUG
      fprintf(stderr, "[ERROR] <%s:%u> Failed to load output image.\n", __FILE__, __LINE__);
    #endif
    return NULL;
  }

  // Translate rgb into cmy
  for (int color=0; color<3; ++color) {
    for (int i=0; i<monoChromeBufferWidth; ++i) {
      outputImage->data[i*5 + color] = ((float) 255 - inputImage->data[i*3 + color]) / 255;
    }
  }

  // Prepare k stream
  for (int i=0; i<monoChromeBufferWidth; ++i) {
    outputImage->data[i*5 + 3] = min(min(min(outputImage->data[i*5], outputImage->data[i*5 + 1]), outputImage->data[i*5 + 2]), 0.999);
  }

  // remove redundant black from cmy
  for (int color=0; color<3; ++color) {
    for (int i=0; i<monoChromeBufferWidth; ++i) {
      outputImage->data[i*5 + color] -= outputImage->data[i*5 + 3];
      outputImage->data[i*5 + color] /= (1 - outputImage->data[i*5 + 3]);
    }
  }

  // Prepare w stream
  for (int i=0; i<monoChromeBufferWidth; ++i) {
    outputImage->data[i*5 + 4] = (
      (double)(inputImage->data[i*3 + 0] + inputImage->data[i*3 + 1] + inputImage->data[i*3 + 2]) / (255 * 3)
    );
  }

  return outputImage;
}


/*
* Radon Transform
*
* Split the pixels of the image into sub-pixels. For each angle, project the centers of the subpixels
* onto the line perpendicular to the line of given angle. Bin the subpixels based on
* ratio of distance to adjacent bins.
*
* If the given pixel is projected onto the bin exactly, give it the complete magnitude of
* the subpixel.
*
* MATLAB uses 4 subpixels per pixel
*
* resolution : how many subpixels make 1 side of a pixel (default 2)
*
*/

NormImage* RadonTransform(NormImage* inputImage, double angles[], int nangles, int nbins, int color, int resolution)
{
  #if DEBUG
    fprintf(stderr, "[INFO] Performing Radon Trasform!\n");
  #endif

  if (color >= inputImage->numColorChannels) {
    #if DEBUG
      fprintf(stderr, "[ERROR] <%s:%u> Tried to access invalid color channel in image.\n", __FILE__, __LINE__);
    #endif
    return NULL;
  }
  NormImage* transformImage = LoadNormImage(nangles, nbins, 1);

  double xc, yc, hyp;
  xc = ceil((double)inputImage->width / 2);
  yc = ceil((double)inputImage->height / 2);
  hyp = sqrt(inputImage->width*inputImage->width + inputImage->height*inputImage->height);

  // fprintf(stderr, "[INFO] center_x=%lf, center_y=%lf, hyp=%lf\n", xc, yc, hyp);
  double separation = sqrt(inputImage->width*inputImage->width + inputImage->height*inputImage->height) / nbins;
  double scale = 1. / (double) (resolution * resolution);

  double pixVal, projVal, bini;
  double xProj, yProj;
  int x, y, m, n;

  for (int anglei=0; anglei < nangles; ++anglei) {
    xProj = -sin(angles[anglei]);
    yProj = cos(angles[anglei]);

    // fprintf(stderr, "\n[LOADING] Angle: %lf", angles[anglei]);

    for (int pixi=0; pixi < inputImage->width * inputImage->height; ++pixi) {

      pixVal = inputImage->data[pixi * inputImage->numColorChannels + color];
      if (!pixVal) continue;

      x = pixi % inputImage->width;
      y = pixi / inputImage->width;

      // if (pixi == 204906) fprintf(stderr, "\n[DEBUG] Pixel %d %d %lf: ", x, y, pixVal);

      for (int subpixi=0; subpixi < resolution * resolution; ++subpixi) {
        m = subpixi % resolution;
        n = subpixi / resolution;

        projVal = (
          (x - xc + (((double)m + 0.5) / resolution)) * xProj +
          (y - yc + (((double)n + 0.5) / resolution)) * yProj
        );

        projVal = modf( (projVal + hyp/2) / separation, &bini );

        // if (pixi == 204906) fprintf(stderr, "[%lf %lf] ", projVal, bini);

        transformImage->data[anglei * nbins + (int) bini + 1] += (pixVal * projVal) * scale;
        transformImage->data[anglei * nbins + (int) bini] += pixVal * (1 - projVal) * scale;
      }
    }
  }

  return transformImage;
}
