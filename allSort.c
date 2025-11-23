#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>
#include <string.h>

long long comparisons = 0;
long long swaps = 0;

typedef struct {
    long long min_comparisons;
    long long max_comparisons;
    long long avg_comparisons;
    long long min_swaps;
    long long max_swaps;
    long long avg_swaps;
} Statistics;

// Function prototypes
void bubbleSort(int arr[], int n);
void selectionSort(int arr[], int n);
void insertionSort(int arr[], int n);
void mergeSort(int arr[], int left, int right);
void merge(int arr[], int left, int mid, int right);
void quickSort(int arr[], int low, int high);
int partition(int arr[], int low, int high);
void heapSort(int arr[], int n);
void heapify(int arr[], int n, int i);

void initializeArray(int arr[], int size);
void resetCounters();
Statistics runAlgorithmTest(const char* algorithmName, int size);

int main() {
    srand(time(NULL));
    
    printf("Comprehensive Sorting Algorithm Analysis\n");
    printf("=======================================\n");
    printf("Array sizes: 100, 200, 300, ..., 1000\n");
    printf("Each test runs 10000 iterations\n");
    printf("Tracking: Min, Max, Average comparisons and swaps\n\n");
    
    int sizes[] = {100, 200, 300, 400, 500, 600, 700, 800, 900, 1000};
    int numSizes = 10;
    
    FILE* file = fopen("detailed_results.csv", "w");
    fprintf(file, "Algorithm,Size,Min_Comparisons,Max_Comparisons,Avg_Comparisons,Min_Swaps,Max_Swaps,Avg_Swaps\n");
    
    const char* algorithms[] = {"Bubble Sort", "Selection Sort", "Insertion Sort", "Merge Sort", "Quick Sort", "Heap Sort"};
    int numAlgorithms = 6;
    
    for (int i = 0; i < numSizes; i++) {
        int size = sizes[i];
        printf("\nTesting array size: %d\n", size);
        printf("-------------------\n");
        
        for (int j = 0; j < numAlgorithms; j++) {
            printf("Running %s (10000 iterations)...", algorithms[j]);
            fflush(stdout);
            
            Statistics stats = runAlgorithmTest(algorithms[j], size);
            
            printf(" Done!\n");
            printf("  Comparisons - Min: %lld, Max: %lld, Avg: %lld\n", 
                   stats.min_comparisons, stats.max_comparisons, stats.avg_comparisons);
            printf("  Swaps       - Min: %lld, Max: %lld, Avg: %lld\n", 
                   stats.min_swaps, stats.max_swaps, stats.avg_swaps);
            
            // Write to CSV file
            fprintf(file, "%s,%d,%lld,%lld,%lld,%lld,%lld,%lld\n",
                    algorithms[j], size,
                    stats.min_comparisons, stats.max_comparisons, stats.avg_comparisons,
                    stats.min_swaps, stats.max_swaps, stats.avg_swaps);
        }
    }
    
    fclose(file);
    printf("\n===========================================\n");
    printf("Analysis Complete!\n");
    printf("Results saved to 'detailed_results.csv'\n");
    printf("Run 'python plot_graphs.py' to generate graphs\n");
    printf("===========================================\n");
    
    return 0;
}

// Function to run 1000 iterations and calculate min, max, avg
Statistics runAlgorithmTest(const char* algorithmName, int size) {
    Statistics stats;
    stats.min_comparisons = LLONG_MAX;
    stats.max_comparisons = 0;
    stats.min_swaps = LLONG_MAX;
    stats.max_swaps = 0;
    
    long long totalComparisons = 0;
    long long totalSwaps = 0;
    
    // Run 1000 iterations
    for (int iteration = 0; iteration < 10000; iteration++) {
        int* arr = malloc(size * sizeof(int));
        initializeArray(arr, size);
        resetCounters();
        
        // Call appropriate sorting algorithm
        if (strcmp(algorithmName, "Bubble Sort") == 0) {
            bubbleSort(arr, size);
        } else if (strcmp(algorithmName, "Selection Sort") == 0) {
            selectionSort(arr, size);
        } else if (strcmp(algorithmName, "Insertion Sort") == 0) {
            insertionSort(arr, size);
        } else if (strcmp(algorithmName, "Merge Sort") == 0) {
            mergeSort(arr, 0, size - 1);
        } else if (strcmp(algorithmName, "Quick Sort") == 0) {
            quickSort(arr, 0, size - 1);
        } else if (strcmp(algorithmName, "Heap Sort") == 0) {
            heapSort(arr, size);
        }
        
        // Update statistics
        if (comparisons < stats.min_comparisons) stats.min_comparisons = comparisons;
        if (comparisons > stats.max_comparisons) stats.max_comparisons = comparisons;
        if (swaps < stats.min_swaps) stats.min_swaps = swaps;
        if (swaps > stats.max_swaps) stats.max_swaps = swaps;
        
        totalComparisons += comparisons;
        totalSwaps += swaps;
        
        free(arr);
    }
    
    // Calculate averages
    stats.avg_comparisons = totalComparisons / 10000;
    stats.avg_swaps = totalSwaps / 10000;
    
    return stats;
}

