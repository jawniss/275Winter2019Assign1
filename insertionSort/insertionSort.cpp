

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

/*
// swap function from eclass quicksort.cpp that swaps two inputs
void swap(int* a,int* b){
	int t = *a;
	*a = *b;
	*b = t;
}

// working i sort
void isort(RestDist* dist,int len){
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
*/


int main(){
  int n;
  int array[1067];
  cin >> n;
  for(int i = 0; i < n; i++){
    cin >> array[i];
  }
  // for the case where an array of 1 element skip sort step
  if (n != 1){
    isort(array,n);
  }
  // output the sorted array
  for (int j = 0; j < n; j++){
    cout << array[j] << " ";
  }
  return 0;
}
