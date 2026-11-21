第4次提交
#include <stdio.h>
#include <stdlib.h>

void getArray(int b[],int n)
{
    int i;
    for(i = 0;i < n;i++)
        scanf("%d",&b[i]);
}

void getArray1(int b[],int n)
{
    int i;
    int *p;
    p = b;
    for(i = 0;i < n;i++)
        scanf("%d",&(*(p+i)));
}
void pntArray(int b[],int n)
{
    int i;
    for(i = 0;i < n;i++)
        printf("%4d",b[i]);
        printf("\n");
}
void BSort(int a[],int n)
{
    int i,j,t;
    int temp;
    for(i = n;i >= 1;i--)
    {
        for(j = 0;j < j;j++)
        {
            if(a[j] > a[j+1])
            {
                temp = a[j];
                a[j] = a[j+1];
                a[j+1] = temp;
            }
        }
    }
}

void SelectSort(int a[],int n)
{
    int i,j;
    int temp,index;
    for(i = 0;i < n-1;i++)
    {
        index = i;
        for(j = i+1;j < n;j++)
        {
            if(a[j] < a[index])
                index = j;
        }
        temp = a[index];
        a[index] = a[i];
        a[i] = temp;
    }
}


int main()
{
    int a[10];
    getArray(a,10);
    printf("排序前：");
    pntArray(a,10);
    //BSort(a,10);
    SelectSort(a,10);
    printf("排序后：");
    pntArray(a,10);
}