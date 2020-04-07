// C program to multiply two square matrices. 
#include <stdio.h> 
#define DIM 4 
  
void mat_mul(int mat1[][DIM], int mat2[][DIM], int result[][DIM]) 
{ 
    int i, j, k; 
    for (i = 0; i < DIM; i++) 
    { 
        for (j = 0; j < DIM; j++) 
        { 
            result[i][j] = 0; 
            for (k = 0; k < DIM; k++) 
                result[i][j] += mat1[i][k]*mat2[k][j]; 
        } 
    } 
} 
  
int main() 
{ 
    int mat1[DIM][DIM] = { {10, 10, 10, 10}, 
                    {20, 20, 20, 20}, 
                    {30, 30, 30, 30}, 
                    {40, 40, 40, 40}}; 
  
    int mat2[DIM][DIM] = { {1, 1, 1, 1}, 
                    {2, 2, 2, 2}, 
                    {3, 3, 3, 3}, 
                    {4, 4, 4, 4}}; 
  
    int result[DIM][DIM];
    int i, j; 
    mat_mul(mat1, mat2, result); 	//Calls Multiplication of Matrixs
    /*
 		printf("Result matrix is \n"); 
		for (i = 0; i < DIM; i++) 
		{ 
		    for (j = 0; j < DIM; j++) 
		       printf("%d ", result[i][j]); 
		    printf("\n"); 
		} 
 	*/
    return 0; 
} 
