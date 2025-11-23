#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define SIZE 1000
#define RUNS 10000

void generate_random_array(int arr[]) 
{
    for (int i = 0; i < SIZE; i++) 
    {
        arr[i] = rand() + 1;
    }
}

void copy_array(int dest[], int src[])              // src:= source, dest:= destination
{              
    for (int i = 0; i < SIZE; i++) 
    {
        dest[i] = src[i];
    }
}

// Bubble Sort
void bubble_sort(int arr[]) 
{
    for (int i = 0; i < SIZE - 1; i++) 
    {
        for (int j = 0; j < SIZE - i - 1; j++) 
        {
            if (arr[j] > arr[j + 1]) 
            {
                int t = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = t;
            }
        }
    }
}

// Heap Sort
void heapify(int arr[], int n, int i) 
{
    int largest = i, l = 2 * i + 1, r = 2 * i + 2;

    if (l < n && arr[l] > arr[largest]) largest = l;
    if (r < n && arr[r] > arr[largest]) largest = r;

    if (largest != i) 
    {
        int tmp = arr[i]; arr[i] = arr[largest]; arr[largest] = tmp;
        heapify(arr, n, largest);
    }
}

void heap_sort(int arr[]) 
{
    for (int i = SIZE / 2 - 1; i >= 0; i--)
        heapify(arr, SIZE, i);

    for (int i = SIZE - 1; i > 0; i--) 
    {
        int tmp = arr[0]; arr[0] = arr[i]; arr[i] = tmp;
        heapify(arr, i, 0);
    }
}

// Merge Sort
void merge(int arr[], int l, int m, int r) 
{
    int n1 = m - l + 1, n2 = r - m;
    int L[n1], R[n2];

    for (int i = 0; i < n1; i++) L[i] = arr[l + i];
    for (int j = 0; j < n2; j++) R[j] = arr[m + 1 + j];

    int i = 0, j = 0, k = l;
    while (i < n1 && j < n2)
        arr[k++] = (L[i] <= R[j]) ? L[i++] : R[j++];
    while (i < n1) arr[k++] = L[i++];
    while (j < n2) arr[k++] = R[j++];
}

void merge_sort(int arr[], int l, int r) 
{
    if (l < r) 
    {
        int m = l + (r - l) / 2;
        merge_sort(arr, l, m);
        merge_sort(arr, m + 1, r);
        merge(arr, l, m, r);
    }
}

// Quick Sort
int partition(int arr[], int low, int high) 
{
    int pivot = arr[high], i = low - 1;
    for (int j = low; j < high; j++) 
    {
        if (arr[j] < pivot) 
        {
            i++;
            int tmp = arr[i]; arr[i] = arr[j]; arr[j] = tmp;
        }
    }
    int tmp = arr[i + 1]; arr[i + 1] = arr[high]; arr[high] = tmp;
    return i + 1;
}

void quick_sort(int arr[], int low, int high) 
{
    if (low < high) 
    {
        int loc = partition(arr, low, high);
        quick_sort(arr, low, loc - 1);
        quick_sort(arr, loc + 1, high);
    }
}

// Comparator for qsort
int compare_doubles(const void *a, const void *b) {
    double diff = *(double *)a - *(double *)b;
    return (diff > 0) - (diff < 0);
}

void compute_stats(const char *name, double times[]) {
    double sum = 0;
    for (int i = 0; i < RUNS; i++) sum += times[i];

    qsort(times, RUNS, sizeof(double), compare_doubles);

    double average = sum / RUNS;
    double min = times[0];
    double max = times[RUNS - 1];
    double median;                                           // calculate median

if (RUNS % 2 == 0) {
    // If number of element is even, then average the two middle elements
    int mid1 = RUNS / 2 - 1;
    int mid2 = RUNS / 2;
    median = (times[mid1] + times[mid2]) / 2.0;  // use 2.0 for correct floating point division
} else {
    // If number of element is odd, take the middle element
    int mid = RUNS / 2;
    median = times[mid];
}

    printf("\n%s Sort:\n", name);
    printf("Average: %.2f clock ticks\n", average);
    printf("Minimum: %.2f clock ticks\n", min);
    printf("Maximum: %.2f clock ticks\n", max);
    printf("Median : %.2f clock ticks\n", median);
}

int main() {
    int original[SIZE], arr[SIZE];
    clock_t start, end;
    double bubble_times[RUNS], heap_times[RUNS], merge_times[RUNS], quick_times[RUNS];

    srand(time(NULL));

    for (int i = 0; i < RUNS; i++) {
        generate_random_array(original);

        // Bubble Sort
        copy_array(arr, original);
        start = clock();
        bubble_sort(arr);
        end = clock();
        bubble_times[i] = (double)(end - start);

        // Heap Sort
        copy_array(arr, original);
        start = clock();
        heap_sort(arr);
        end = clock();
        heap_times[i] = (double)(end - start);

        // Merge Sort
        copy_array(arr, original);
        start = clock();
        merge_sort(arr, 0, SIZE - 1);
        end = clock();
        merge_times[i] = (double)(end - start);

        // Quick Sort
        copy_array(arr, original);
        start = clock();
        quick_sort(arr, 0, SIZE - 1);
        end = clock();
        quick_times[i] = (double)(end - start);
    }

    printf("\nCPU Clock Tick Statistics (based on %d runs, SIZE = %d):\n", RUNS, SIZE);

    compute_stats("Bubble", bubble_times);
    compute_stats("Heap", heap_times);
    compute_stats("Merge", merge_times);
    compute_stats("Quick", quick_times);

    return 0;
}
