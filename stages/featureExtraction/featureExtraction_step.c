/**
 * featureExtraction_step.c: this file is part of the ALNSB project.
 *
 * ALNSB: the Adaptive Lung Nodule Screening Benchmark
 *
 * Copyright (C) 2014,2015 University of California Los Angeles
 *
 * This program can be redistributed and/or modified under the terms
 * of the license specified in the LICENSE.txt file at the root of the
 * project.
 *
 * Contact: Alex Bui <buia@mii.ucla.edu>
 *
 */
/**
 * Written by: Shiwen Shen, Prashant Rawat, Louis-Noel Pouchet and William Hsu
 *
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include <stdint.h>

#include <stages/featureExtraction/featureExtraction_step.h>
#include <toolbox/bwconncomp.h>
#include <toolbox/imPerimeter.h>
#include <toolbox/imSurface.h>
#include <toolbox/imMeanBreadth.h>
#include <toolbox/imEuler3d.h>
#include <toolbox/stdev.h>
#include <toolbox/skewness.h>
#include <toolbox/kurtosis.h>




#ifndef M_PI
#define M_PI 3.14159265358979323846264338327
#endif

#define T 64
#define PI M_PI
#ifdef min
# undef min
#endif
#ifdef max
# undef max
#endif
#define min(a,b) (a < b ? a : b)
#define max(a,b) (a > b ? a : b)

static
void GeometricFeature2D (ALNSB_IMAGE_TYPE_REAL *featureResult, ALNSB_IMAGE_TYPE_BIN *bina2D_in, int dim0, int dim1, int min_rowIn, int max_rowIn, int min_colIn, int max_colIn, float *xyzSpace, int dimOffset) {
   int i, j;
   int area = 0;
   float d1 = xyzSpace[0];
   float d2 = xyzSpace[1];
   float f1, f2, f3, f4, xLength, yLength;
   ALNSB_IMAGE_TYPE_BIN (*bina2D)[dim1] =
     (ALNSB_IMAGE_TYPE_BIN (*)[dim1])bina2D_in;

#pragma omp parallel for private(j) reduction(+:area)
   for (i=0; i<dim0; i++) {
      for (j=0; j<dim1; j++) {
         if (abs (bina2D[i][j]) > 0)
            area++;
      }
   }
   f1 = area * d1 * d2;
   xLength = (max_colIn - min_colIn + 1) * d1;
   yLength = (max_rowIn - min_rowIn + 1) * d2;
   f2 = max (xLength, yLength);
   f3 = alnsb_imPerimeter_bin2d (bina2D_in, dim0, dim1);
   f3 = f3 * d1;
   f4 = 4 * PI * f1 / pow (f3,2);
   featureResult[dimOffset+0] = f1;
   featureResult[dimOffset+1] = f2;
   featureResult[dimOffset+2] = f3;
   featureResult[dimOffset+3] = f4;
}

static
void GeometricFeature3D (ALNSB_IMAGE_TYPE_REAL *featureResult, int *rowIn, int *colIn, int *zIn, int dim0, int dim1, int dim2, int min_rowIn, int max_rowIn, int min_colIn, int max_colIn, int min_zIn, int max_zIn, int midZ, int numPix, float *xyzSpace, int dimOffset,
                         ALNSB_IMAGE_TYPE_BIN *tempNoduleMask_in_box, int dim0_box, int dim1_box, int dim2_box, int midZ_new) {
   int i, j, k;
   float d1 = xyzSpace[0];
   float d2 = xyzSpace[1];
   float d3 = xyzSpace[2];
   float f5, f6, f7, f8, f9, f10, f11, f12, xLength, yLength, zLength;
   float maxl, minl, radius, rootMeanSqDis = 0;
   int areaT = 0, centerX, centerY, centerZ;
   float perimeterT, surfaceArea;
   ALNSB_IMAGE_TYPE_BIN *maskTem_in = (ALNSB_IMAGE_TYPE_BIN *) malloc (dim1*dim2*sizeof(ALNSB_IMAGE_TYPE_BIN));
   ALNSB_IMAGE_TYPE_BIN (*maskTem)[dim2] = (ALNSB_IMAGE_TYPE_BIN (*)[dim2])maskTem_in;

   f5 = numPix * d1 * d2 * d3;
   xLength = (max_colIn - min_colIn + 1) * d1;
   yLength = (max_rowIn - min_rowIn + 1) * d2;
   zLength = (max_zIn - min_zIn + 1) * d3;

   maxl = max (xLength, yLength);
   maxl = max (maxl, zLength);
   radius =  maxl * 0.5;

   centerX = round ((max_rowIn + min_rowIn+2) * 0.5);
   centerY = round ((max_colIn + min_colIn+2) * 0.5);
   centerZ = midZ+1;

   for (i=0; i < numPix; i++) {
      float dis = pow((rowIn[i]+1-centerX)*d1, 2) + pow((colIn[i]+1-centerY)*d2, 2) + pow((zIn[i]+1-centerZ)*d3, 2);
      rootMeanSqDis = rootMeanSqDis + dis;
   }

   rootMeanSqDis = rootMeanSqDis / numPix;
   rootMeanSqDis = sqrt (rootMeanSqDis);

   f6 = radius / rootMeanSqDis;

   f7 = min (xLength, yLength) / max (xLength, yLength);

   minl = min (xLength, yLength);
   minl = min (minl, zLength);

   f8 = minl / maxl;

    surfaceArea = alnsb_imSurface_bin3d (tempNoduleMask_in_box, xyzSpace, dim0_box, dim1_box, dim2_box);
   f9 = pow (surfaceArea, 3) / (pow (f5,2) * 36 * PI);
    
    f10 = alnsb_imMeanBreadth_bin3d (tempNoduleMask_in_box, xyzSpace, dim0_box, dim1_box, dim2_box);
    f11 = alnsb_imEuler3d_bin3d (tempNoduleMask_in_box, dim0_box, dim1_box, dim2_box);

   for (i=0; i<dim1; i++)
     for (j=0; j<dim2; j++)
        maskTem[i][j] = 0;

#pragma omp parallel for private(j,k)
   for (i=0; i<numPix; i++) {
      j = rowIn[i];
      k = colIn[i];
      maskTem[j][k] = 1;
   }

#pragma omp parallel for private(j) reduction(+:areaT)
   for (i=0; i<dim1; i++) {
      for (j=0; j<dim2; j++) {
	 if (abs (maskTem[i][j]) > 0) {
	    areaT++;
         }
      }
   }

   perimeterT = alnsb_imPerimeter_bin2d (maskTem_in, dim1, dim2);
   f12 = 4 * PI * areaT / pow (perimeterT, 2);

   featureResult[dimOffset+4] = f5;
   featureResult[dimOffset+5] = f6;
   featureResult[dimOffset+6] = f7;
   featureResult[dimOffset+7] = f8;
   featureResult[dimOffset+8] = f9;
   featureResult[dimOffset+9] = f10;
   featureResult[dimOffset+10] = f11;
   featureResult[dimOffset+11] = f12;
   free (maskTem_in);
}

static
void intensityFeature2D (ALNSB_IMAGE_TYPE_REAL *featureResult, ALNSB_IMAGE_TYPE_REAL *volume_image_in_full, ALNSB_IMAGE_TYPE_BIN *bina2D_in, int dim0, int dim1, int dim2, int midZ, int min_rowIn, int max_rowIn, int min_colIn, int max_colIn, int dimOffset) {
   int i, j, k;
   ALNSB_IMAGE_TYPE_REAL (*volume_image_full)[dim1][dim2] = (ALNSB_IMAGE_TYPE_REAL (*)[dim1][dim2])volume_image_in_full;
   ALNSB_IMAGE_TYPE_BIN (*bina2D)[dim2] = (ALNSB_IMAGE_TYPE_BIN (*)[dim2])bina2D_in;
   float meanInside = 0, meanOut = 0;
   float f13 = DBL_MAX, f14, f15, f16, f17, f18, f19, f20, f21, f22;
   float m00 = 0, m01 = 0, m10 = 0, m11 = 0, m12 = 0;
   int m = 0, rowUp = dim1, rowDown = 0, colLef = dim2, colRig = 0;
   int vec_sz = 0, r_sz = 0, outBoundingSize = 5;
   int *rowT = (int *) malloc (dim1*dim2*sizeof (int));
   int *colT = (int *) malloc (dim1*dim2*sizeof (int));
   ALNSB_IMAGE_TYPE_REAL *binVec = (ALNSB_IMAGE_TYPE_REAL *) malloc (dim1*dim2*sizeof (ALNSB_IMAGE_TYPE_REAL));
   ALNSB_IMAGE_TYPE_BIN *bina2DOut_in = (ALNSB_IMAGE_TYPE_BIN *) malloc (dim1*dim2*sizeof (ALNSB_IMAGE_TYPE_BIN));
   ALNSB_IMAGE_TYPE_BIN (*bina2DOut)[dim2] = (ALNSB_IMAGE_TYPE_BIN (*)[dim2])bina2DOut_in;

   for (i=0; i<dim1; i++) {
      for (j=0; j<dim2; j++) {
         if (bina2D[i][j] != 0) {
	    binVec[vec_sz] = volume_image_full[midZ][i][j];
	    vec_sz++;
            rowUp = min (rowUp, i);
            rowDown = max (rowDown, i);
            colLef = min (colLef, j);
            colRig = max (colRig, j);
         }
      }
   }
   rowUp = rowUp - outBoundingSize;
   rowDown = rowDown + outBoundingSize;
   colLef = colLef - outBoundingSize;
   colRig = colRig + outBoundingSize;

   for (i=0; i<vec_sz; i++) {
       meanInside += binVec[i];
       f13 = min (f13, binVec[i]);
   }

   f15 = alnsb_stdev_real1d (binVec, vec_sz);
   f16 = alnsb_skewness_real1d (binVec, vec_sz);
   f17 = alnsb_kurtosis_real1d (binVec, vec_sz);

   free (binVec);

   meanInside = meanInside / vec_sz;

   for (i=0; i<dim1; i++)
      for (j=0; j<dim2; j++)
	 bina2DOut[i][j] = 0;

#pragma omp parallel for private(j)
   for (i=rowUp; i<=rowDown; i++)
      for (j=colLef; j<=colRig; j++)
	  bina2DOut[i][j] = 1;

#pragma omp parallel for private(j)
   for (i=0; i<dim1; i++)
      for (j=0; j<dim2; j++)
	bina2DOut[i][j] = (bina2DOut[i][j] != 0) && (abs(bina2D[i][j] - 1) != 0);

#pragma omp parallel for private(j) reduction(+:m,meanOut)
   for (i=0; i<dim1; i++) {
      for (j=0; j<dim2; j++) {
         if (bina2DOut[i][j] == 1) {
	    m++;
            meanOut += volume_image_full[midZ][i][j];
         }
      }
   }
   meanOut = meanOut / m;
   f14 = (meanInside-meanOut) / (meanInside+meanOut);

   free (bina2DOut_in);

   for (i=0; i<dim1; i++) {
      for (j=0; j<dim2; j++) {
	 if (bina2D[i][j] != 0) {
	     rowT[r_sz] = i;
	     colT[r_sz] = j;
	     r_sz++;
         }
      }
   }

   for (i=0; i<r_sz; i++) {
      int u = rowT[i];
      int x = u + 1;
      for (j=0; j<r_sz; j++) {
	  int v = colT[j];
	  int y = v + 1;
	  m00 = m00 + volume_image_full[midZ][u][v];
          m01 = m01 + (y * volume_image_full[midZ][u][v]);
	  m10 = m10 + (x * volume_image_full[midZ][u][v]);
          m11 = m11 + (x * y * volume_image_full[midZ][u][v]);
          m12 = m12 + (x * y * y * volume_image_full[midZ][u][v]);
      }
   }

   free (rowT);
   free (colT);

   f18=m01/m00;
   f19=m10/m00;
   f20=m11/m00;
   f21=m12/m00;
   f22=m00;

   featureResult[dimOffset+12] = f13;
   featureResult[dimOffset+13] = f14;
   featureResult[dimOffset+14] = f15;
   featureResult[dimOffset+15] = f16;
   featureResult[dimOffset+16] = f17;
   featureResult[dimOffset+17] = f18;
   featureResult[dimOffset+18] = f19;
   featureResult[dimOffset+19] = f20;
   featureResult[dimOffset+20] = f21;
   featureResult[dimOffset+21] = f22;
}

static
void intensityFeature3D (ALNSB_IMAGE_TYPE_REAL *featureResult, ALNSB_IMAGE_TYPE_REAL *volume_image_in_full, int dim0, int dim1, int dim2, int min_rowIn, int max_rowIn, int min_colIn, int max_colIn, int min_zIn, int max_zIn, int dimOffset,
                         ALNSB_IMAGE_TYPE_REAL *volume_image_in_box, ALNSB_IMAGE_TYPE_BIN *tempNoduleMask_in_box, int dim0_box, int dim1_box, int dim2_box) {
   int i, j, k;
   ALNSB_IMAGE_TYPE_REAL (*volume_image_full)[dim1][dim2] = (ALNSB_IMAGE_TYPE_REAL (*)[dim1][dim2])volume_image_in_full;
    ALNSB_IMAGE_TYPE_REAL (*volume_image_box)[dim1_box][dim2_box] = (ALNSB_IMAGE_TYPE_REAL (*)[dim1_box][dim2_box])volume_image_in_box;
    ALNSB_IMAGE_TYPE_BIN (*tempNoduleMask_box)[dim1_box][dim2_box] = (ALNSB_IMAGE_TYPE_BIN (*)[dim1_box][dim2_box])tempNoduleMask_in_box;
   ALNSB_IMAGE_TYPE_BIN *bina3DOut_in = (ALNSB_IMAGE_TYPE_BIN *) malloc (dim0*dim1*dim2*sizeof (ALNSB_IMAGE_TYPE_BIN));
   ALNSB_IMAGE_TYPE_BIN (*bina3DOut)[dim1][dim2] = (ALNSB_IMAGE_TYPE_BIN (*)[dim1][dim2])bina3DOut_in;
    int outBoundingSize = 5;
    
   // Vastly oversized: assume the largest object is of the size of
   // the 3D image...
   ALNSB_IMAGE_TYPE_REAL *volVec = (ALNSB_IMAGE_TYPE_REAL *) malloc (dim0*dim1*dim2*sizeof (ALNSB_IMAGE_TYPE_REAL));
   float meanInside = 0, meanOut = 0;
   float f23= DBL_MAX, f24, f25, f26, f27;
   int rowUp, rowDown, colLef, colRig, zFr, zBeh;
    int vec_sz = 0, bv_sz = 0;
    
   for (i=0; i<dim0_box; i++) {
      for (j=0; j<dim1_box; j++) {
         for (k=0; k<dim2_box; k++) {
             if (tempNoduleMask_box[i][j][k] == 1) {
	          volVec[vec_sz] = volume_image_box[i][j][k];
		  meanInside += volume_image_box[i][j][k];
		  f23 = min (f23, volume_image_box[i][j][k]);
	          vec_sz++;
             }
         }
      }
   }
   meanInside = meanInside / vec_sz;

   rowUp = min_rowIn - outBoundingSize;
   rowDown = max_rowIn + outBoundingSize;
   colLef = min_colIn - outBoundingSize;
   colRig = max_colIn + outBoundingSize;
   zFr = min_zIn - outBoundingSize;
   zBeh = max_zIn + outBoundingSize;

   for (i=0; i<dim0; i++)
      for (j=0; j<dim1; j++)
	for (k=0; k<dim2; k++)
	    bina3DOut[i][j][k] = 0;

#pragma omp parallel for private(j)
   for (i=rowUp; i<=rowDown; i++)
      for (j=colLef; j<=colRig; j++)
	  bina3DOut[0][i][j] = 1;

#pragma omp parallel for private(j,k) reduction(+:meanOut,bv_sz)
   for (i=0; i<dim0; i++) {
      for (j=0; j<dim1; j++) {
         for (k=0; k<dim2; k++) {
             if (bina3DOut[i][j][k] == 1) {
	         meanOut = meanOut + volume_image_full[i][j][k];
	         bv_sz++;
             }
         }
      }
   }
   meanOut = meanOut / bv_sz;
    
   f24 = (meanInside-meanOut) / (meanInside+meanOut);

   f25 = alnsb_stdev_real1d (volVec, vec_sz);
   f26 = alnsb_skewness_real1d (volVec, vec_sz);
   f27 = alnsb_kurtosis_real1d (volVec, vec_sz);

   featureResult[dimOffset+22] = f23;
   featureResult[dimOffset+23] = f24;
   featureResult[dimOffset+24] = f25;
   featureResult[dimOffset+25] = f26;
   featureResult[dimOffset+26] = f27;

   free (bina3DOut_in);
   free (volVec);
}

static
void featureExtractionCandidate (ALNSB_IMAGE_TYPE_REAL *featureResult,
				 int *objectPosition, int *dimArray,
				 ALNSB_IMAGE_TYPE_REAL *volume_image_in_full,
				 float *xyzSpace, int numNodule,
				 int dim0, int dim1, int dim2)
{
   int i, j, k;
   ALNSB_IMAGE_TYPE_REAL (*volume_image_full)[dim1][dim2] = (ALNSB_IMAGE_TYPE_REAL (*)[dim1][dim2])volume_image_in_full;
   float meanValue = 0, stdValue;
   int vol = dim0*dim1*dim2, ctr = 0, objOffset = 0;
   ALNSB_IMAGE_TYPE_REAL *stdVec = (ALNSB_IMAGE_TYPE_REAL *) malloc (dim0*dim1*dim2*sizeof (ALNSB_IMAGE_TYPE_REAL));
   ALNSB_IMAGE_TYPE_BIN *bina2D_in = (ALNSB_IMAGE_TYPE_BIN *) malloc(dim1*dim2*sizeof (ALNSB_IMAGE_TYPE_BIN));
   ALNSB_IMAGE_TYPE_BIN (*bina2D)[dim2] = (ALNSB_IMAGE_TYPE_BIN (*)[dim2])bina2D_in;
   
    /* Compute mean and std */
   for (i=0; i<dim0; i++) {
      for (j=0; j<dim1; j++) {
	 for (k=0; k<dim2; k++) {
             stdVec[ctr] = volume_image_full[i][j][k];
	     meanValue += volume_image_full[i][j][k];
	     ctr++;
          }
      }
   }
   meanValue = meanValue / (float)vol;
   stdValue = alnsb_stdev_real1d (stdVec, vol);
   free (stdVec);

