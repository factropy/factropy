( Example: Send a signal periodically )

( The in-game computer entity is a stack machine programmed with reverse-polish notation. )
( https://en.wikipedia.org/wiki/Reverse_Polish_notation )

( If you have a programming background from before the insane modern world of Javascript you )
( may recognise a dialect of Forth, or have come across something similar on old calculators. )

( The archtecture is: )
( * 32 words of RAM addressed 0-31 )
( * 32 words of ROM addressed 32-63 )
( * 60 CPU instruction cycles per tick
( * Environment variables in the game window become key/value pairs start at ROM #0 )
( * String literals in code are packed into ROM as counted strings [size][characters...]

( Period in seconds, 10 min. )
: period 300 ;

( Environment variable #1 key at ROM address #0: the signal to send. )
: signal rom @ ;

( Environment variable #1 value at ROM address #1: the count to send. )
: value rom 1 + @ ;

( Function to send the defined signal on the first network interface. )
: trigger value nic0 signal send ;

( Use modulo division to compare the number of seconds since the game started and the )
( period, leaving the remainder on the stack. )
now period % dup ram !

( If the remainder on the stack is 0 then then "now" is a multiple of the period, so send )
( the signal. Since this program executes every tick it will keep sending for 1 second. )
0 = if trigger then
"hi" print