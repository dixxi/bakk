#pragma once

#include "../../../common/CLAlgorithm.h"
#include "../../SortAlgorithm.h"

using namespace std;

namespace gpu
{
    namespace amd
    {
        /**
         * From: AMD Stream SDK Samples
         */
        template<typename T>
        class BitonicSort : public CLAlgorithm<T>, public SortAlgorithm
        {
            public:
                const string getName() override
                {
                    return "Bitonic Sort (AMD)";
                }

                bool isInPlace() override
                {
                    return false;
                }

                void init() override
                {
                    program = context->createProgram("gpu/amd/BitonicSort.cl", "-D T=" + getTypeName<T>());
                    kernel = program->createKernel("BitonicSort");
                    delete program;
                }

                void upload(size_t workGroupSize, T* data, size_t size) override
                {
                    in = context->createBuffer(CL_MEM_READ_WRITE, sizeof(T) * size);
                    queue->enqueueWrite(in, data);
                }

                void run(size_t workGroupSize, size_t size) override
                {
                    /*for (size_t length = 1; length < size; length <<= 1)
                        for (size_t inc = length; inc > 0; inc >>= 1)
                        {
                            kernel->setArg(0, in);
                            kernel->setArg(1, swapBuffers ? in : out);
                            kernel->setArg(2, inc);
                            kernel->setArg(3, length<<1);

                            queue->enqueueKernel(kernel, 1, globalWorkSizes, localWorkSizes);
                            queue->enqueueBarrier();
                            swapBuffers = !swapBuffers;
                            //nk++;
                        }
                    queue->finish();*/

                    size_t globalWorkSizes[1] = { size / 2 };
                    size_t localWorkSizes[1] = { workGroupSize };

                    /*
                     * This algorithm is run as NS stages. Each stage has NP passes.
                     * so the total number of times the kernel call is enqueued is NS * NP.
                     *
                     * For every stage S, we have S + 1 passes.
                     * eg: For stage S = 0, we have 1 pass.
                     *     For stage S = 1, we have 2 passes.
                     *
                     * if length is 2^N, then the number of stages (numStages) is N.
                     * Do keep in mind the fact that the algorithm only works for
                     * arrays whose size is a power of 2.
                     *
                     * here, numStages is N.
                     *
                     * For an explanation of how the algorithm works, please go through
                     * the documentation of this sample.
                     */

                    /*
                     * 2^numStages should be equal to length.
                     * i.e the number of times you halve length to get 1 should be numStages
                     */
                    size_t numStages = 0;
                    for(size_t temp = size; temp > 1; temp >>= 1)
                        ++numStages;

                    // Set appropriate arguments to the kernel

                    // the input array - also acts as output for this pass (input for next)
                    //status = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&inputBuffer);
                    kernel->setArg(0, in);

                    // length - i.e number of elements in the array
                    //status = clSetKernelArg(kernel, 3, sizeof(cl_uint), (void *)&size);
                    kernel->setArg(3, (cl_uint)size);

                    // whether sort is to be in increasing order. CL_TRUE implies increasing
                    cl_bool sortDescending = CL_TRUE;
                    //status = clSetKernelArg(kernel, 4, sizeof(cl_uint), (void *)&sortDescending);
                    kernel->setArg(4, sortDescending);

                    for(cl_uint stage = 0; stage < numStages; ++stage)
                    {
                        // stage of the algorithm
                        //status = clSetKernelArg(kernel, 1, sizeof(cl_uint), (void *)&stage);
                        kernel->setArg(1, stage);

                        // Every stage has stage + 1 passes
                        for(cl_uint passOfStage = 0; passOfStage < stage + 1; ++passOfStage)
                        {
                            // pass of the current stage
                            //status = clSetKernelArg(kernel, 2, sizeof(cl_uint), (void *)&passOfStage);
                            kernel->setArg(2, passOfStage);

                            /*
                             * Enqueue a kernel run call.
                             * For simplicity, the groupsize used is 1.
                             *
                             * Each thread writes a sorted pair.
                             * So, the number of  threads (global) is half the length.
                             */
                            //cl_event ndrEvt;
                            //status = clEnqueueNDRangeKernel(commandQueue, kernel, 1, NULL, globalThreads, localThreads, 0, NULL, &ndrEvt);
                            queue->enqueueKernel(kernel, 1, globalWorkSizes, localWorkSizes);
                            queue->enqueueBarrier();

                            //CHECK_OPENCL_ERROR(status, "clEnqueueNDRangeKernel failed.");

                            //status = clFlush(commandQueue);
                            //CHECK_OPENCL_ERROR(status, "clFlush failed.");

                            //status = sampleCommon->waitForEventAndRelease(&ndrEvt);
                            //CHECK_ERROR(status, 0, "WaitForEventAndRelease(ndrEvt) Failed");
                        }
                    }

                    // Enqueue readBuffer
                    /*cl_event readEvt;
                    status = clEnqueueReadBuffer(
                                commandQueue,
                                inputBuffer,
                                CL_FALSE,
                                0,
                                length * sizeof(cl_uint),
                                input,
                                0,
                                NULL,
                                &readEvt);
                    CHECK_OPENCL_ERROR(status, "clEnqueueReadBuffer failed.");

                    status = clFlush(commandQueue);
                    CHECK_OPENCL_ERROR(status, "clFlush failed.");

                    status = sampleCommon->waitForEventAndRelease(&readEvt);
                    CHECK_ERROR(status, 0, "WaitForEventAndRelease(inMapEvt1) Failed");

                    return SDK_SUCCESS;*/
                }

                void download(T* result, size_t size) override
                {
                    queue->enqueueRead(in, result);
                    delete in;
                }

                void cleanup() override
                {
                    delete kernel;
                }

                virtual ~BitonicSort() {}

            private:
                Program* program;
                Kernel* kernel;
                Buffer* in;
        };
    }
}
