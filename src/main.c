#include "options.h"
#include "images.h"
#include "filters.h"

#include <stdio.h>
#include <math.h>

int main()
{
  #if DEBUG
    fprintf(stderr, "[INFO] Started Program successfully!\n");
  #endif

  /* HYPERPARAMS */
  const char* imageFile = "../images/circle.jpg";
  int totalNails = 2000;
  /* END HYPERPARAMS */

  RawImage* imageData = LoadJpegFromPath(imageFile); /* Path is relative to directory program runs from */
  if (!imageData) {
    #if DEBUG
      fprintf(stderr, "[WARNING] Could not read image!\n");
    #endif
    return 1;
  }

  // Compute frame with equal spacing with
  // totalNails nails in total around it
  totalNails += totalNails % 2;
  double origRatio = (float) imageData->width / imageData->height;
  int n = (int) floor( (origRatio / (origRatio + 1.)) * (totalNails / 2.) );
  double newRatio;
  if (fabs( (2. * n) / (totalNails - 2. * n) - origRatio ) < fabs(  (2. * n + 2.) / (totalNails - 2. * n - 2.) - origRatio  )) {
    newRatio = (2. * n) / (totalNails - 2. * n);
  } else {
    newRatio = (2. * n + 2.) / (totalNails - 2. * n - 2.);
  }

  unsigned int baseNails = (int) n + 1;
  unsigned int heightNails = (int) (totalNails / 2) - n + 1;

  unsigned long int newWidth, newHeight;
  if (newRatio > origRatio) {
    newWidth = imageData->width;
    newHeight = lround( imageData->width / newRatio );
  } else {
    newHeight = imageData->height;
    newWidth = lround( imageData->height * newRatio );
  }
  double seperation = (double) newWidth / n;

  #if DEBUG
    fprintf(stderr, "[INFO] OrigRatio=%lf, NewRatio=%lf, NewDims=(%lu x %lu), baseNails=%u heightNails=%u, seperation=%lf\n",
            origRatio, newRatio, newWidth, newHeight, baseNails, heightNails, seperation);
  #endif

  if (newWidth > imageData->width || newHeight > imageData->height) {
    #if DEBUG
      fprintf(stderr, "[ERROR] <%s:%u> Invalid image ratio!\n", __FILE__, __LINE__);
    #endif
    return 1;
  }
  // End frame size calculation

  // Crop image to calculated frame size
  RawImage* image = LoadRawImage(newWidth, newHeight, imageData->numColorChannels);
  if (!image) {
    #if DEBUG
      fprintf(stderr, "[WARNING] <%s:%u> Could not create cropped image!\n", __FILE__, __LINE__);
    #endif
    return 1;
  }

  int rowStart = floor((double)(imageData->height - newHeight) / 2.);
  int colStart = floor((double)(imageData->width - newWidth) / 2.);
  for (int rowi=rowStart; rowi <= rowStart + newHeight; ++rowi ) {
    for (int coli=colStart; coli <= colStart + newWidth; ++coli ) {
      for (int color=0; color<imageData->numColorChannels; ++color) {
        image->data[((rowi - rowStart) * newWidth + coli - colStart) * imageData->numColorChannels + color] =
          imageData->data[((rowi * imageData->width) + coli) * imageData->numColorChannels + color];
      }
    }
  }

  // End Center Cropping

  // Start inversion of Image into CMYK and W
  NormImage* invImage = RgbToPrintable(image);
  if (!invImage) {
    #if DEBUG
      fprintf(stderr, "[ERROR] <%s:%u> Failed to convert image into printable colors CMYK!\n", __FILE__, __LINE__);
    #endif
    return 1;
  }

  // for (int j=0; j<invImage->height; ++j) {
  //   for (int i=0; i<image->width; ++i) {
  //     fprintf(stdout, "%lf ", invImage->data[(i + j * invImage->width) * invImage->numColorChannels + 4]);
  //   }
  //   fprintf(stdout, "\n");
  // }
  //
  // NormImage* invImage = NormalizeRawImage(image);
  // if (!invImage) {
  //   #if DEBUG
  //     fprintf(stderr, "[ERROR] <%s:%u> Failed to Normalize image!\n", __FILE__, __LINE__);
  //   #endif
  //   return 1;
  // }

  // Unload Cropped Image
  UnloadRawImage(image);

  // Create Radon Transform
  double angles[180] = {0};
  for (int i=0; i<180; ++i) angles[i] = i * 3.14 / 180;

  #if DEBUG
    fprintf(stderr, "[INFO] Taking radon transform.\n");
  #endif
  NormImage* radonImage = RadonTransform(invImage, angles, 180, 500, 4, 1);

  // End inversion into CMYK and W
  UnloadNormImage(invImage);

  #if DEBUG
    fprintf(stderr, "[INFO] Writing to stdout.\n");
  #endif
  for (int i=0; i<180; ++i) {
    for (int j=0; j<500; ++j) {
      printf("%lf ", radonImage->data[i * 500 + j]);
    }
    printf("\n");
  }

  UnloadNormImage(radonImage);

  #if DEBUG
    fprintf(stderr, "[INFO] Done!\n");
  #endif
  return 0;
}
