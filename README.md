# Procedure:
1.) On startup the LPC4088 digital signal controller will signal to the LCD display to initialize.
2.) The 4x4 matrix keypad columns are set to high to allow for row scanning and the temperature sensor is initialized.
3.) The LCD display will then display the alphabet all in uppercase split between the first two rows, and then the entire alphabet all in lowercase split           between the last two rows. Lastly all values from 0-9 will be displayed from increasing order on the first row and in decreasing order on the second row.
4.) The LCD display will then prompt the user to enter the current date and time using the keypad. I2C serial communication is used to interface the keypad and     the real-time clock.
5.) The LCD display will then prompt the user to enter a date and time for an alarm using the keypad. Again I2C serial communication is used to do this.
6.) The program then begins in "normal mode" where the I2C serial communication is used to display the current the date and time from the real-time clock and       the current temperature in celsius and fahrenheit. Additionally, if the current date and time align with the alarm date and time, the alarm date and time       will be displayed along with an LED blinking to signal that the alarm is going off.
7.) When in "normal mode" if the "F" key is pressed on the matrix keypad the program will transition to "calculator mode"; where addition, subtraction,             multiplication and division operations can be used to and where the operations and results are displayed on the LCD display. Similarly, when in "calculator     mode" the user can once again press the "F" key on the 4x4 matrix keypad to switch back to "normal mode".

# Project Circuit:
![Project%20Circuit](https://github.com/jwrhone/embedded_system_project/blob/main/Project.jpg)
