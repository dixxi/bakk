#pragma once

#include "../../../common/CLAlgorithm.h"
#include "../../SortAlgorithm.h"

using namespace std;

namespace gpu
{
    namespace gpugems
    {
        /**
         * From: http://http.developer.nvidia.com/GPUGems2/gpugems2_chapter46.html
         */
        template<typename T>
        class OddEvenTransition : public CLAlgorithm<T>, public SortAlgorithm
        {
            public:
                const string getName() override
                {
                    return "Odd Even Transition (GPU Gems)";
                }

                bool isInPlace() override
                {
                    return false;
                }

                void init() override
                {
                    Program* program = context->createProgram("gpu/gpugems/OddEvenTransition.cl", "-D T=" + getTypeName<T>());
                    kernel = program->createKernel("OddEvenTransition");
                    delete program;
                }

                void upload(size_t workGroupSize, T* data, size_t size) override
                {
                    buffer = context->createBuffer(CL_MEM_READ_WRITE, sizeof(T) * size);
                    queue->enqueueWrite(buffer, data);
                }

                void run(size_t workGroupSize, size_t size) override
                {
                    for (size_t i = 0; i < size; i++)
                    {
                        kernel->setArg(0, buffer);
                        size_t globalWorkSizes[1] = { size / 2 };
                        size_t localWorkSizes[1] = { workGroupSize };
                        queue->enqueueKernel(kernel, 1, globalWorkSizes, localWorkSizes);
                        queue->enqueueBarrier();
                    }
                }

                void download(T* result, size_t size) override
                {
                    queue->enqueueRead(buffer, result);
                    delete buffer;
                }

                void cleanup() override
                {
                    delete kernel;
                }

                virtual ~OddEvenTransition() {}

            private:
                Kernel* kernel;
                Buffer* buffer;
        };
    }
}