#pragma omp parallel for private(j,k)
   for (i=0; i<dim0; i++)
      for (j=0; j<dim1; j++)
	 for (k=0; k<dim2; k++)
	   volume_image_full[i][j][k] = (volume_image_full[i][j][k] - meanValue) / stdValue;

   for (i=0; i<numNodule; i++) {
        int l, midZ = 0;
        int min_rowIn = dim1, max_rowIn = 0;
        int min_colIn = dim2, max_colIn = 0;
        int min_zIn = dim0, max_zIn = 0;
        int min_rowBIn = dim1, max_rowBIn = 0;
	int min_colBIn = dim2, max_colBIn = 0;
	int dimOp = dimArray[i];
   	int *rowIn = (int *) malloc (dimOp*sizeof (int));
	int *colIn = (int *) malloc (dimOp*sizeof (int));
	int *zIn = (int *) malloc (dimOp*sizeof (int));
	int dimOffset = i*27, oft = 0;

        for (j=objOffset; j<objOffset+dimOp; j++) {
           int r,c,h;
           int pos = objectPosition[j];
           h = pos / (dim1*dim2);
           pos = pos % (dim1*dim2);
           c = pos / dim1;
           r = pos % dim1;
           rowIn[oft] = r;
           colIn[oft] = c;
           zIn[oft] = h;
	   oft++;
        }
	objOffset = objOffset + dimOp;

        for (j=0; j<dimOp; j++) {
           int r, c, h;
	   r = rowIn[j];
           c = colIn[j];
	   h = zIn[j];
           min_rowIn = min (min_rowIn, r);
	   max_rowIn = max (max_rowIn, r);
	   min_colIn = min (min_colIn, c);
	   max_colIn = max (max_colIn, c);
           min_zIn = min (min_zIn, h);
	   max_zIn = max (max_zIn, h);
        }
        midZ = round((max_zIn + min_zIn)*0.5);
       
       ///1 pixel margin around the box
       int dim0_box = (max_zIn - min_zIn + 1 +2);
       int dim1_box = (max_rowIn - min_rowIn + 1 +2);
       int dim2_box = (max_colIn - min_colIn + 1 +2);
       int midZ_new = round((max_zIn - min_zIn)*0.5) +1;
       ALNSB_IMAGE_TYPE_BIN *tempNoduleMask_in_box = (ALNSB_IMAGE_TYPE_BIN *) malloc (dim0_box*dim1_box*dim2_box*sizeof(ALNSB_IMAGE_TYPE_BIN));
       ALNSB_IMAGE_TYPE_BIN (*tempNoduleMask_box)[dim1_box][dim2_box] = (ALNSB_IMAGE_TYPE_BIN (*)[dim1_box][dim2_box])tempNoduleMask_in_box;
       
       for (j=0; j<dim0_box; j++)
           for (k=0; k<dim1_box; k++)
               for (l=0; l<dim2_box; l++)
                   tempNoduleMask_box[j][k][l] = 0;
       
       for (j = 0; j < oft; j++)
           tempNoduleMask_box[zIn[j] - min_zIn +1][rowIn[j] - min_rowIn +1][colIn[j] - min_colIn +1] = 1;
       
       ALNSB_IMAGE_TYPE_REAL *volume_image_in_box = (ALNSB_IMAGE_TYPE_REAL *) malloc (dim0_box*dim1_box*dim2_box*sizeof (ALNSB_IMAGE_TYPE_REAL));
       ALNSB_IMAGE_TYPE_REAL (*volume_image_box)[dim1_box][dim2_box] = (ALNSB_IMAGE_TYPE_REAL (*)[dim1_box][dim2_box])volume_image_in_box;
       for (j=0; j<dim0_box; j++)
           for (k=0; k<dim1_box; k++)
               for (l=0; l<dim2_box; l++)
                   volume_image_box[j][k][l] = volume_image_full[j + min_zIn -1][k + min_rowIn -1][l + min_colIn -1];

        for (j=0; j<dim1; j++) {
	  for (k=0; k<dim2; k++) {
          if(j >= min_rowIn && j <= max_rowIn && k >= min_colIn && k <= max_colIn)
          bina2D[j][k] = tempNoduleMask_box[midZ_new][j - min_rowIn +1][k - min_colIn +1];
          else
              bina2D[j][k] = 0;
	     if (bina2D[j][k] != 0) {
		max_rowBIn = max (max_rowBIn, j);
		min_rowBIn = min (min_rowBIn, j);
		max_colBIn = max (max_colBIn, k);
	        min_colBIn = min (min_colBIn, k);
	    }
          }
       }

	/// FIXME: LNP: new code added to select only the largest
	/// connected comp.
	/* bina2DCC=bwconncomp(bina2D); */
	/* numPixels = cellfun(@numel,bina2DCC.PixelIdxList); */
	/* [largest1,idx1] = max(numPixels); */
	/*  bina2D= bina2D&0; */
	/*  bina2D(bina2DCC.PixelIdxList{idx1}) = 1; */
	int nbc = 0;
	int** ccs = NULL;
	int* cz = NULL;

	alnsb_bwconncomp_bin (bina2D_in, 1, dim1, dim2,
			      &ccs, &cz, &nbc, NULL, 1);
	int m_sz = 0, m_id;
	for (j = 0; j < nbc; ++j)
	  if (cz[j] > m_sz)
	    {
	      m_sz = cz[j];
	      m_id = j;
	    }
        for (j=0; j<dim1; j++)
	  for (k=0; k<dim2; k++)
	    bina2D[j][k] = 0;
	ALNSB_IMAGE_TYPE_BIN* bina2Dflat = (ALNSB_IMAGE_TYPE_BIN*)bina2D;
       
#pragma omp parallel for
	for (j = 0; j < m_sz; ++j)
	  bina2Dflat[ccs[m_id][j]] = 1;
	free (cz);
       
#pragma omp parallel for
	for (j = 0; j < nbc; ++j)
	  free (ccs[j]);
	free (ccs);
	/// !LNP
      fprintf(stdout, "   %d nodule candidate\n",i);
        fprintf(stdout, "    here works before GeometricFeature2D\n");
       GeometricFeature2D (featureResult, bina2D_in, dim1, dim2, min_rowBIn, max_rowBIn, min_colBIn, max_colBIn, xyzSpace, dimOffset);
       fprintf(stdout, "    here works before GeometricFeature3D\n");
       GeometricFeature3D (featureResult, rowIn, colIn, zIn, dim0, dim1, dim2, min_rowIn, max_rowIn, min_colIn, max_colIn, min_zIn, max_zIn, midZ, dimOp, xyzSpace, dimOffset,
                           tempNoduleMask_in_box, dim0_box, dim1_box, dim2_box, midZ_new);
       fprintf(stdout, "    here works before intensityFeature2D\n");
       intensityFeature2D (featureResult, volume_image_in_full, bina2D_in, dim0, dim1, dim2, midZ, min_rowIn, max_rowIn, min_colIn, max_colIn, dimOffset);
       fprintf(stdout, "    here works before intensityFeature3D\n");
       intensityFeature3D (featureResult, volume_image_in_full, dim0, dim1, dim2, min_rowIn, max_rowIn, min_colIn, max_colIn, min_zIn, max_zIn, dimOffset,
                           volume_image_in_box, tempNoduleMask_in_box, dim0_box, dim1_box, dim2_box);
       
       free (rowIn);
       free (colIn);
       free (zIn);

   }
   free (bina2D_in);
}



