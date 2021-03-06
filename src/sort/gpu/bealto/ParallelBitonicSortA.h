#pragma once

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
        class ParallelBitonicSortA : public CLAlgorithm<T>, public SortAlgorithm
        {
            public:
                const string getName() override
                {
                    return "Parallel bitonic A (Bealto)";
                }

                bool isInPlace() override
                {
                    return false;
                }

                void init() override
                {
                    Program* program = context->createProgram("gpu/bealto/ParallelBitonicSortA.cl", "-D T=" + getTypeName<T>());
                    kernel = program->createKernel("ParallelBitonicSortA");
                    delete program;
                }

                void upload(size_t workGroupSize, T* data, size_t size) override
                {
                    in = context->createBuffer(CL_MEM_READ_WRITE, sizeof(T) * size);
                    queue->enqueueWrite(in, data);
                    out = context->createBuffer(CL_MEM_READ_WRITE, sizeof(T) * size);
                }

                void run(size_t workGroupSize, size_t size) override
                {
                    for (cl_int length = 1; length < size; length <<= 1)
                        for (cl_int inc = length; inc > 0; inc >>= 1)
                        {
                            kernel->setArg(0, in);
                            kernel->setArg(1, out);
                            kernel->setArg(2, inc);
                            kernel->setArg(3, length << 1);
                            size_t globalWorkSizes[1] = { size };
                            size_t localWorkSizes[1] = { workGroupSize };
                            queue->enqueueKernel(kernel, 1, globalWorkSizes, localWorkSizes);
                            queue->enqueueBarrier();

                            swap(in, out);
                        }
                }

                void download(T* result, size_t size) override
                {
                    queue->enqueueRead(in, result);
                   // printArr(result, size);
                    delete in;
                    delete out;
                }

                void cleanup() override
                {
                    delete kernel;
                }

                virtual ~ParallelBitonicSortA() {}

            private:
                Kernel* kernel;
                Buffer* in;
                Buffer* out;
        };
    }
}
