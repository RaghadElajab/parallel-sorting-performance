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
// Divide the input array into nearly equal chunks
IntArray *splitArray( IntArray input,int workerCount)
{
    IntArray *chunks = malloc( workerCount * sizeof(IntArray) );

    int basicSize = input.size / workerCount;
    int extraItems = input.size % workerCount;
    int start = 0;

    for ( int workerId = 0; workerId < workerCount; workerId++)
    {
        int chunkSize = basicSize;

        // Give one extra value to the first few chunks
        if (workerId < extraItems)
        {
            chunkSize++;
        }

        chunks[workerId].size = chunkSize;
        chunks[workerId].numbers = malloc( chunkSize * sizeof(int) );

        // Copy this section of the input into the chunk
        memcpy( chunks[workerId].numbers, &input.numbers[start], chunkSize * sizeof(int));
        start += chunkSize;
    }

    return chunks;
}
IntArray mergeTwoArrays( IntArray leftArray, IntArray rightArray)//merge two sorted arrays into one sorted array
{
    IntArray mergedArray;
    mergedArray.size = leftArray.size + rightArray.size;
    mergedArray.numbers = malloc( mergedArray.size * sizeof(int));
    int left = 0;
    int right = 0;
    int merged = 0;

    while ( left < leftArray.size &&right < rightArray.size) //repeatedly copy the smaller available value
    {
        if (leftArray.numbers[left] <=rightArray.numbers[right])
        {
            mergedArray.numbers[merged] = leftArray.numbers[left];
            left++;
        }
        else
        {
            mergedArray.numbers[merged] =  rightArray.numbers[right];
            right++;
        }
        merged++;
    }
    while (left < leftArray.size) //copy any remaining values from the left array
    {
        mergedArray.numbers[merged] = leftArray.numbers[left];
        left++;
        merged++;
    }
    while (right < rightArray.size) //copy any remaining values from the right array
    {
        mergedArray.numbers[merged] = rightArray.numbers[right];
        right++;
        merged++;
    }
    return mergedArray;
}

IntArray mergeAllArrays(IntArray *sortedChunks, int chunkCount) //repeatedly merge pairs of chunks until one array remains
{
    while (chunkCount > 1)
    {
        int mergedCount = (chunkCount + 1) / 2;
        IntArray *mergedChunks = malloc(mergedCount * sizeof(IntArray));
        int mergedId = 0;
        for ( int chunkId = 0;  chunkId < chunkCount;   chunkId += 2 )
        {
            if (chunkId + 1 < chunkCount)
            {
                mergedChunks[mergedId] = mergeTwoArrays( sortedChunks[chunkId],  sortedChunks[chunkId + 1]); //mrge the current pair of chunks
                
                free(sortedChunks[chunkId].numbers);// The old chunks are no longer needed
                free(sortedChunks[chunkId + 1].numbers);
            }
            else
            {              
                mergedChunks[mergedId] =sortedChunks[chunkId];//move an unpaired final chunk to the next round
            }
            mergedId++;
        }
        free(sortedChunks);
        sortedChunks = mergedChunks;
        chunkCount = mergedCount;
    }
    IntArray result = sortedChunks[0];
    free(sortedChunks);
    return result;
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