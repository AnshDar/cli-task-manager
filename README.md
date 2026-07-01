# CLI Task Manager

A simple, dependency-free command-line task manager written in C++17.
Tasks are stored in a plain-text file and auto-saved after every change.

## Features

- Add, view, edit, and delete tasks
- Mark tasks as complete
- Case-insensitive keyword search
- Priority levels (`low` / `medium` / `high`)
- **Optional due dates** with **overdue highlighting**
- **Sortable views** — by newest, by priority, or pending-first
- **Colored output** (ANSI) that auto-disables when piped or `NO_COLOR` is set
- Persistent storage (auto-saved to `tasks.txt`)
- Task statistics: progress bar, priority breakdown, and overdue count

## Build & Run

With the provided Makefile (requires `make` + `g++`):

```bash
make        # compile
make run    # compile and run
make clean  # remove build artifacts
```

Or compile manually:

```bash
g++ -std=c++17 -static -o task_manager \
    src/main.cpp src/task.cpp src/taskmanager.cpp src/storage.cpp
./task_manager        # on Windows: .\task_manager.exe
```

> **Windows / MSYS2 note:** the `-static` flag makes the executable
> self-contained. Without it, on a machine that has both the MinGW64
> (`msvcrt`) and UCRT64 toolchains, the exe can load a mismatched
> `libstdc++-6.dll` from whichever `bin` directory is first on `PATH`, which
> crashes at startup. Static linking avoids this entirely.

## Usage

Run the program and choose an option from the menu:

```
+------------------------------------+
|      TASK MANAGER - Main Menu       |
+------------------------------------+
| 1. Add Task                        |
| 2. View All Tasks                  |
| 3. Mark Task Complete              |
| 4. Delete Task                     |
| 5. Search Tasks                    |
| 6. Edit Task                       |
| 7. Statistics                      |
| 8. Exit                            |
+------------------------------------+
Enter choice (1-8): 1
Enter task description: Buy groceries
Enter priority (low/medium/high) [medium]: high
Enter due date (YYYY-MM-DD, blank for none): 2024-06-20
✓ Task added successfully (ID: 1)
```

- **View** (option 2) asks how to sort: newest, by priority, or pending-first.
- **Edit** (option 6) lets you change the description, priority, and due date;
  press Enter to keep any field, or type `clear` to remove a due date.
- Overdue tasks are shown in **red** (or marked with `*` when colour is off).
- At any numeric prompt you can press Enter on a blank line to cancel and
  return to the menu.

## Storage Format

Tasks are saved one per line in `tasks.txt`:

```
ID | Description | Status | Priority | DateCreated | DueDate
1 | Buy groceries | false | high | 2024-06-14 10:30:00 | 2024-06-20
2 | Complete project | true | high | 2024-06-14 11:15:00 |
```

`DueDate` may be empty (no due date). The description is escaped on write
(`\\`, `\|`, `\n`) so a literal `|` or newline inside a description never
collides with the field delimiter — it round-trips exactly. Lines written by
an older 5-field version (without `DueDate`) are still read. A missing
`tasks.txt` on first run is treated as an empty list and created on first save.

## Project Structure

```
task-manager/
├── src/
│   ├── main.cpp          # Menu loop & user interface
│   ├── task.h / .cpp     # Task data model
│   ├── taskmanager.h/cpp # Core task logic (add/edit/delete/search/stats)
│   └── storage.h / .cpp  # File I/O (load/save)
├── tasks.txt             # Persistent data (created at runtime)
├── Makefile
├── README.md
└── .gitignore
```

## Notes

- IDs auto-increment and are never reused, even after deletions.
- On Windows the console output code page is set to UTF-8 so the ✓/✗ status
  glyphs render correctly.
