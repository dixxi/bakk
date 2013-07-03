#include <iostream>
#include <fstream>
#include <array>

#include "../common/Runner.h"
#include "SortPlugin.h"

#include "cpu/Quicksort.h"
#include "cpu/QSort.h"
#include "cpu/STLSort.h"
#include "cpu/TimSort.h"
#include "cpu/amd/RadixSort.h"
#include "cpu/stereopsis/radixsort.h"

#include "gpu/bealto/ParallelSelectionSort.h"
#include "gpu/bealto/ParallelSelectionSortLocal.h"
#include "gpu/bealto/ParallelSelectionSortBlocks.h"
#include "gpu/bealto/ParallelBitonicSortLocal.h"
#include "gpu/bealto/ParallelBitonicSortLocalOptim.h"
#include "gpu/bealto/ParallelBitonicSortA.h"
#include "gpu/bealto/ParallelBitonicSortB2.h"
#include "gpu/bealto/ParallelBitonicSortB4.h"
#include "gpu/bealto/ParallelBitonicSortB8.h"
#include "gpu/bealto/ParallelBitonicSortB16.h"
#include "gpu/bealto/ParallelBitonicSortC.h"
#include "gpu/bealto/ParallelMergeSort.h"
#include "gpu/libCL/RadixSort.h"
#include "gpu/clpp/RadixSort.h"
#include "gpu/amd/BitonicSort.h"
#include "gpu/amd/RadixSort.h"
#include "gpu/nvidia/RadixSort.h"
#include "gpu/nvidia/BitonicSort.h"
#include "gpu/dixxi/RadixSort.h"
#include "gpu/dixxi/RadixSortAtomicCounters.h"
#include "gpu/amd_dixxi/RadixSort.h"
#include "gpu/gpugems/OddEvenTransition.h"

using namespace std;

int main()
{
    try
    {
        array<size_t, 13> sizes = { 1<<10, 1<<15, 1<<17, 1<<19, 1<<20, 1<<21, 1<<22, 1<<23, 1<<24, 1<<25, 1<<26, 1<<27, 1<<28 };
        //array<size_t, 1> sizes = { 1<<25 };
        Runner<cl_uint, SortPlugin> runner(5, sizes.begin(), sizes.end());
        //Runner<cl_uint, SortPlugin> runner(1, { 1<<17 });

        //runner.run<cpu::Quicksort>();
        runner.run<cpu::QSort>();
        runner.run<cpu::STLSort>();
        ////runner.run<cpu::TimSort>();
        //runner.run<cpu::amd::RadixSort>();
        runner.run<cpu::stereopsis::RadixSort>();

        //runner.run<gpu::bealto::ParallelSelectionSort>(CLRunType::GPU, true);
        //runner.run<gpu::bealto::ParallelSelectionSortLocal>(CLRunType::GPU, true);
        //runner.run<gpu::bealto::ParallelSelectionSortBlocks>(CLRunType::GPU, true);
        //runner.run<gpu::bealto::ParallelBitonicSortLocal>(CLRunType::GPU, true);
        //runner.run<gpu::bealto::ParallelBitonicSortLocalOptim>(CLRunType::GPU, true);
        //runner.run<gpu::bealto::ParallelBitonicSortA>(CLRunType::GPU, false);
        //runner.run<gpu::bealto::ParallelBitonicSortB2>(CLRunType::GPU, true);
        //runner.run<gpu::bealto::ParallelBitonicSortB4>(CLRunType::GPU, true);
        //runner.run<gpu::bealto::ParallelBitonicSortB8>(CLRunType::GPU, true);
        //runner.run<gpu::bealto::ParallelBitonicSortB16>(CLRunType::GPU, false);
        //runner.run<gpu::bealto::ParallelBitonicSortC>(CLRunType::GPU, false);
        //runner.run<gpu::bealto::ParallelMergeSort>(CLRunType::GPU, true);

        //runner.run<gpu::clpp::RadixSort>(CLRunType::GPU, true); // not working

        //runner.run<gpu::libcl::RadixSort>(CLRunType::GPU, false);

        //runner.run<gpu::amd::BitonicSort>(CLRunType::GPU, false);
        //runner.run<gpu::amd::RadixSort>(CLRunType::GPU, false); // crashes on large arrays
        //runner.run<gpu::amd_dixxi::RadixSort>(CLRunType::GPU, false);

        //runner.run<gpu::nvidia::RadixSort>(CLRunType::GPU, false);
        //runner.run<gpu::nvidia::BitonicSort>(CLRunType::GPU, false);

        //runner.run<gpu::dixxi::RadixSort>(CLRunType::GPU, false);
        //runner.run<gpu::dixxi::RadixSortAtomicCounters>(CLRunType::GPU, false);

        //runner.run<gpu::gpugems::OddEvenTransition>(CLRunType::GPU, false);

        runner.writeStats("stats.csv");
        //runner.writeGPUDeviceInfo("gpuinfo.csv");

        getchar();
    }
    catch(OpenCLException& e)
    {
        cerr << e.what() << endl;
        return 1;
    }

    return 0;
}
