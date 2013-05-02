__kernel void Mult(const __global float* a, const __global float* b, __global float* c, size_t size)
{
    size_t col = get_global_id(0);
    size_t row = get_global_id(1);

    // check bounds
    if(row >= size || col >= size)
        return;

    T sum = 0;
    for(size_t i = 0; i < size; i++)
        sum += a[row * size + i] * b[i * size + col];

    c[get_global_id(0)] = sum;
}