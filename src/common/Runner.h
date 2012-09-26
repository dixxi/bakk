#ifndef RUNNER_H
#define RUNNER_H

#include "../common/OpenCL.h"
#include "../common/Timer.h"
#include "../common/utils.h"

#include <typeinfo>
#include <iomanip>
#include <map>
#include <algorithm>
#include <vector>
#include <string>

using namespace std;

/**
 * Used to run algorithms.
 */
template <typename T, template <typename> class Plugin>
class Runner
{
    public:
        const int FLOAT_PRECISION = 3;

        enum RunType
        {
            CPU,
            CL_CPU,
            CL_GPU
        };

        Runner(bool noCPU = true)
            : noCPU(noCPU)
        {
            OpenCL::init();
            gpuContext = OpenCL::getGPUContext();
            if(noCPU)
                cpuContext = nullptr;
            else
                cpuContext = OpenCL::getCPUContext();
            gpuQueue = gpuContext->createCommandQueue();
            if(cpuContext)
                cpuQueue = cpuContext->createCommandQueue();

            plugin = new Plugin<T>();
        }

        virtual ~Runner()
        {
            delete gpuContext;
            delete gpuQueue;
            if(cpuContext)
            {
                delete cpuContext;
                delete cpuQueue;
            }
            OpenCL::cleanup();

            delete plugin;
        }

        /**
         * Prints some information about the OpenCL devices used.
         */
        void printCLInfo()
        {
            cout << "Running on the following devices:" << endl;
            if(cpuContext)
                printDevice(cpuContext);
            printDevice(gpuContext);
            cout << endl;
        }

        void printDevice(Context* context)
        {
            cout << "" << context->getInfo<string>(CL_DEVICE_NAME) << endl;
            cout << "   " << context->getInfo<string>(CL_DEVICE_VENDOR) << endl;
            cout << "   " << context->getInfo<size_t>(CL_DEVICE_MAX_COMPUTE_UNITS) << " compute units" << endl;
            cout << "   " << context->getInfo<size_t>(CL_DEVICE_MAX_CLOCK_FREQUENCY) << " MHz frequency" << endl;
            cout << "   " << fixed << setprecision(FLOAT_PRECISION) << sizeToString(context->getInfo<cl_ulong>(CL_DEVICE_GLOBAL_MEM_SIZE)) << " global mem" << endl;
            cout << "   " << fixed << setprecision(FLOAT_PRECISION) << sizeToString(context->getInfo<cl_ulong>(CL_DEVICE_LOCAL_MEM_SIZE)) << " local mem" << endl;
        }

        /**
         * Runs an algorithm on the CPU with the given problem size.
         * The results of the run are printed to stdout.
         */
        template <template <typename> class Algorithm>
        void printRun(size_t size)
        {
            RunStats stats = run<Algorithm>(size);

            cout << "###############################################################################" << endl;
            cout << "# " << stats.algorithmName << endl;
            cout << "#  " << stats.taskDescription << endl;
            cout << "#  CPU       " << fixed << setprecision(FLOAT_PRECISION) << stats.runTime << "s " << (stats.verificationResult ? "SUCCESS" : "FAILED ") << endl;
            cout << "###############################################################################" << endl;
            cout << endl;
        }

        /**
         * Runs an algorithm on the CPU using OpenCL.
         */
        template <template <typename> class Algorithm>
        void runCLCPU(size_t size, bool useMultipleWorkGroupSizes)
        {
            runCL<Algorithm>(cpuContext, cpuQueue, useMultipleWorkGroupSizes, size);
        }

        /**
         * Runs an algorithm on the GPU using OpenCL.
         */
        template <template <typename> class Algorithm>
        void runCLGPU(size_t size, bool useMultipleWorkGroupSizes)
        {
            runCL<Algorithm>(gpuContext, gpuQueue, useMultipleWorkGroupSizes, size);
        }

    private:
        struct RunStats
        {
            RunType runType;
            const string algorithmName;
            const string taskDescription;
            size_t wgSize;
            double initTime;
            double uploadTime;
            double runTime;
            double downloadTime;
            double cleanupTime;
            bool verificationResult;
            bool exceptionOccured;
            string exceptionMsg;

            RunStats(RunType runType, const string algorithmName, const string taskDescription)
                : runType(runType), algorithmName(algorithmName), taskDescription(taskDescription), wgSize(0), initTime(0), uploadTime(0), runTime(0), downloadTime(0), cleanupTime(0), verificationResult(false), exceptionOccured(false)
            {
            }
        };

        /**
         * Runs an algorithm on the CPU with the given problem size.
         */
        template <template <typename> class Algorithm>
        RunStats run(size_t size)
        {
            // create algorithm and stats, prepare input
            Algorithm<T>* alg = new Algorithm<T>();
            RunStats stats(RunType::CPU, alg->getName(), plugin->getTaskDescription(size));

            data = plugin->genInput(size);
            result = plugin->genResult(size);

            // run algorithm
            timer.start();
            alg->run(data, result, size);
            stats.runTime = timer.stop();

            // verfiy result
            stats.verificationResult = plugin->verifyResult(alg, data, result, size);

            // cleanup
            plugin->freeInput(data);
            plugin->freeResult(result);

            delete alg;

            return stats;
        }