// Bubble Sort
void bubbleSort(int arr[], int n) {
    for (int i = 0; i < n - 1; i++) {
        for (int j = 0; j < n - i - 1; j++) {
            comparisons++;  
            if (arr[j] > arr[j + 1]) {
                // Swap elements
                swaps++; 
                int temp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = temp;
            }
        }
    }
}

// Selection Sort
void selectionSort(int arr[], int n) {
    for (int i = 0; i < n - 1; i++) {
        int min_index = i;
        
        for (int j = i + 1; j < n; j++) {
            comparisons++;  // Count comparison
            if (arr[j] < arr[min_index]) {
                min_index = j;
            }
        }
        
        if (min_index != i) {
            swaps++;  // Count swap
            int temp = arr[min_index];
            arr[min_index] = arr[i];
            arr[i] = temp;
        }
    }
}

// Insertion Sort
void insertionSort(int arr[], int n) {
    for (int i = 1; i < n; i++) {
        int key = arr[i];
        int j = i - 1;
        
        while (j >= 0) {
            comparisons++;
            if (arr[j] > key) {
                swaps++;
                arr[j + 1] = arr[j];
                j--;
            } else {
                break;
            }
        }
        arr[j + 1] = key;
    }
}

// Merge Sort
void merge(int arr[], int left, int mid, int right) {
    int n1 = mid - left + 1;
    int n2 = right - mid;
    
    int* L = malloc(n1 * sizeof(int));
    int* R = malloc(n2 * sizeof(int));
    
    for (int i = 0; i < n1; i++)
        L[i] = arr[left + i];
    for (int j = 0; j < n2; j++)
        R[j] = arr[mid + 1 + j];
    
    int i = 0, j = 0, k = left;
    
    while (i < n1 && j < n2) {
        comparisons++;
        if (L[i] <= R[j]) {
            arr[k] = L[i];
            i++;
        } else {
            arr[k] = R[j];
            j++;
        }
        swaps++;
        k++;
    }
    
    while (i < n1) {
        arr[k] = L[i];
        swaps++;
        i++;
        k++;
    }
    
    while (j < n2) {
        arr[k] = R[j];
        swaps++;
        j++;
        k++;
    }
    
    free(L);
    free(R);
}

void mergeSort(int arr[], int left, int right) {
    if (left < right) {
        int mid = left + (right - left) / 2;
        mergeSort(arr, left, mid);
        mergeSort(arr, mid + 1, right);
        merge(arr, left, mid, right);
    }
}

// Quick Sort - Divide and conquer algorithm
int partition(int arr[], int low, int high) {
    int pivot = arr[high];
    int i = low - 1;
    
    for (int j = low; j < high; j++) {
        comparisons++;
        if (arr[j] < pivot) {
            i++;
            swaps++;
            int temp = arr[i];
            arr[i] = arr[j];
            arr[j] = temp;
        }
    }
    

    swaps++; 
    int temp = arr[i + 1];
    arr[i + 1] = arr[high];
    arr[high] = temp;
    
    return i + 1;
}

void quickSort(int arr[], int low, int high) {
    if (low < high) {
        int pi = partition(arr, low, high);
        quickSort(arr, low, pi - 1);
        quickSort(arr, pi + 1, high);
    }
}


void heapify(int arr[], int n, int i) {
    int largest = i;
    int left = 2 * i + 1;
    int right = 2 * i + 2;
    
    if (left < n) {
        comparisons++;
        if (arr[left] > arr[largest])
            largest = left;
    }
    
    if (right < n) {
        comparisons++;
        if (arr[right] > arr[largest])
            largest = right;
    }
    
    if (largest != i) {
        swaps++;
        int temp = arr[i];
        arr[i] = arr[largest];
        arr[largest] = temp;
        heapify(arr, n, largest);
    }
}

void heapSort(int arr[], int n) {
    // Build max heap
    for (int i = n / 2 - 1; i >= 0; i--)
        heapify(arr, n, i);
    
    // Extract elements from heap one by one
    for (int i = n - 1; i > 0; i--) {
        swaps++;
        int temp = arr[0];
        arr[0] = arr[i];
        arr[i] = temp;
        heapify(arr, i, 0);
    }
}

void initializeArray(int arr[], int size) {
    for (int i = 0; i < size; i++) {
        arr[i] = rand() ;  // Random numbers from 0 to 9999
    }
}

void resetCounters() {
    comparisons = 0;
    swaps = 0;
}