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

// Sort chunks sequentially before merging them
IntArray sequentialChunkedInsertionSort(IntArray input, int chunkCount)
{
    if (input.size < 2)
        return insertionSort(input);

    if (chunkCount > input.size)
        chunkCount = input.size;

    // Divide the input into the same chunks as the parallel methods
    IntArray *chunks = splitArray(input,chunkCount);

    // Sort every chunk one at a time
    for (int chunkId = 0; chunkId < chunkCount; chunkId++)
    {
        IntArray sortedChunk =insertionSort(chunks[chunkId]);
        free(chunks[chunkId].numbers);
        chunks[chunkId] = sortedChunk;
    }

    // Merge the chunks using the same merging method
    return mergeAllArrays( chunks, chunkCount);
}

void runBenchmark(int chunkCount)
{
    int arraySizes[] = {
        1000,
        10000,
        100000,
        1000000,
        10000000
    };

    int randomLimits[] = {
        100,
        1000,
        10000,
        100000,
        1000000
    };

    FILE *resultsFile = fopen( "results/sequential_chunked_results.csv", "w");

    if (resultsFile == NULL)
    {
        perror("Could not create results file");
        exit(1);
    }

    fprintf( resultsFile,"Array Size,Random Limit,Method,""Workers,Time,Correct\n");

    fflush(resultsFile);

    printf("\nSequential Chunked Benchmark\n");
    printf("Chunks: %d\n", chunkCount);
    printf("Workers: 1\n");

    for (int sizeId = 0; sizeId < 5; sizeId++)
    {
        for (int limitId = 0; limitId < 5; limitId++)
        {
            int arraySize = arraySizes[sizeId];
            int randomLimit = randomLimits[limitId];

            IntArray input;

            input.size = arraySize;
            input.numbers = malloc(arraySize * sizeof(int));

            if (input.numbers == NULL)
            {
                perror("Memory allocation failed");
                fclose(resultsFile);
                exit(1);
            }

            srand(42);

            for (int index = 0;index < arraySize;index++)
                input.numbers[index] =rand() % (randomLimit + 1);
            

            printf("\nRunning size %d, limit %d...\n", arraySize, randomLimit);

            double startTime = getTime();

            IntArray sortedNumbers =sequentialChunkedInsertionSort( input, chunkCount );

            double elapsedTime =getTime() - startTime;

            int correct = isSorted( sortedNumbers);

            printf( "Size: %10d | Limit: %9d | " "Time: %.6f seconds | Correct: %s\n", arraySize, randomLimit,elapsedTime,correct ? "True" : "False" );

            // Workers is 1 because the chunks run sequentially
            fprintf( resultsFile,"%d,%d,Sequential Chunked,1,%.6f,%s\n",arraySize,randomLimit,elapsedTime, correct ? "True" : "False");

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

    // Must match the parallel worker count
    int chunkCount = 10;

    runBenchmark(chunkCount);

    return 0;
}