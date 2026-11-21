#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


typedef struct //structure used to store an integer array and its size
{
    int *numbers; //Pointer to the first element of the array
    int size;  //Number of elements in the array
} IntArray;



double getTime() //function returns the current system time in seconds
{
    struct timespec currentTime;
    clock_gettime(CLOCK_MONOTONIC, &currentTime );
    return currentTime.tv_sec +  currentTime.tv_nsec/1000000000.0;
}


IntArray insertionSort(IntArray input) //sequential insertionsort implementation
{
    IntArray sorted; //prep: allocating memory and copying input array
    sorted.size = input.size;
    sorted.numbers = malloc(sorted.size * sizeof(int));
    memcpy(sorted.numbers,input.numbers, sorted.size * sizeof(int));

    for (int cur = 1; cur < sorted.size; cur++) //start at the second element because the first is already sorted
    {
        int val = sorted.numbers[cur];//value to be inserted
        int prev = cur - 1; //begin comparing with the element before the current value
        while ( prev >= 0 && sorted.numbers[prev] > val) //shift larger values one position to the right
        {
            sorted.numbers[prev+1]= sorted.numbers[prev];
            prev--;
        }
        sorted.numbers[prev+1] = val;//insert the current value into its correct position
    }
    return sorted;
}


int isSorted(IntArray array)
{
    for (int index = 1; index < array.size; index++)
    {
        if (array.numbers[index - 1] > array.numbers[index] )
        {
            return 0;
        }
    }

    return 1;
}


void printArray(IntArray array)
{
    printf("[");

    for (int index = 0; index < array.size; index++)
    {
        printf("%d", array.numbers[index]);

        if (index < array.size - 1)
        {
            printf(", ");
        }
    }

    printf("]\n");
}


void runBenchmark()
{
    int arraySizes[] = { 1000, 10000, 100000,1000000, 10000000 };

    int randomLimits[] = { 100,1000, 10000, 100000, 1000000};

    FILE *resultsFile = fopen( "sequential_c_results.csv", "w");

    if (resultsFile == NULL)
    {
        perror("Could not create results file");
        exit(1);
    }

    fprintf(
        resultsFile,
        "Array Size,Random Limit,Method,"
        "Workers,Time,Correct\n"
    );

    fflush(resultsFile);

    printf("\nSequential C Benchmark\n");

    for (int sizeId = 0; sizeId < 5; sizeId++)
    {
        for (int limitId = 0; limitId < 5; limitId++)
        {
            int arraySize = arraySizes[sizeId];
            int randomLimit = randomLimits[limitId];

            IntArray input;

            input.size = arraySize;
            input.numbers = malloc(
                arraySize * sizeof(int)
            );

            srand(42);

            for (
                int index = 0;
                index < arraySize;
                index++
            )
            {
                input.numbers[index] =
                    rand() % (randomLimit + 1);
            }

            printf( "\nRunning size %d, limit %d...\n",  arraySize,  randomLimit );

            double startTime = getTime();

            IntArray sortedNumbers =
                insertionSort(input);

            double elapsedTime =
                getTime() - startTime;

            int correct = isSorted(
                sortedNumbers
            );

            printf( "Size: %10d | Limit: %9d | " "Time: %.6f seconds | Correct: %s\n", arraySize,  randomLimit, elapsedTime,    correct ? "True" : "False" );

            fprintf(  resultsFile,"%d,%d,Sequential C,1,%.6f,%s\n", arraySize, randomLimit,  elapsedTime, correct ? "True" : "False" );

            fflush(resultsFile);
            fflush(stdout);

            free(input.numbers);
            free(sortedNumbers.numbers);
        }
    }

    fclose(resultsFile);
}


int main()
{
    setvbuf(stdout, NULL, _IOLBF, 0);

    int sampleValues[20] = {
        81, 14, 3, 94, 35,
        31, 28, 17, 94, 13,
        86, 94, 69, 11, 75,
        54, 4, 27, 29, 64
    };

    IntArray sample;

    sample.numbers = sampleValues;
    sample.size = 20;

    printf("Original sample:\n");
    printArray(sample);

    IntArray sortedSample = insertionSort(sample);

    printf("\nSequential sorted sample:\n");
    printArray(sortedSample);

    free(sortedSample.numbers);

    runBenchmark();

    return 0;
}