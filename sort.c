#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define MAX_SIZE 1000
#define RUNS 10000

#ifdef _WIN32                                            // 9 to 28 copied from ChatGPT
#include <windows.h>

double get_time_in_microseconds() 
{
    LARGE_INTEGER frequency, counter;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&counter);
    return (double)(counter.QuadPart * 1e6) / frequency.QuadPart;
}
#else
#include <time.h>

double get_time_in_microseconds() 
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1e6 + ts.tv_nsec / 1e3;
}
#endif

void generate_random_array(int arr[], int size) 
{
    for (int i = 0; i < size; i++) {
        arr[i] = rand() + 1;
    }
}

void copy_array(int dest[], int src[], int size)                          //src:= source, dest:= destination
{
    for (int i = 0; i < size; i++) 
    {
        dest[i] = src[i];
    }
}

// Bubble Sort
void bubble_sort(int arr[], int size) 
{
    for (int i = 0; i < size - 1; i++) 
    {
        for (int j = 0; j < size - i - 1; j++) 
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

void heap_sort(int arr[], int size) 
{
    for (int i = size / 2 - 1; i >= 0; i--)
        heapify(arr, size, i);

    for (int i = size - 1; i > 0; i--) 
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
int compare_doubles(const void *a, const void *b) 
{
    double diff = *(double *)a - *(double *)b;
    return (diff > 0) - (diff < 0);
}

void compute_and_print_stats(const char *sort_name, int size, double times[]) 
{
    double sum = 0;
    for (int i = 0; i < RUNS; i++) sum += times[i];

    qsort(times, RUNS, sizeof(double), compare_doubles);

    double avg = sum / RUNS;
    double min = times[0];
    double max = times[RUNS - 1];
    double median;

if (RUNS % 2 == 0) 
{
    // If number of elements is odd,then take average of the two middle elements
    int mid1 = RUNS / 2 - 1;
    int mid2 = RUNS / 2;
    median = (times[mid1] + times[mid2]) / 2.0;  // use 2.0 for correct floating point division
} 
else 
{
    // If number of elements is odd,then take the middle element
    int mid = RUNS / 2;
    median = times[mid];
}
    printf("%d,%s,%.0f,%.0f,%.2f,%.2f\n", size, sort_name, min, max, median, avg);
}

int main() 
{
    int original[MAX_SIZE], arr[MAX_SIZE];
    double times[RUNS];

    srand(time(NULL));
    printf("size,sort,min,max,median,avg\n");

    for (int size = 100; size <= 1000; size += 100) 
    {
        // Bubble Sort
        for (int i = 0; i < RUNS; i++) {
            generate_random_array(original, size);
            copy_array(arr, original, size);
            double start = get_time_in_microseconds();
            bubble_sort(arr, size);
            double end = get_time_in_microseconds();
            times[i] = end - start;
        }
        compute_and_print_stats("bubble", size, times);

        // Merge Sort
        for (int i = 0; i < RUNS; i++) 
        {
            generate_random_array(original, size);
            copy_array(arr, original, size);
            double start = get_time_in_microseconds();
            merge_sort(arr, 0, size - 1);
            double end = get_time_in_microseconds();
            times[i] = end - start;
        }
        compute_and_print_stats("merge", size, times);

        // Quick Sort
        for (int i = 0; i < RUNS; i++) 
        {
            generate_random_array(original, size);
            copy_array(arr, original, size);
            double start = get_time_in_microseconds();
            quick_sort(arr, 0, size - 1);
            double end = get_time_in_microseconds();
            times[i] = end - start;
        }
        compute_and_print_stats("quick", size, times);

        // Heap Sort
        for (int i = 0; i < RUNS; i++) 
        {
            generate_random_array(original, size);
            copy_array(arr, original, size);
            double start = get_time_in_microseconds();
            heap_sort(arr, size);
            double end = get_time_in_microseconds();
            times[i] = end - start;
        }
        compute_and_print_stats("heap", size, times);
    }

    return 0;
}
