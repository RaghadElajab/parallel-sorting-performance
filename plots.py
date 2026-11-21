import matplotlib.pyplot as plt


def readResults(fileName):
    results = []

    with open(fileName, "r") as resultsFile:
        lines = resultsFile.readlines()

    # Skip the CSV heading
    for line in lines[1:]:
        values = line.strip().split(",")

        if len(values) == 6:
            results.append(
                {
                    "arraySize": int(values[0]),
                    "randomLimit": int(values[1]),
                    "time": float(values[4]),
                }
            )

    return results


# Read every method's CSV file
methodResults = {
    "Sequential C": readResults(
        "sequential_c_results.csv"
    ),
    "Pthreads": readResults(
        "multithreading_results.csv"
    ),
    "OpenMP": readResults(
        "openmp_results.csv"
    ),
    "MPI C": readResults(
        "mpi_c_results.csv"
    ),
}

arraySizes = [
    1000,
    10000,
    100000,
    1000000,
    10000000,
]

randomLimits = [
    100,
    1000,
    10000,
    100000,
    1000000,
]

# Use equally spaced labels instead of a logarithmic x-axis
limitLabels = [
    "100",
    "1K",
    "10K",
    "100K",
    "1M",
]

figure, graphs = plt.subplots(
    2,
    3,
    figsize=(18, 10),
)

# Convert the 2D graph array into one list
graphs = graphs.flatten()

for graphId in range(len(arraySizes)):
    graph = graphs[graphId]
    arraySize = arraySizes[graphId]

    for methodName in methodResults:
        times = []

        for randomLimit in randomLimits:
            elapsedTime = None

            for result in methodResults[methodName]:
                if (
                    result["arraySize"] == arraySize
                    and result["randomLimit"] == randomLimit
                ):
                    elapsedTime = result["time"]
                    break

            times.append(elapsedTime)

        graph.plot(
            limitLabels,
            times,
            marker="o",
            linewidth=2,
            label=methodName,
        )

    graph.set_title(
        f"Array Size: {arraySize:,}"
    )

    graph.set_xlabel(
        "Random Integer Limit"
    )

    graph.set_ylabel(
        "Execution Time (seconds)"
    )

    graph.grid(
        True,
        linestyle="--",
        alpha=0.5,
    )

# Remove the unused sixth graph
figure.delaxes(graphs[5])

# Create one shared legend for the complete figure
handles, labels = graphs[0].get_legend_handles_labels()

figure.legend(
    handles,
    labels,
    loc="lower center",
    ncol=4,
)

figure.suptitle(
    "Insertion Sort Performance Comparison",
    fontsize=16,
)

plt.tight_layout(
    rect=[0, 0.07, 1, 0.95]
)

plt.savefig(
    "all_methods_comparison.png",
    dpi=300,
)

plt.show()