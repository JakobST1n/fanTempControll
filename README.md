# fanTempControll
This simple arduino-script is made to run on a attiny85 with the official arduino core. It basically controlls a pwm-signal based on the readings of a DS18x-temperature-sensor.

it basically follows this curve:
```
f(x) = 7.8x + 135
```
It cuts the fan under 25 degress.


It beeps with four short beeps if it hasnt been able to get a reading from the sensor for 10 seconds.

It makes a "alarm" sound (long C, short F#, long C, short F#), if the temperature is above 65 degrees.

It prints to a serialport (TX pin 4), the debug lines is pretty self-explanatory. ONE LINE = ONE RUN THROUGH THE LOOP FUNCTION.

Other than that? Have fun, i haven't made any schematics or anything, just because all use-cases will be so different. I used this for a KORAD KA3005d power supply, which a very smart guy on youtube already have a tutorial on. And the chip and circuit is sp simple that you should be fine anyways. I have a npn controlling the ground pin on my fan, a small piexo element directly connected, and the sensor with a 4.7k resistor.
