// includes, system
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

// includes, project
#include "cufft.h"
#include "globals.h"
#include "gpu_helpers.h"

__device__ unsigned long computeHash_CUDA(int f1, int f2, int f3, int f4){
    
      const unsigned long FUZ_FACTOR = 3;

      if((f1 == -1 || f2==-1) || (f3==-1 || f4==-1)) return 0; //We return zero as the value of a hash in case of a silence
      unsigned long result = 0;
      result += (unsigned long) ((f4-(f4%FUZ_FACTOR)) * 100000000);
      result += (unsigned long) ((f3-(f3%FUZ_FACTOR)) * 100000);
      result += (unsigned long) ((f2-(f2%FUZ_FACTOR)) * 100);
      result += (unsigned long) (f1-(f1%FUZ_FACTOR));
      return result;
}

__global__ void tranformToHashes_CUDA(Complex * input, int numChunks, unsigned long * outputHashes){
    
    int FREQ_RANGES[5] = { 40, 80, 120, 180, 300 };

    int i = threadIdx.x + blockIdx.x*blockDim.x; //My chunk

    int firstBin = (CHUNK_SECONDS * FREQ_LOWERLIMIT)-2;
    int lastBin = (CHUNK_SECONDS  * FREQ_UPPERLIMIT)+2;
    int f1, f2, f3, f4;
    float f1val, f2val, f3val, f4val;

    f1 = -1; f2 = -1; f3 = -1 ; f4 = -1;
    f1val = 0; f2val = 0; f3val = 0 ; f4val = 0;



    if( i < numChunks)
    {
        for(int k=firstBin; k<lastBin; k++)
        {

          Complex bin = input[k+(CHUNK_SAMPLES*i)];

          int binFreq =  ((float) SAMPLE_RATE / (float) CHUNK_SAMPLES) * k;
          float binMag = sqrt( bin.re * bin.re  + bin.im * bin.im); 

          if((binFreq >= FREQ_RANGES[0] && binFreq < FREQ_RANGES[1]) && binMag > f1val ) { f1 = binFreq; f1val = binMag; }
          if((binFreq >= FREQ_RANGES[1] && binFreq < FREQ_RANGES[2]) && binMag > f2val ) { f2 = binFreq; f2val = binMag; }
          if((binFreq >= FREQ_RANGES[2] && binFreq < FREQ_RANGES[3]) && binMag > f3val ) { f3 = binFreq; f3val = binMag; }
          if((binFreq >= FREQ_RANGES[3] && binFreq < FREQ_RANGES[4]) && binMag > f4val ) { f4 = binFreq; f4val = binMag; }
        }

        unsigned long hash = computeHash_CUDA(f1, f2, f3, f4);

        //STEP 3: Hash all of the top 4 frequencies
        (outputHashes)[i] = computeHash_CUDA(f1, f2, f3, f4);

    }
}

void audioToHashes_CUDA(Complex * input, int numChunks, unsigned long ** outputHashes){

    *outputHashes = (unsigned long *)malloc(sizeof(unsigned long) * numChunks);

    // Step 1: Allocate and copy input audio to GPU
    Complex *d_input;
    cudaMalloc((void **)&d_input, sizeof(Complex) * numChunks * CHUNK_SAMPLES);
    cudaMemcpy(d_input, input, sizeof(Complex) * numChunks * CHUNK_SAMPLES, cudaMemcpyHostToDevice);

    // Step 1.5: Perform batched FFTs
    cufftHandle plan;
    int rank = 1;                          // 1D FFT
    int n[1] = {CHUNK_SAMPLES};           // FFT size
    int istride = 1, ostride = 1;
    int idist = CHUNK_SAMPLES, odist = CHUNK_SAMPLES;
    int inembed[] = {0}, onembed[] = {0}; // Auto
    int batch = numChunks;

    cufftPlanMany(&plan, rank, n,
                  inembed, istride, idist,
                  onembed, ostride, odist,
                  CUFFT_C2C, batch);

    cufftExecC2C(plan, (cufftComplex *)d_input, (cufftComplex *)d_input, CUFFT_FORWARD);
    cufftDestroy(plan);

    // Step 2: Launch CUDA kernel to hash each chunk
    unsigned long *d_hashes;
    cudaMalloc((void **)&d_hashes, sizeof(unsigned long) * numChunks);

    int numBlocks = (numChunks + BLOCK_SIZE - 1) / BLOCK_SIZE;
    dim3 dim_grid(numBlocks, 1, 1);
    dim3 dim_block(BLOCK_SIZE, 1, 1);

    tranformToHashes_CUDA<<<dim_grid, dim_block>>>(d_input, numChunks, d_hashes);
    cudaDeviceSynchronize();

    // Step 3: Copy back to host
    cudaMemcpy(*outputHashes, d_hashes, sizeof(unsigned long) * numChunks, cudaMemcpyDeviceToHost);

    // Optional cleanup
    cudaFree(d_input);
    cudaFree(d_hashes);
}