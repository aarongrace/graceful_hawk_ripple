# Make uses sh, not bash, so we need to install the hawk and hawklink commands;
# these definitions are lifted from the aliases in .bashrc
# (If you try to use this makefile on a different system, you will need
# to change the file names to where hawk and hawklink are installed there.)
smal=~/maison/hawk/g_smal32 -P 45 -U ~/maison/hawk -L
hawklink=~/maison/hawk/hawklink -m -o ripples ~/maison/hawk/monitor.o
hawk=~/maison/graceful_hawk/hawk

modules = canvas.o display.o draw_objects.o generate_circle.o ripple.o

##########
# primary make target

whatever.o: main.o $(modules) common.h
	$(hawklink) main.o $(modules)
	$(hawk)	ripples.o


##########
# other make targets
main.o: main.a common.h
	$(smal) main.a

canvas.o: canvas.a common.h
	$(smal) canvas.a

display.o: display.a common.h
	$(smal) display.a

generate_circle.o: generate_circle.a common.h
	$(smal) generate_circle.a

draw_objects.o: draw_objects.a common.h
	$(smal) draw_objects.a

ripple.o: ripple.a common.h
	$(smal) ripple.a
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


