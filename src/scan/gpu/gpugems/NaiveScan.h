#ifndef GPUGEMSNAIVESCAN_H
#define GPUGEMSNAIVESCAN_H

#include "../../ScanAlgorithm.h"
#include "../../../common/GPUAlgorithm.h"
#include "../../../common/utils.h"

namespace gpu
{
    namespace gpugems
    {
        /**
         * From: http://http.developer.nvidia.com/GPUGems3/gpugems3_ch39.html
         * Chapter: 39.2.1 A Naive Parallel Scan
         */
        template<typename T, size_t count>
        class NaiveScan : public GPUAlgorithm<T, count>, public ScanAlgorithm
        {
            public:
                string getName() override
                {
                    return "Naive Scan (GPU Gems) (inclusiv)";
                }

                bool isInclusiv() override
                {
                    return true;
                }

                void init(Context* context) override
                {
                    Program* program = context->createProgram("gpu/gpugems/NaiveScan.cl", "-D T=" + getTypeName<T>());
                    kernel = program->createKernel("NaiveScan");
                }

                void upload(Context* context, size_t workGroupSize, T* data) override
                {
                    bufferSize = pow2roundup(count);

                    source = context->createBuffer(CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, bufferSize * sizeof(T), data);
                    destination = context->createBuffer(CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, bufferSize * sizeof(T), data);
                }

                void run(CommandQueue* queue, size_t workGroupSize) override
                {
                    for(size_t dpower = 1; dpower < bufferSize; dpower <<= 1)
                    {
                        kernel->setArg(0, source);
                        kernel->setArg(1, destination);
                        kernel->setArg(2, dpower);

                        size_t globalWorkSizes[] = { count };
                        size_t localWorkSizes[] = { workGroupSize };

                        queue->enqueueKernel(kernel, 1, globalWorkSizes, localWorkSizes);
                        queue->enqueueCopy(destination, source);
                    }
                    queue->finish();
                }

                void download(CommandQueue* queue, T* result) override
                {
                    queue->enqueueRead(source, result, 0, count * sizeof(T));
                }

                void cleanup() override
                {
                    delete kernel;
                    delete source;
                    delete destination;
                }

                virtual ~NaiveScan() {}

            private:
                size_t bufferSize;
                Kernel* kernel;
                Buffer* source;
                Buffer* destination;
        };
    }
}


#endif // GPUGEMSNAIVESCAN_H
