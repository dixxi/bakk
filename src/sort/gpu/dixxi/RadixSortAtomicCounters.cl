#ifndef RADIX
#error "RADIX has to be defined"
#endif

#define BUCKETS (1 << RADIX)
#define RADIX_MASK (BUCKETS - 1)

#pragma OPENCL EXTENSION cl_khr_local_int32_base_atomics : enable
#pragma OPENCL EXTENSION cl_khr_global_int32_base_atomics : enable
#pragma OPENCL EXTENSION cl_ext_atomic_counters_32 : enable

__kernel void Histogram(__global const T* data, __global uint* histogram, const uint start, const uint end, const uint bits)
{
    size_t id = get_global_id(0);

    if(id > end - start)
        return;

    id += start;

    T element = data[id];
    uint bucket = ((uint)element >> bits) & RADIX_MASK;

    atomic_inc(&histogram[bucket]);
}

__kernel void Scan(__global uint* histogram)
{
    uint sum = 0;
    for(size_t i = 0; i < BUCKETS; ++i)
    {
        //uint val = atomic_xchg(&histogram[i], sum);
        uint val = histogram[i];
        histogram[i] = sum;
        sum += val;
    }
}

__kernel void Permute(__global const T* data, __global T* result, __global uint* histogram, const uint start, const uint end, const uint bits)
{
    size_t id = get_global_id(0);

    if(id > end - start)
        return;

    id += start;

    T element = data[id];
    uint bucket = ((uint)element >> bits) & RADIX_MASK;

    uint index = atomic_inc(&histogram[bucket]);

    result[start + index] = element;
}

//__kernel void Permute(__global const T* data, __global __write_only T* result, const uint bits,
//                      counter32_t histogram0, counter32_t histogram1, counter32_t histogram2, counter32_t histogram3,
//                      counter32_t histogram4, counter32_t histogram5, counter32_t histogram6, counter32_t histogram7)
//{
//    size_t id = get_global_id(0);
//
//    T element = data[id];
//
//    uint bucket = ((uint)element >> bits) & (uint)RADIX_MASK;
//    uint index;
//    switch(bucket)
//    {
//        case 0: index = atomic_inc(histogram0); break;
//        case 1: index = atomic_inc(histogram1); break;
//        case 2: index = atomic_inc(histogram2); break;
//        case 3: index = atomic_inc(histogram3); break;
//        case 4: index = atomic_inc(histogram4); break;
//        case 5: index = atomic_inc(histogram5); break;
//        case 6: index = atomic_inc(histogram6); break;
//        case 7: index = atomic_inc(histogram7); break;
//        //default: printf("Error\n"); break;
//    }
//
//    //printf("id %d el %d bits %d bucket %d index %d\n", id, element, bits, bucket, index);
//    //printf(" ");
//
//    result[id] = index;
//}