        template <template <typename> class Algorithm>
        void runCL(Context* context, CommandQueue* queue, bool useMultipleWorkGroupSizes, size_t size)
        {
            // create algorithm and stats, prepare input
            Algorithm<T>* alg = new Algorithm<T>();
            data = plugin->genInput(size);
            result = plugin->genResult(size);

            // run custom initialization
            timer.start();
            alg->init(context);
            double initTime = timer.stop();

            map<int, RunStats> stats;

            size_t maxWorkGroupSize = min(context->getInfo<size_t>(CL_DEVICE_MAX_WORK_GROUP_SIZE), size);
            if(!useMultipleWorkGroupSizes)
                stats[maxWorkGroupSize] = uploadRunDownload(alg, context, queue, maxWorkGroupSize, size);
            else
                for(size_t i = 1; i <= maxWorkGroupSize; i <<= 1)
                    stats[i] = uploadRunDownload(alg, context, queue, i);

            // cleanup
            timer.start();
            alg->cleanup();
            double cleanupTime = timer.stop();

            // calculate sum figures
            pair<int, RunStats> fastest = *min_element(stats.begin(), stats.end(), [](pair<int, RunStats> a, pair<int, RunStats> b)
            {
                if(a.second.exceptionOccured || !a.second.verificationResult)
                    return false;
                if(b.second.exceptionOccured || !b.second.verificationResult)
                    return true;
                return a.second.uploadTime + a.second.runTime + a.second.downloadTime < b.second.uploadTime + b.second.runTime + b.second.downloadTime;
            });

            double uploadAvg = 0;
            for(auto e : stats)
                if(!e.second.exceptionOccured)
                    uploadAvg += e.second.uploadTime;
            uploadAvg /= stats.size();

            double downloadAvg = 0;
            for(auto e : stats)
                if(!e.second.exceptionOccured)
                    downloadAvg += e.second.downloadTime;
            downloadAvg /= stats.size();

            // print results
            cout << "###############################################################################" << endl;
            cout << "# " << alg->getName() << endl;
            cout << "#  " << plugin->getTaskDescription() << endl;
            cout << "#  (Init)         " << fixed << setprecision(FLOAT_PRECISION) << initTime << "s" << endl;
            cout << "#  Upload (avg)   " << fixed << setprecision(FLOAT_PRECISION) << uploadAvg << "s" << endl;

            for(auto s : stats)
                if(s.second.exceptionOccured)
                    cout << "#  GPU (WG: " << setw(4) << s.first << ") EXCEPTION: " << s.second.exceptionMsg << endl;
                else
                    cout << "#  GPU (WG: " << setw(4) << s.first << ") " << fixed << setprecision(FLOAT_PRECISION) << s.second.runTime << "s " << (s.second.verificationResult ? "SUCCESS" : "FAILED ") << endl;

            cout << "#  Download (avg) " << fixed << setprecision(FLOAT_PRECISION) << downloadAvg << "s" << endl;
            cout << "#  (Cleanup)      " << fixed << setprecision(FLOAT_PRECISION) << cleanupTime << "s" << endl;
            cout << "#  Fastest        " << fixed << setprecision(FLOAT_PRECISION) << (fastest.second.uploadTime + fastest.second.runTime + fastest.second.downloadTime) << "s " << "(WG size: " << fastest.first << ") " << endl;
            cout << "###############################################################################" << endl;
            cout << endl;

            // cleanup
            plugin->freeInput(data);
            plugin->freeResult(result);

            delete alg;
        }

        template <template <typename> class Algorithm>
        inline RunStats uploadRunDownload(Algorithm<T>* alg, Context* context, CommandQueue* queue, size_t workGroupSize, size_t size)
        {
            RunStats stats;

            try
            {
                // upload data
                timer.start();
                alg->upload(context, queue, workGroupSize, data, size);
                queue->finish();
                stats.uploadTime = timer.stop();

                // run algorithm
                timer.start();
                alg->run(queue, workGroupSize, size);
                queue->finish();
                stats.runTime = timer.stop();

                // download data
                timer.start();
                alg->download(queue, result, size);
                queue->finish();
                stats.downloadTime = timer.stop();

                // verify
                stats.verificationResult = plugin->verify(alg, data, result);
            }
            catch(OpenCLException& e)
            {
                stats.exceptionOccured = true;
                stats.exceptionMsg = e.what();
            }

            return stats;
        }

        void prepareTest(size_t size)
        {

        }

        void finishTest()
        {

        }

        Context* gpuContext;
        Context* cpuContext;
        CommandQueue* gpuQueue;
        CommandQueue* cpuQueue;

        Plugin<T>* plugin;

        Timer timer;

        void* data;
        void* result;

        bool noCPU;
};

#endif // RUNNER_H