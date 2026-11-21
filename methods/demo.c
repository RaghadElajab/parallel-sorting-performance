#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef struct
{
    int *numbers; // Pointer to the array values
    int size;     // Number of values in the array
} IntArray;

IntArray insertionSort(IntArray input) // sequential insertionsort implementation
{
    IntArray sorted; // prep: allocating memory and copying input array
    sorted.size = input.size;
    sorted.numbers = malloc(sorted.size * sizeof(int));
    memcpy(sorted.numbers, input.numbers, sorted.size * sizeof(int));
    printf("Unsorted array: ");
    printArray(sorted);
    for (int cur = 1; cur < sorted.size; cur++) // start at the second element because the first is already sorted
    {
        int val = sorted.numbers[cur];                  // value to be inserted
        int prev = cur - 1;                             // begin comparing with the element before the current value
        while (prev >= 0 && sorted.numbers[prev] > val) // shift larger values one position to the right
        {
            sorted.numbers[prev + 1] = sorted.numbers[prev];
            prev--;
        }
        sorted.numbers[prev + 1] = val; // insert the current value into its correct position
        printf("Step %d: ", cur);
        printArray(sorted);
    }

    return sorted;
}
// Print an array to the console
void printArray(IntArray array)
{
    printf("[");

    for (int index = 0; index < array.size; index++)
    {
        printf("%d", array.numbers[index]);

        if (index < array.size - 1)
            printf(", ");
    }
    printf("]\n");
}

int main()
{
    int sampleValues[10] = { 8, 3, 5, 1, 9, 6, 2, 7, 4, 0};

    IntArray sample;

    sample.numbers = sampleValues;
    sample.size = 10;

    IntArray sortedSample = insertionSort(sample);

    printf("\nSorted array: ");
    printArray(sortedSample);

    free(sortedSample.numbers);

    return 0;
}
