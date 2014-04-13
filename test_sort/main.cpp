
#include <iostream>                               
#include <fstream>

#include <stdio.h>
#include <stdlib.h>

#define uint64_t unsigned long long
using namespace std;

int checkOrder(FILE* f);

int main(){
	int unsorted;

	FILE * f;
	//f = fopen("test_rand" , "rb" );
	f = fopen("test_sort" , "rb" );

	unsorted = checkOrder(f);

    fclose(f);

	if (unsorted != 0)
		cout << "List is unsorted" << endl;
	else
		cout << "List is sorted" << endl;

	return 0;                                      
}

//Checks if the file is in ascending ordering
//return 0 if sorted ascending
int checkOrder(FILE* f)
{
	uint64_t last = 0;
	uint64_t current = 0;
	int i = 0;
	int size;
	int unsorted = 0;

	size_t result = 0;



	if (f == NULL) {
		cerr << "File error" << endl;
		return 1;
	} 

	//get filesize
	fseek (f , 0 , SEEK_END);
	size = ftell (f);
	rewind (f);

	//check min 2 numbers
	if (size < 16)
	{
		cerr << "Need min 2 numbers to check" << endl;
		return 1;
	}


	//read first
	result += fread(&last,1,8,f);

	//read
	while (result < size)
	{
		last = current;
		result += fread(&current,1,8,f);

		if (last > current)
		{
			unsorted = 1;
			break;
		}

		//cout << ++i << ":" << current << endl;
	}

	return unsorted;
}