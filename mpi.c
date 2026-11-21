#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <mpi.h>

// Stores an integer array together with its size
typedef struct
{
    int *numbers; // Pointer to the array values
    int size;     // Number of values in the array
} IntArray;

// Stores the input and output belonging to one thread
typedef struct
{
    IntArray chunk;       // chunk assigned to the thread
    IntArray sortedChunk; // sorted result produced by the thread
} ThreadData;

IntArray insertionSort(IntArray input) // sequential insertionsort implementation
{
    IntArray sorted; // prep: allocating memory and copying input array
    sorted.size = input.size;
    sorted.numbers = malloc(sorted.size * sizeof(int));
    memcpy(sorted.numbers, input.numbers, sorted.size * sizeof(int));

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
    }
    return sorted;
}

IntArray *splitArray(IntArray input, int workerCount)// Divide the input array into nearly equal chunks
{
    IntArray *chunks = malloc(workerCount * sizeof(IntArray));

    int local_n = input.size / workerCount;
    int extraItems = input.size % workerCount;
    int start = 0;

    for (int workerId = 0; workerId < workerCount; workerId++)
    {
        int chunkSize = local_n;
        //give one extra value to the first few chunks
        if (workerId < extraItems)
            chunkSize++;

        chunks[workerId].size = chunkSize;
        chunks[workerId].numbers = malloc(chunkSize * sizeof(int));

        //copy this section of the input into the chunk
        memcpy(chunks[workerId].numbers, &input.numbers[start], chunkSize * sizeof(int));
        start += chunkSize;
    }

    return chunks;
}

IntArray mergeTwoArrays(IntArray leftArray, IntArray rightArray) // merge two sorted arrays into one sorted array
{
    IntArray mergedArray;
    mergedArray.size = leftArray.size + rightArray.size;
    mergedArray.numbers = malloc(mergedArray.size * sizeof(int));
    int left = 0;
    int right = 0;
    int merged = 0;

    while (left < leftArray.size && right < rightArray.size) // repeatedly copy the smaller available value
    {
        if (leftArray.numbers[left] <= rightArray.numbers[right])
        {
            mergedArray.numbers[merged] = leftArray.numbers[left];
            left++;
        }
        else
        {
            mergedArray.numbers[merged] = rightArray.numbers[right];
            right++;
        }
        merged++;
    }
    while (left < leftArray.size) // copy any remaining values from the left array
    {
        mergedArray.numbers[merged] = leftArray.numbers[left];
        left++;
        merged++;
    }
    while (right < rightArray.size) // copy any remaining values from the right array
    {
        mergedArray.numbers[merged] = rightArray.numbers[right];
        right++;
        merged++;
    }
    return mergedArray;
}