void featureExtraction_cpu (s_alnsb_environment_t* __ALNSB_RESTRICT_PTR env,
			    image3DReal* __ALNSB_RESTRICT_PTR inputPrep,
			    image3DBin* __ALNSB_RESTRICT_PTR inputPresel,
			    image3DReal** __ALNSB_RESTRICT_PTR outputFeatures)
{
  // Need a duplicate of the inputPrep image as it is modified by
  // featureExtractionCandidate.
  image3DReal* tmpVol = (image3DReal*) image3D_duplicate (inputPrep->image3D);
  // 1D view of input data.
  ALNSB_IMBinTo1D(tmpVol, base_img);
  ALNSB_IMBinTo1D(inputPresel, in_img);

  // in_img -> noduleCandidateMask (result of preselection step).
  // base_img -> volume_image
  // out_img -> featureResult
  float xyzSpace[] = { env->scanner_pixel_spacing_x_mm,
		       env->scanner_pixel_spacing_y_mm,
		       env->scanner_slice_thickness_mm };

/* [xc,yc,zc] = size (candidateMsak); */
  int xc = inputPresel->rows; // candidateMask is an image of same
			      // size as 'in_img' and co.
  int yc = inputPresel->cols;
  int zc = inputPresel->slices;
  unsigned int sz = xc * yc * zc;


  int i, j;

  int** comp_coordinates = NULL;
  int* comp_sz = NULL;
  int nbcomp = 0;
  alnsb_bwconncomp_bin (in_img, zc, xc, yc,
			&comp_coordinates, &comp_sz, &nbcomp, NULL, 1);

  // Allocate output features image. We have 27 features, one per
  // component. It is a 2D image so its size is 1 x nbcomp x 27.
  *outputFeatures = image3DReal_alloc (1, nbcomp, 27);
  ALNSB_IMRealTo1D(*outputFeatures, out_img);

  unsigned int nb_pix_tot = 0;
    
#pragma omp parallel for reduction(+:nb_pix_tot)
  for (i = 0; i < nbcomp; ++i)
    nb_pix_tot += comp_sz[i];
  int* objectPosition = (int*) malloc (sizeof(int) * nb_pix_tot);
  int pos = 0;
  for (i = 0; i < nbcomp; ++i)
    for (j = 0; j < comp_sz[i]; ++j)
      objectPosition[pos++] = comp_coordinates[i][j];
  int* dimArray = (int*) malloc (sizeof(int) * nbcomp);
    
#pragma omp parallel for
  for (i = 0; i < nbcomp; ++i)
    dimArray[i] = comp_sz[i];
  free (comp_sz);
    
#pragma omp parallel for
  for (i = 0; i < nbcomp; ++i)
    free (comp_coordinates[i]);
  free (comp_coordinates);
  fprintf(stdout, "    here works before featureExtractionCandidate\n");
  featureExtractionCandidate (out_img, objectPosition, dimArray, base_img,
			      xyzSpace, nbcomp, zc, xc, yc);

  free (dimArray);
  free (objectPosition);
  image3D_free (tmpVol->image3D);
}
