#pragma once

#include <algorithm>

#include "../../../common/CLAlgorithm.h"
#include "../../SortAlgorithm.h"

using namespace std;

namespace gpu
{
    namespace bealto
    {
        /**
         * From: http://www.bealto.com/gpu-sorting_intro.html
         */
        template<typename T>
        class ParallelBitonicSortB2 : public CLAlgorithm<T>, public SortAlgorithm
        {
            public:
                const string getName() override
                {
                    return "Parallel bitonic B2 (Bealto)";
                }

                bool isInPlace() override
                {
                    return false;
                }

                void init() override
                {
                    Program* program = context->createProgram("gpu/bealto/ParallelBitonicSortB2.cl", "-D T=" + getTypeName<T>());
                    kernel = program->createKernel("ParallelBitonicSortB2");
                    delete program;
                }

                void upload(size_t workGroupSize, T* data, size_t size) override
                {
                    buffer = context->createBuffer(CL_MEM_READ_WRITE, sizeof(T) * size);
                    queue->enqueueWrite(buffer, data);
                }

                void run(size_t workGroupSize, size_t size) override
                {
                    for (cl_int length = 1; length < size; length <<= 1)
                    {
                        cl_int inc = length;
                        while (inc > 0)
                        {
                            int ninc = 0;

                            // Always allow B2
                            if (ninc == 0)
                                ninc = 1;
                            size_t nThreads = size >> ninc;
                            workGroupSize = min(workGroupSize, nThreads);
                            kernel->setArg(0, buffer);
                            kernel->setArg(1, inc);          // INC passed to kernel
                            kernel->setArg(2, length << 1);  // DIR passed to kernel
                            size_t globalWorkSizes[1] = { nThreads };
                            size_t localWorkSizes[1] = { workGroupSize };
                            queue->enqueueKernel(kernel, 1, globalWorkSizes, localWorkSizes);
                            queue->enqueueBarrier();
                            inc >>= ninc;
                        }
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

                virtual ~ParallelBitonicSortB2() {}

            private:
                Kernel* kernel;
                Buffer* buffer;
        };
    }
}
