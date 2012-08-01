#ifndef QUICKSORT_H
#define QUICKSORT_H

#include "../CPUSortingAlgorithm.h"

template<typename T, size_t count>
class Quicksort : public CPUSortingAlgorithm<T, count>
{
    public:
        Quicksort()
            : CPUSortingAlgorithm<T, count>("Quicksort")
        {
        }

    protected:
        void sort()
        {
            sort_r(0, count - 1);
        }

    private:
        /**
         * Recursive quicksort implementation.
         *
         * @param start
         *            Start index.
         * @param end
         *            End index.
         */
        void sort_r(int start, int end)
        {
            if (start >= end)
                return;

            int left = start;
            int right = end;

            // select pivot
            int pivot = SortingAlgorithm<T, count>::data[(left + right) / 2];

            // partition array
            while (left <= right)
            {
                while (SortingAlgorithm<T, count>::data[left] < pivot)
                    left++;
                while (pivot < SortingAlgorithm<T, count>::data[right])
                    right--;

                // swap
                if (left <= right)
                {
                    ::swap(SortingAlgorithm<T, count>::data[right], SortingAlgorithm<T, count>::data[left]);

                    left++;
                    right--;
                }
            }

            // recursion
            sort_r(start, left - 1);
            sort_r(left, end);
        }
};

#endif // QUICKSORT_H