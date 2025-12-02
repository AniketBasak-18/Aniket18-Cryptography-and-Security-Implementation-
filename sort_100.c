#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define SIZE 100
#define RUNS 10000

void generate_random_array(int arr[])
{
    for (int i = 0; i < SIZE; i++) 
    {
        arr[i] = rand() % 1000 + 1; // Random positive integers from 1 to 1000
    }
}

void copy_array(int dest[], int src[]) 
{
    for (int i = 0; i < SIZE; i++) 
    {
        dest[i] = src[i];
    }
}

// Different Sorting Algorithms
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
    int largest = i; // root
    int l = 2 * i + 1;
    int r = 2 * i + 2;

    if (l < n && arr[l] > arr[largest]) largest = l;
    if (r < n && arr[r] > arr[largest]) largest = r;

    if (largest != i) 
    {
        int tmp = arr[i];
        arr[i] = arr[largest];
        arr[largest] = tmp;
        heapify(arr, n, largest);
    }
}

void heap_sort(int arr[]) 
{
    for (int i = SIZE / 2 - 1; i >= 0; i--)
        heapify(arr, SIZE, i);

    for (int i = SIZE - 1; i > 0; i--) 
    {
        int tmp = arr[0];
        arr[0] = arr[i];
        arr[i] = tmp;
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

// Calculation of CPU cycle 

int main() {
    int original[SIZE], arr[SIZE];
    clock_t start, end;
    double bubble_total = 0, heap_total = 0, merge_total = 0, quick_total = 0;

    srand(time(NULL));

    for (int i = 0; i < RUNS; i++) {
        generate_random_array(original);

        // Bubble Sort
        copy_array(arr, original);
        start = clock();
        bubble_sort(arr);
        end = clock();
        bubble_total += (double)(end - start);

        // Heap Sort
        copy_array(arr, original);
        start = clock();
        heap_sort(arr);
        end = clock();
        heap_total += (double)(end - start);

        // Merge Sort
        copy_array(arr, original);
        start = clock();
        merge_sort(arr, 0, SIZE - 1);
        end = clock();
        merge_total += (double)(end - start);

        // Quick Sort
        copy_array(arr, original);
        start = clock();
        quick_sort(arr, 0, SIZE - 1);
        end = clock();
        quick_total += (double)(end - start);
    }

    printf("Average CPU cycles (approx):\n");
    printf("Bubble Sort: %.2f\n", bubble_total / RUNS);
    printf("Heap Sort  : %.2f\n", heap_total / RUNS);
    printf("Merge Sort : %.2f\n", merge_total / RUNS);
    printf("Quick Sort : %.2f\n", quick_total / RUNS);

    return 0;
}
