#include <stdio.h>
#include <time.h>

int dropMessage(float p)
{
    float random_number = (float)(rand() % 2) / 2;
    printf("RN : %f, ", random_number);
    // Check if the generated number is less than p
    if (random_number < p)
    {
        return 1; // Return 1 if success
    }
    else
    {
        return 0; // Return 0 if failure
    }
}

int main()
{
    int i = 0;
    srand(time(NULL));
    while (i < 1000)
    {
        /* code */
        printf("%d \n", dropMessage(0.5));
        i++;
    }
}