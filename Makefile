TARGET = bin/dbview
SRC = $(wildcard src/*.c)
OBJ = $(patsubst src/%.c, obj/%.o, $(SRC))

run: clean default
	./$(TARGET) -f ./mydb.db -n
	./$(TARGET) -f ./mydb.db
	./$(TARGET) -f ./mydb.db -a "Steven Suarez,123 applebees ln,120"

leak_check: clean default
	valgrind --leak-check=yes ./$(TARGET) -f ./mydb.db -n -a "Steven Suarez,123 applebees ln,120" -l

default: $(TARGET)

clean:
	rm -f obj/*.o
	rm -f bin/*
	rm -f *.db

$(TARGET): $(OBJ)
	gcc -o $@ $?

obj/%.o : src/%.c
	gcc -g -O0 -c $< -o $@ -Iinclude
