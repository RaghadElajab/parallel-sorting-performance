#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>
#include <time.h>



// Stores an integer array together with its size
typedef struct
{
    int *numbers; // Pointer to the array values
    int size;     // Number of values in the array
} IntArray;


// Stores the input and output belonging to one thread
typedef struct
{
    IntArray chunk; //chunk assigned to the thread
    IntArray sortedChunk; //sorted result produced by the thread
} ThreadData;


double getTime() //return the current monotonic time in seconds
{
    struct timespec currentTime;
    clock_gettime( CLOCK_MONOTONIC, &currentTime);

    return currentTime.tv_sec + currentTime.tv_nsec / 1000000000.0;
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

IntArray openMPInsertionSort( IntArray input,int workerCount)
{
    if (input.size < 2)//arrays with fewer than two elements are already sorted
        return insertionSort(input);

    if (workerCount > input.size)
        workerCount = input.size;

    IntArray *chunks=splitArray(input, workerCount);

    #pragma omp parallel for num_threads(workerCount) //run the following loop in parallel using workerCount threads
    for ( int chunkId = 0;chunkId < workerCount;chunkId++ )
    {
        IntArray sortedChunk =insertionSort(chunks[chunkId]);
        free(chunks[chunkId].numbers);
        chunks[chunkId] = sortedChunk; //store the sorted chunk in its original position
    }
    return mergeAllArrays( chunks,workerCount);
}



// Check whether an array is sorted
int isSorted(IntArray array)
{
    for (int index = 1; index < array.size; index++)
    {
        if (
            array.numbers[index - 1] >
            array.numbers[index]
        )
        {
            return 0;
        }
    }

    return 1;
}


// Print an array
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


// Run the OpenMP benchmark
void runBenchmark(int workerCount)
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

    // Open the CSV file and replace its previous contents
    FILE *resultsFile = fopen(
        "openmp_results.csv",
        "w"
    );

    if (resultsFile == NULL)
    {
        perror("Could not create results file");
        exit(1);
    }

    // Write the CSV headings
    fprintf(
        resultsFile,
        "Array Size,Random Limit,Method,"
        "Workers,Time,Correct\n"
    );

    fflush(resultsFile);

    printf("\nOpenMP C Benchmark\n");
    printf("Workers: %d\n", workerCount);

    // Test every array-size and random-limit combination
    for (int sizeId = 0; sizeId < 5; sizeId++)
    {
        for (int limitId = 0; limitId < 5; limitId++)
        {
            int arraySize = arraySizes[sizeId];
            int randomLimit = randomLimits[limitId];
            IntArray input;
            input.size = arraySize;
            input.numbers = malloc( arraySize * sizeof(int));
            if (input.numbers == NULL)
            {
                perror("Memory allocation failed");
                fclose(resultsFile);
                exit(1);
            }
            // Use the same random seed for every test
            srand(42);
            for (
                int index = 0;
                index < arraySize;
                index++
            )
            {
                input.numbers[index] = rand() % (randomLimit + 1);
            }

            printf("\nRunning size %d, limit %d...\n", arraySize,randomLimit );

            // Start timing before the OpenMP sorting operation
            double startTime = omp_get_wtime();

            IntArray sortedNumbers = openMPInsertionSort(input, workerCount);

            // Stop timing after sorting and merging finish
            double elapsedTime =omp_get_wtime() - startTime;

            // Check correctness after timing
            int correct = isSorted( sortedNumbers);

            printf("Size: %10d | Limit: %9d | " "Time: %.6f seconds | Correct: %s\n",arraySize, randomLimit,elapsedTime, correct ? "True" : "False" );

            // Save the completed result
            fprintf( resultsFile, "%d,%d,OpenMP,%d,%.6f,%s\n", arraySize, randomLimit, workerCount, elapsedTime, correct ? "True" : "False");

            // Save output immediately
            fflush(resultsFile);
            fflush(stdout);

            // Release allocated memory
            free(input.numbers);
            free(sortedNumbers.numbers);
        }
    }

    fclose(resultsFile);
}


int main()
{
    // Print output immediately when running through Slurm
    setvbuf(stdout,  NULL,  _IOLBF, 0 );

    // Use four OpenMP threads when running locally
    int workerCount = 4;

    // Use the number of CPUs allocated by Slurm when available
    char *workerText = getenv( "SLURM_CPUS_PER_TASK");

    if (workerText != NULL)
        workerCount = atoi(workerText);

    // Set the number of OpenMP threads
    omp_set_num_threads(workerCount);

    // Sample set of 20 integers
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

    // Sort the sample using OpenMP
    IntArray sortedSample =
        openMPInsertionSort(
            sample,
            workerCount
        );

    printf("\nOpenMP sorted sample:\n");
    printArray(sortedSample);

    printf(
        "\nOpenMP workers: %d\n",
        workerCount
    );

    free(sortedSample.numbers);

    // Run all 25 benchmark combinations
    runBenchmark(workerCount);

    return 0;
}