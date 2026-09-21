/**Level 1: Breaking the "Python Brain" (Syntax & Logic)
Goal: Get used to static typing, compilation, and basic control flow.
Exercise 1: The Temperature Converter
The Task: Write a program that asks the user for a temperature in Celsius and converts it to Fahrenheit. Then, ask for Fahrenheit and convert to Celsius.
The Catch: You must write two separate functions: celsius_to_fahrenheit(float c) and fahrenheit_to_celsius(float f).
What you learn: Basic data types (float, int), printf/scanf formatting, and how to write/compile functions in C. **/  

#include <stdio.h>

float celsius_to_fahrenheit(float c);
float fahrenheit_to_celsius(float f);


int main(){
    int user_option;
    float temp_value = 0.0f;

    do{
        printf("\n===== Celsius <-> Fahrenheit Converter ======");

        /*ask user for input*/
        printf("\nSelect option below:");
        printf("\n1. Celsius to Fahrenheit");
        printf("\n2. Fahrenheit to Celsius");
        printf("\n3. Exit program\n");
        printf("\nYour selection:");

        if(scanf("%d", &user_option) != 1){
            printf("\n[ERROR] Invalid input! Please enter a number (1-3).\n");
            while(getchar()!='\n');
            continue;
        }

        switch(user_option){
            case 1:
            printf("\n======== Celcius -> Fahrenheit ==========");
            printf("\nEnter the celsius value: ");
            /** Guard against bad float input**/
            while(scanf("%f", &temp_value) != 1){
                printf("Invalid temperature! Enter a valid number: ");
                while(getchar() != '\n');
            }
            printf("\n[%.2f Celsius is equal to %.2f Fahrenheit]\n", temp_value, celsius_to_fahrenheit(temp_value));
            break;

            case 2:
            printf("\n====== Fahrenheit -> Celcius =========");
            printf("\nEnter the Fahrenheit value: ");
            /** Guard against bad float input**/
            while(scanf("%f", &temp_value) != 1){
                printf("Invalid temperature! Enter a valid number: ");
                while(getchar() != '\n');
            }
            printf("\n[%.2f Fahrenheit is equal to %.2f Celsius]\n", temp_value, fahrenheit_to_celsius(temp_value));
            break;

            case 3:
            printf("\n======Bye Bye!======\n");
            break;

            default:
            printf("\n[ERROR] Invalid option! Choose between 1-3. \n");
            break;
        }

    }while(user_option != 3); 
    return 0;
}


float celsius_to_fahrenheit(float c){
    /** Multiply 1.8 and then + 32 **/
    return (c * 1.8) + 32;
}


float fahrenheit_to_celsius(float f){
    /** subtract 32 , multiply 5, divide 9 **/
    return (f - 32) * 5 / 9;
}