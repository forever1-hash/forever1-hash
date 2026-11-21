第3次提交
#include <stdio.h>
#include <stdlib.h>

void getArray(int b[],int n)
{
    int i;
    for(i = 0;i < n;i++)
        scanf("%d",&b[i]);
}
void pntArray(int b[],int n)
{
    int i;
    for(i = 0;i < n;i++)
        printf("%4d",b[i]);
        printf("\n");
}

int main()
{
    int n;
    int i;
    int a[MAX];
    scanf("%d",&n);
    for(i = 0;i < n;i++)
        a[i] = i;
    for(i = 0;i < n;i++)
        printf("%d",a[i]);
    printf("\n");
    for(i = 0;i < n;i++)
        scanf("%d",&a[i]);
    for(i = 0;i < n;i++)
        printf("%d",a[i]);

    int a[5],i;
    getArray(a,5);
    for(i = 5;i >= 0;i--)
        pntArray(a,i);
    return 0;
}