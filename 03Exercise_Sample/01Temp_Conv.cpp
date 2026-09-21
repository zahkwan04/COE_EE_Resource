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
    float temp_value = 0;

    do{
        printf("\n===== Celsius <-> Fahrenheit Converter ======");

        /*ask user for input*/
        printf("\nSelect option below:");
        printf("\n1. Celsius to Fahrenheit");
        printf("\n2. Fahrenheit to Celsius");
        printf("\n3. Exit program\n");
        printf("\nYour selection:");
        scanf("%d", &user_option);

        if(user_option <1 || user_option > 3 ){
        printf("\nInvalid option! Choose between 1-3.");
        continue;
        }

        switch(user_option){
            case 1:
            printf("\n======== Celcius -> Fahrenheit ==========");
            printf("\nEnter the celsius value: ");
            scanf("%f", &temp_value);
            printf("\n[%.2f Celsius is equal to %.2f Fahrenheit]\n", temp_value, celsius_to_fahrenheit(temp_value));
            break;

            case 2:
            printf("\n====== Fahrenheit -> Celcius =========");
            printf("\nEnter the Fahrenheit value: ");
            scanf("%f", &temp_value);
            printf("\n[%.2f Fahrenheit is equal to %.2f Celsius]\n", temp_value, fahrenheit_to_celsius(temp_value));
            break;

        }

    }while(user_option != 3);

    printf("\n======Bye Bye!======\n");

    return 0;
}


float celsius_to_fahrenheit(float c){
    float fahrenheit = 0;

    /** Multiply 1.8 and then + 32 **/
    fahrenheit = (c * 1.8) + 32;
    
    return fahrenheit;
}


float fahrenheit_to_celsius(float f){
    float celsius = 0;

    /** subtract 32 , multiply 5, divide 9 **/
    celsius = (f - 32) * 5 / 9;
    
    return celsius;
}