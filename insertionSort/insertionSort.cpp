

#include <iostream>
#include <cmath>

using namespace std;

// swap function from eclass quicksort.cpp that swaps two inputs
void swap(int* a,int* b){
	int t = *a;
	*a = *b;
	*b = t;
}

// working i sort
void isort(int array[],int lenArray){
  int i;
  int j;
  i = 1;
  while (i < lenArray){
    j = i;
    while (( j>0 ) && (array[j-1] > array[j])){
      swap(array[j],array[j-1]);
      j = j-1;
    }
    i = i+1;
  }
}

/* This function takes last element as pivot, places
the pivot element at its correct position in sorted
	array, and places all smaller (smaller than pivot)
to left of pivot and all greater elements to right
of pivot */
int partition (RestDist dist[], int low, int high)
{
	int pivot = dist[high].dist; // pivot
	int i = (low - 1); // Index of smaller element

	for (int j = low; j <= high- 1; j++)
	{
		// If current element is smaller than or
		// equal to pivot
		if (dist[j].dist <= pivot)
		{
			i++; // increment index of smaller element
			swap(dist[i], dist[j]);
		}
	}
	swap(dist[i + 1], dist[high]);
	return (i + 1);
}

/* The main function that implements QuickSort
arr[] --> Array to be sorted,
low --> Starting index,
high --> Ending index */
void quickSort(RestDist dist[], int low, int high)
{ 
	if (low < high)
	{
		/* pi is partitioning index, arr[p] is now
		at right place */
		int pi = partition(dist[], low, high);

		// Separately sort elements before
		// partition and after partition
		quickSort(dist[], low, pi - 1);
		quickSort(dist[], pi + 1, high);
	}
}


int main(){
	int ln, lt;
	for (int i = 0; i < 1067; i++) {
    getRestaurantFast(i, &rest);
    ln = lon_to_x(rest.lon);
    lt = lat_to_y(rest.lat);
    rest_dist[i].dist = manhatten(xposcursor, ln, yposcursor, lt);
    rest_dist[i].index = i;
  }
  return 0;
}
