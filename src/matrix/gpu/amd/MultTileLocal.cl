#define TILEX 4
#define TILEX_SHIFT 2
#define TILEY 4
#define TILEY_SHIFT 2

/* Matrix A is cached into local memory block */
/* Required global threads = (widthC / 4, heightC / 4) */
__kernel void MultTileLocal(__global T4 * matrixA,
							__global T4 * matrixB,
							__global T4 * matrixC,
							int widthA,
							__local T4 *blockA)

{
	int blockPos = get_local_id(0) + get_local_size(0) * (get_local_id(1) << TILEY_SHIFT); //Should be : localId * (TILEX / 4) (int4)

	/* Position of thread will be according to the number of values it writes i.e TILE size */
	int globalPos =  get_global_id(0) + (get_global_id(1) << TILEY_SHIFT) * get_global_size(0);

	/* Each thread writes 4 int4s */
	T4 sum0 = (T4)(0);
	T4 sum1 = (T4)(0);
	T4 sum2 = (T4)(0);
	T4 sum3 = (T4)(0);

	int temp = widthA / 4;

	/* This loop runs for number of blocks of A in horizontal direction */
	for(int i = 0; i < (temp / get_local_size(0)); i++)
	{
		/* Calculate global ids of threads from the particular block to load from matrix A depending on i */
		int globalPosA = i * get_local_size(0) + get_local_id(0) + (get_global_id(1) << TILEY_SHIFT) * temp;

		/* Load values in blockA from matrixA */
		blockA[blockPos] =							matrixA[globalPosA];
		blockA[blockPos + get_local_size(0)] =		matrixA[globalPosA + temp];
		blockA[blockPos + 2 * get_local_size(0)] =	matrixA[globalPosA + 2 * temp];
		blockA[blockPos + 3 * get_local_size(0)] =	matrixA[globalPosA + 3 * temp];

		barrier(CLK_LOCAL_MEM_FENCE);

		/* Calculate global ids of threads from the particular block to load from matrix B depending on i */
		int globalPosB = get_global_id(0) + ((i * get_local_size(0)) << TILEY_SHIFT) * get_global_size(0);

		/* This loop runs for number of threads in horizontal direction in the block of A */
		for(int j = 0; j < get_local_size(0) * 4; j=j+4)
		{
			/* Load 4 int4s from blockA : access patters = strided from local memory */
			T4 tempA0 = blockA[(j >> 2) + get_local_id(1) * TILEY * get_local_size(0)];
			T4 tempA1 = blockA[(j >> 2) + (get_local_id(1) * TILEY + 1) * get_local_size(0)];
			T4 tempA2 = blockA[(j >> 2) + (get_local_id(1) * TILEY + 2) * get_local_size(0)];
			T4 tempA3 = blockA[(j >> 2) + (get_local_id(1) * TILEY + 3) * get_local_size(0)];

			/* Load corresponding values from matrixB, access pattern = linear from global memory */
			T4 tempB0 = matrixB[globalPosB  + j *  get_global_size(0)]; //Should be localId.x * (TILEX / 4)
			T4 tempB1 = matrixB[globalPosB  + (j + 1) * get_global_size(0)];
			T4 tempB2 = matrixB[globalPosB  + (j + 2) * get_global_size(0)];
			T4 tempB3 = matrixB[globalPosB  + (j + 3) * get_global_size(0)];

			sum0.x += tempA0.x * tempB0.x + tempA0.y * tempB1.x + tempA0.z * tempB2.x + tempA0.w * tempB3.x;
			sum0.y += tempA0.x * tempB0.y + tempA0.y * tempB1.y + tempA0.z * tempB2.y + tempA0.w * tempB3.y;
			sum0.z += tempA0.x * tempB0.z + tempA0.y * tempB1.z + tempA0.z * tempB2.z + tempA0.w * tempB3.z;
			sum0.w += tempA0.x * tempB0.w + tempA0.y * tempB1.w + tempA0.z * tempB2.w + tempA0.w * tempB3.w;

			sum1.x += tempA1.x * tempB0.x + tempA1.y * tempB1.x + tempA1.z * tempB2.x + tempA1.w * tempB3.x;
			sum1.y += tempA1.x * tempB0.y + tempA1.y * tempB1.y + tempA1.z * tempB2.y + tempA1.w * tempB3.y;
			sum1.z += tempA1.x * tempB0.z + tempA1.y * tempB1.z + tempA1.z * tempB2.z + tempA1.w * tempB3.z;
			sum1.w += tempA1.x * tempB0.w + tempA1.y * tempB1.w + tempA1.z * tempB2.w + tempA1.w * tempB3.w;

			sum2.x += tempA2.x * tempB0.x + tempA2.y * tempB1.x + tempA2.z * tempB2.x + tempA2.w * tempB3.x;
			sum2.y += tempA2.x * tempB0.y + tempA2.y * tempB1.y + tempA2.z * tempB2.y + tempA2.w * tempB3.y;
			sum2.z += tempA2.x * tempB0.z + tempA2.y * tempB1.z + tempA2.z * tempB2.z + tempA2.w * tempB3.z;
			sum2.w += tempA2.x * tempB0.w + tempA2.y * tempB1.w + tempA2.z * tempB2.w + tempA2.w * tempB3.w;

			sum3.x += tempA3.x * tempB0.x + tempA3.y * tempB1.x + tempA3.z * tempB2.x + tempA3.w * tempB3.x;
			sum3.y += tempA3.x * tempB0.y + tempA3.y * tempB1.y + tempA3.z * tempB2.y + tempA3.w * tempB3.y;
			sum3.z += tempA3.x * tempB0.z + tempA3.y * tempB1.z + tempA3.z * tempB2.z + tempA3.w * tempB3.z;
			sum3.w += tempA3.x * tempB0.w + tempA3.y * tempB1.w + tempA3.z * tempB2.w + tempA3.w * tempB3.w;

		}
		barrier(CLK_LOCAL_MEM_FENCE);
	}
	/* Write 16 values to matrixC */
	matrixC[globalPos] = sum0;
	matrixC[globalPos +  get_global_size(0)] = sum1;
	matrixC[globalPos +  2 * get_global_size(0)] = sum2;
	matrixC[globalPos +  3 * get_global_size(0)] = sum3;
}