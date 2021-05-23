# Clock / Timer for 7 segment TM1637 display

This code uses the code found at https://github.com/wahlencraft/TM1637-pico to control the TM1637 display via PIO.
Many thanks to wahlencraft.

This project turns a 7 segment TM1637 display into a simple clock and timer.

This project require the Pi Pico, display, four switches and a piezo buzzer.

The buttons work as follows:

Whilst in the menu:

- A - Back out of the menu or move backwards
- B - Select and move forwards
- X - Up
- Y - Down

When not in a menu:

- A - Brings up the menu
- B - Pause / un-pause the timer (if set)
- X - Show Clock
- Y - Show Timer


When booted up you will be asked for the current time. 
Use the Up and Down buttons to set the hour.
Then use the B button to move forward to the minute part.
Use the Up and Down buttons to set the minute.
If you have made a mistake then use the A button to go back to the previous part.
Otherwise press the B button to set the time.

The time will now be displayed on the device. It defaults to a simple clock.

To set a timer:

Press the A button.
Select the 'Set Timer' option by using the X and Y buttons to get to the timer option and then the B button to select.
Set the timer in the same way as you set the clock.

The timer will now be set.

To pause / un-pause the timer press the B button.

To see the actual timer press the Y button.
To go back to the clock press the X button.

