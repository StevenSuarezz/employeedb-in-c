# Employee Database Learning Project
A small C program written to reinforce fundamental C concepts, particularly memory management, pointer manipulation, and struct handling. This project simulates a basic employee database system while focusing on learning rather than practical functionality.

## Code Structure
- `main.c`: Program entry point and argument handling
- `parse.c`: Core database operations (add/list/read employees)
- `file.c`: File I/O operations
- `common.h`: Shared definitions and structures

## Usage
```bash
./dbview -n -f <database file>    # Create new database
./dbview -f <database file> -a "name,address,hours"  # Add employee
./dbview -f <database file> -l    # List employees
```

## Personal Notes
While working on this project, I wrote the following code:

```c 
employees = realloc(employees, dbheader->count * sizeof(struct employee_t));
```

I was under the impression that realloc would zero out the newly created struct. Though I look back at this as a silly oversight, I feel it really helped drill in me the concept of being extra explicit and intentional when allocating memory.

After examining my db file using `xxd` I was seeing unexpected output in the binary. 
Looking back again I remember, I had seen this quite before in other projects but didnt make the connection for a bit.
All I was missing was:
```c
memset(&employees[dbheader->count - 1], 0, sizeof(struct employee_t));
```
Moral of the story: You need to zero out your memory when you allocate it or you run the risk of introducing garbage value bugs into the code