IntArray mergeAllArrays(IntArray *sortedChunks, int chunkCount) // repeatedly merge pairs of chunks until one array remains
{
    while (chunkCount > 1)
    {
        int mergedCount = (chunkCount + 1) / 2;
        IntArray *mergedChunks = malloc(mergedCount * sizeof(IntArray));
        int mergedId = 0;
        for (int chunkId = 0; chunkId < chunkCount; chunkId += 2)
        {
            if (chunkId + 1 < chunkCount)
            {
                mergedChunks[mergedId] = mergeTwoArrays(sortedChunks[chunkId], sortedChunks[chunkId + 1]); // mrge the current pair of chunks

                free(sortedChunks[chunkId].numbers); // The old chunks are no longer needed
                free(sortedChunks[chunkId + 1].numbers);
            }
            else
            {
                mergedChunks[mergedId] = sortedChunks[chunkId]; // move an unpaired final chunk to the next round
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

IntArray mpiInsertionSort(IntArray input, MPI_Comm communicator)
{
    int myRank; // initialization
    int comm_sz;
    MPI_Comm_rank(communicator, &myRank);
    MPI_Comm_size(communicator, &comm_sz);

    int *chunkSizes = NULL; // store the number of elements assigned to each process
    int *chunkStarts = NULL;

    if (myRank == 0) // rank 0 calculates the chunks
    {
        chunkSizes = malloc(comm_sz * sizeof(int)); // allocate the chunksize & start array
        chunkStarts = malloc(comm_sz * sizeof(int));
        int local_n = input.size / comm_sz;    // num elements per chunk
        int extraItems = input.size % comm_sz; // extra elements
        int start = 0;

        for (int processId = 0; processId < comm_sz; processId++) // calculate the size and starting position of every chunk
        {
            chunkSizes[processId] = local_n; // assign n tasks to each process and allocate the extra ones also

            if (processId < extraItems)
                chunkSizes[processId]++;

            chunkStarts[processId] = start; // store where this chunk begins in the input array
            start += chunkSizes[processId]; // move tsart to beginning of next chunk
        }
    }

    int localSize = 0;

    MPI_Scatter(chunkSizes, 1, MPI_INT, &localSize, 1, MPI_INT, 0, communicator); // send each process its chunk size

    IntArray localChunk;
    localChunk.size = localSize;
    localChunk.numbers = NULL;

    if (localSize > 0)
        localChunk.numbers = malloc(localSize * sizeof(int)); // allocate memory if this process receives any values
    // send a differen -sized section of the array to each process
    MPI_Scatterv(input.numbers, chunkSizes, chunkStarts, MPI_INT, localChunk.numbers, localSize, MPI_INT, 0, communicator);
    IntArray sortedLocalChunk = insertionSort(localChunk); // each process sorts its chunk

    free(localChunk.numbers);

    int *gatheredNumbers = NULL;
    if (myRank == 0) // only rank 0 requires space for the complete gathered array
        gatheredNumbers = malloc(input.size * sizeof(int));

    // return sorted chunks to rank 0
    MPI_Gatherv(sortedLocalChunk.numbers, sortedLocalChunk.size, MPI_INT, gatheredNumbers, chunkSizes, chunkStarts, MPI_INT, 0, communicator);
    free(sortedLocalChunk.numbers);
    IntArray result;
    result.numbers = NULL;
    result.size = 0;

    // Rank 0 merges the sorted chunks
    if (myRank == 0)
    {
        IntArray *sortedChunks = malloc(comm_sz * sizeof(IntArray));

        for (int processId = 0; processId < comm_sz; processId++)
        {
            int chunkSize = chunkSizes[processId];
            sortedChunks[processId].size = chunkSize;
            sortedChunks[processId].numbers = NULL;
            if (chunkSize > 0)
            {
                sortedChunks[processId].numbers = malloc(chunkSize * sizeof(int));
                memcpy(sortedChunks[processId].numbers, &gatheredNumbers[chunkStarts[processId]], chunkSize * sizeof(int));
            }
        }
        result = mergeAllArrays(sortedChunks, comm_sz);
        free(gatheredNumbers);
        free(chunkSizes);
        free(chunkStarts);
    }

    return result;
}

int isSorted(IntArray array)//check if sorted
{
    for (int index = 1; index < array.size; index++)
    {
        if (array.numbers[index - 1] > array.numbers[index])
            return 0;
    }

    return 1;
}

void printArray(IntArray array)//printing result
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

void runBenchmark(MPI_Comm communicator)
{
    int myRank;
    int comm_sz;

    MPI_Comm_rank(communicator, &myRank);

    MPI_Comm_size(communicator, &comm_sz);

    int arraySizes[] = {
        1000,
        10000,
        100000,
        1000000,
        10000000};

    int randomLimits[] = {
        100,
        1000,
        10000,
        100000,
        1000000};

    FILE *resultsFile = NULL;

    if (myRank == 0)
    {
        resultsFile = fopen("mpi_c_results.csv", "w");

        if (resultsFile == NULL)
        {
            perror("Could not create results file");
            MPI_Abort(communicator, 1);
        }

        fprintf(resultsFile, "Array Size,Random Limit,Method,"
                             "Workers,Time,Correct\n");

        fflush(resultsFile);

        printf("\nMPI C Benchmark\n");
        printf("Processes: %d\n", comm_sz);
    }

    for (int sizeId = 0; sizeId < 5; sizeId++)
    {
        for (int limitId = 0; limitId < 5; limitId++)
        {
            IntArray input;

            input.numbers = NULL;
            input.size = 0;

            int arraySize = arraySizes[sizeId];
            int randomLimit = randomLimits[limitId];

            if (myRank == 0)
            {
                input.size = arraySize;
                input.numbers = malloc(
                    arraySize * sizeof(int));

                srand(42);

                for (
                    int index = 0;
                    index < arraySize;
                    index++)
                {
                    input.numbers[index] =
                        rand() % (randomLimit + 1);
                }

                printf(
                    "\nRunning size %d, limit %d...\n",
                    arraySize,
                    randomLimit);

                fflush(stdout);
            }

            MPI_Barrier(communicator);


            double startTime = MPI_Wtime();

            IntArray sortedNumbers =
                mpiInsertionSort(
                    input,
                    communicator);

            double elapsedTime =
                MPI_Wtime() - startTime;

            MPI_Barrier(communicator);


            if (myRank == 0)
            {
                int correct = isSorted(
                    sortedNumbers);

                printf(
                    "Size: %10d | Limit: %9d | "
                    "Time: %.6f seconds | Correct: %s\n",
                    arraySize,
                    randomLimit,
                    elapsedTime,
                    correct ? "True" : "False");

                fprintf(
                    resultsFile,
                    "%d,%d,MPI C,%d,%.6f,%s\n",
                    arraySize,
                    randomLimit,
                    comm_sz,
                    elapsedTime,
                    correct ? "True" : "False");

                fflush(resultsFile);
                fflush(stdout);

                free(input.numbers);
                free(sortedNumbers.numbers);
            }
        }
    }

    if (myRank == 0)
    {
        fclose(resultsFile);
    }
}

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);

    int myRank;
    int comm_sz;

    MPI_Comm_rank(
        MPI_COMM_WORLD,
        &myRank);

    MPI_Comm_size(
        MPI_COMM_WORLD,
        &comm_sz);

    if (myRank == 0)
    {
        setvbuf(
            stdout,
            NULL,
            _IOLBF,
            0);
    }

    int sampleValues[20] = {
        81, 14, 3, 94, 35,
        31, 28, 17, 94, 13,
        86, 94, 69, 11, 75,
        54, 4, 27, 29, 64};

    IntArray sample;

    if (myRank == 0)
    {
        sample.numbers = sampleValues;
        sample.size = 20;

        printf("Original sample:\n");
        printArray(sample);
    }
    else
    {
        sample.numbers = NULL;
        sample.size = 0;
    }

    IntArray sortedSample =
        mpiInsertionSort(
            sample,
            MPI_COMM_WORLD);

    if (myRank == 0)
    {
        printf("\nMPI sorted sample:\n");
        printArray(sortedSample);

        printf(
            "\nMPI processes: %d\n",
            comm_sz);

        free(sortedSample.numbers);
    }

    runBenchmark(MPI_COMM_WORLD);

    MPI_Finalize();

    return 0;
}