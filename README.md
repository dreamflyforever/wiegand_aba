## wiegand_aba
The linux dreiver is for door card identification, it supports wiegand-26 & ABA 
format automatically.
It is difficult to read the ABA	data because it comes time between 50ms, so the
driver use tasklet method, fetch data in the interrupt upper half, and logic handle
in the lower half, then perfectly solve the problem.

## compile
It depends on linux-2.6.30.4 kernel library in my project.  
make; sudo insmod readcard_auto.ko;

## author
GPL by Jim
Anyway, sorry for the program use magic number.
