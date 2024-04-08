# Make uses sh, not bash, so we need to install the hawk and hawklink commands;
# these definitions are lifted from the aliases in .bashrc
# (If you try to use this makefile on a different system, you will need
# to change the file names to where hawk and hawklink are installed there.)
smal=~/maison/hawk/smal32 -P 45 -U ~/maison/hawk -L
hawklink=~/maison/hawk/hawklink -m -o ripples ~/maison/hawk/monitor.o
hawk=~/maison/graceful_hawk/hawk

##########
# primary make target

whatever.o: main.o canvas.o display.o move_objects.o
	$(hawklink) main.o move_objects.o canvas.o display.o &&	$(hawk)	ripples.o

##########
# other make targets
main.o: main.a
	$(smal) main.a

move_objects.o: move_objects.a
	$(smal) move_objects.a

canvas.o: canvas.a
	$(smal) canvas.a

display.o: display.a
	$(smal) display.a

##########
# Make utilities

# demonsrate the program
run: ripples.o
	$(hawk)	ripples.o

# remove object and listing files
clean:
	rm -f *.o *.l

#create a shell archive of the project
stackdemo.shar README Makefile main.a stack.a stack.h:
	shar README Makefile main.a stack.a stack.h > stackdemo.shar


