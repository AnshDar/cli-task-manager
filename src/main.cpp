// CLI Task Manager - interactive menu loop.
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <string>
#include <vector>

#include "task.h"
#include "taskmanager.h"

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#define ISATTY _isatty
#define FILENO _fileno
#else
#include <unistd.h>
#define ISATTY isatty
#define FILENO fileno
#endif

namespace {

const int DESC_COL_WIDTH = 40; // Descriptions longer than this are truncated.
const int DUE_COL_WIDTH = 11;  // 10-char date + room for an overdue '*' marker.

// ----- ANSI colour support -------------------------------------------------

namespace ansi {
const char* RESET  = "\033[0m";
const char* BOLD   = "\033[1m";
const char* RED    = "\033[31m";
const char* GREEN  = "\033[32m";
const char* YELLOW = "\033[33m";
const char* CYAN   = "\033[36m";
const char* GRAY   = "\033[90m";
} // namespace ansi

bool gColor = false; // set in main() based on terminal / NO_COLOR

// Wrap text in an ANSI colour if colouring is enabled.
std::string col(const std::string& text, const char* code) {
    if (!gColor) return text;
    return std::string(code) + text + ansi::RESET;
}

const char* priorityColor(const std::string& p) {
    if (p == "high") return ansi::RED;
    if (p == "medium") return ansi::YELLOW;
    if (p == "low") return ansi::GREEN;
    return ansi::GRAY;
}

// ----- date helpers --------------------------------------------------------

std::string getToday() {
    std::time_t now = std::time(nullptr);
    std::tm tmBuf{};
#if defined(_WIN32)
    localtime_s(&tmBuf, &now);
#else
    localtime_r(&now, &tmBuf);
#endif
    char buf[16];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d", &tmBuf);
    return std::string(buf);
}

// Structural + light range check for a YYYY-MM-DD date.
bool isValidDate(const std::string& s) {
    if (s.size() != 10) return false;
    for (size_t i = 0; i < 10; ++i) {
        if (i == 4 || i == 7) {
            if (s[i] != '-') return false;
        } else if (!std::isdigit(static_cast<unsigned char>(s[i]))) {
            return false;
        }
    }
    int month = std::stoi(s.substr(5, 2));
    int day   = std::stoi(s.substr(8, 2));
    return month >= 1 && month <= 12 && day >= 1 && day <= 31;
}

// ----- small input helpers -------------------------------------------------

bool readLine(const std::string& prompt, std::string& out) {
    std::cout << prompt;
    if (!std::getline(std::cin, out)) return false;
    return true;
}

std::string trim(const std::string& s) {
    const std::string ws = " \t\r\n";
    const size_t start = s.find_first_not_of(ws);
    if (start == std::string::npos) return "";
    const size_t end = s.find_last_not_of(ws);
    return s.substr(start, end - start + 1);
}

std::string toLower(const std::string& s) {
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return out;
}

// Prompt for an integer. Returns false if the user cancels (blank line) or EOF.
bool readInt(const std::string& prompt, int& out) {
    std::string line;
    if (!readLine(prompt, line)) return false;
    line = trim(line);
    if (line.empty()) return false;
    try {
        size_t pos = 0;
        int value = std::stoi(line, &pos);
        if (pos != line.size()) return false; // trailing junk
        out = value;
        return true;
    } catch (...) {
        return false;
    }
}

// Ask for a priority, returning a validated value. Empty input -> `fallback`.
std::string promptPriority(const std::string& fallback) {
    std::string priority;
    readLine("Enter priority (low/medium/high) [" + fallback + "]: ", priority);
    priority = toLower(trim(priority));
    if (priority.empty()) return fallback;
    if (priority != "low" && priority != "medium" && priority != "high") {
        std::cout << "  Unknown priority, using '" << fallback << "'.\n";
        return fallback;
    }
    return priority;
}

// ----- display helpers -----------------------------------------------------

void displayMessage(const std::string& msg, char type) {
    // type: '+' success, '-' error, 'i' info.
    switch (type) {
        case '+': std::cout << col("\xE2\x9C\x93 " + msg, ansi::GREEN) << "\n"; break;
        case '-': std::cout << col("\xE2\x9C\x97 " + msg, ansi::RED) << "\n"; break;
        default:  std::cout << "  " << msg << "\n"; break;
    }
}

std::string truncate(const std::string& s, size_t width) {
    if (s.size() <= width) return s;
    if (width <= 3) return s.substr(0, width);
    return s.substr(0, width - 3) + "...";
}

std::string padRight(const std::string& s, size_t width) {
    if (s.size() >= width) return s;
    return s + std::string(width - s.size(), ' ');
}

void displayTaskTable(const std::vector<Task>& tasks, const std::string& today) {
    if (tasks.empty()) {
        displayMessage("No tasks to show.", 'i');
        return;
    }

    const std::string sep(
        4 + 3 + DESC_COL_WIDTH + 3 + 8 + 3 + 8 + 3 + DUE_COL_WIDTH + 3 + 19, '-');
    std::string header = padRight("ID", 4) + " | "
                       + padRight("Description", DESC_COL_WIDTH) + " | "
                       + padRight("Status", 8) + " | "
                       + padRight("Priority", 8) + " | "
                       + padRight("Due", DUE_COL_WIDTH) + " | "
                       + padRight("Created", 19);

    std::cout << sep << "\n" << col(header, ansi::BOLD) << "\n" << sep << "\n";

    for (const Task& t : tasks) {
        // Status cell.
        std::string statusText = t.completed ? "done" : "pending";
        std::string statusCell =
            col(padRight(statusText, 8), t.completed ? ansi::GREEN : ansi::GRAY);

        // Priority cell.
        std::string priorityCell =
            col(padRight(t.priority, 8), priorityColor(t.priority));

        // Due cell (highlight overdue).
        std::string dueRaw = t.dueDate.empty() ? "-" : t.dueDate;
        std::string dueCell;
        if (t.isOverdue(today)) {
            // Bold red when colour is on; a trailing '*' marks it otherwise.
            dueCell = gColor
                ? std::string(ansi::BOLD) + ansi::RED
                      + padRight(dueRaw, DUE_COL_WIDTH) + ansi::RESET
                : padRight(dueRaw + "*", DUE_COL_WIDTH);
        } else if (t.dueDate.empty()) {
            dueCell = col(padRight(dueRaw, DUE_COL_WIDTH), ansi::GRAY);
        } else {
            dueCell = col(padRight(dueRaw, DUE_COL_WIDTH), ansi::CYAN);
        }

        std::cout << padRight(std::to_string(t.id), 4) << " | "
                  << padRight(truncate(t.description, DESC_COL_WIDTH),
                              DESC_COL_WIDTH) << " | "
                  << statusCell << " | "
                  << priorityCell << " | "
                  << dueCell << " | "
                  << padRight(t.dateCreated, 19) << "\n";
    }
    std::cout << sep << "\n";
    std::cout << tasks.size() << " task(s).\n";
}

void displayMenu() {
    std::cout << "\n"
              << col("+------------------------------------+", ansi::CYAN) << "\n"
              << col("|      TASK MANAGER - Main Menu       |", ansi::CYAN) << "\n"
              << col("+------------------------------------+", ansi::CYAN) << "\n"
              << "| 1. Add Task                        |\n"
              << "| 2. View All Tasks                  |\n"
              << "| 3. Mark Task Complete              |\n"
              << "| 4. Delete Task                     |\n"
              << "| 5. Search Tasks                    |\n"
              << "| 6. Edit Task                       |\n"
              << "| 7. Statistics                      |\n"
              << "| 8. Exit                            |\n"
              << col("+------------------------------------+", ansi::CYAN) << "\n";
}

// ----- menu actions --------------------------------------------------------

void addTaskMenu(TaskManager& tm) {
    std::string description;
    if (!readLine("Enter task description: ", description)) return;
    description = trim(description);
    if (description.empty()) {
        displayMessage("Description cannot be empty. Task not added.", '-');
        return;
    }

    std::string priority = promptPriority("medium");

    std::string due;
    readLine("Enter due date (YYYY-MM-DD, blank for none): ", due);
    due = trim(due);
    if (!due.empty() && !isValidDate(due)) {
        displayMessage("Invalid date, saving task without a due date.", 'i');
        due = "";
    }

    int id = tm.addTask(description, priority, due);
    displayMessage("Task added successfully (ID: " + std::to_string(id) + ")",
                   '+');
}

void viewTasksMenu(const TaskManager& tm, const std::string& today) {
    std::cout << "Sort by: [1] Newest (default)  [2] Priority  [3] Pending first\n";
    int choice = 1;
    readInt("Choice: ", choice); // blank/invalid -> keeps default 1

    SortOrder order = SortOrder::DateNewest;
    if (choice == 2) order = SortOrder::PriorityHighFirst;
    else if (choice == 3) order = SortOrder::PendingFirst;

    displayTaskTable(tm.getAllTasks(order), today);
}

void markCompleteMenu(TaskManager& tm) {
    int id;
    if (!readInt("Enter task ID to mark complete: ", id)) {
        displayMessage("Invalid ID.", '-');
        return;
    }
    const Task* t = tm.getTaskById(id);
    if (!t) {
        displayMessage("No task with ID " + std::to_string(id) + ".", '-');
        return;
    }
    if (t->completed) {
        displayMessage("Task " + std::to_string(id) + " is already complete.",
                       'i');
        return;
    }
    tm.completeTask(id);
    displayMessage("Task " + std::to_string(id) + " marked complete.", '+');
}

void deleteTaskMenu(TaskManager& tm) {
    int id;
    if (!readInt("Enter task ID to delete: ", id)) {
        displayMessage("Invalid ID.", '-');
        return;
    }
    const Task* t = tm.getTaskById(id);
    if (!t) {
        displayMessage("No task with ID " + std::to_string(id) + ".", '-');
        return;
    }

    std::cout << "About to delete: " << t->toString() << "\n";
    std::string confirm;
    readLine("Are you sure? (y/N): ", confirm);
    if (toLower(trim(confirm)) != "y") {
        displayMessage("Deletion cancelled.", 'i');
        return;
    }
    tm.deleteTask(id);
    displayMessage("Task " + std::to_string(id) + " deleted.", '+');
}

void searchTasksMenu(const TaskManager& tm, const std::string& today) {
    std::string keyword;
    if (!readLine("Enter search keyword: ", keyword)) return;
    keyword = trim(keyword);
    if (keyword.empty()) {
        displayMessage("Empty keyword.", '-');
        return;
    }
    std::vector<Task> results = tm.searchTask(keyword);
    if (results.empty()) {
        displayMessage("No tasks match \"" + keyword + "\".", 'i');
        return;
    }
    displayTaskTable(results, today);
}

void editTaskMenu(TaskManager& tm) {
    int id;
    if (!readInt("Enter task ID to edit: ", id)) {
        displayMessage("Invalid ID.", '-');
        return;
    }
    const Task* t = tm.getTaskById(id);
    if (!t) {
        displayMessage("No task with ID " + std::to_string(id) + ".", '-');
        return;
    }
    std::cout << "Editing task " << id << ". Press Enter to keep the current "
                 "value.\n";

    // Description.
    std::cout << "Current description: " << t->description << "\n";
    std::string newDesc;
    readLine("New description: ", newDesc);
    newDesc = trim(newDesc);
    if (!newDesc.empty()) tm.editTask(id, newDesc);

    // Priority.
    std::cout << "Current priority: " << t->priority << "\n";
    std::string p;
    readLine("New priority (low/medium/high): ", p);
    p = toLower(trim(p));
    if (!p.empty()) {
        if (p == "low" || p == "medium" || p == "high") {
            tm.setPriority(id, p);
        } else {
            displayMessage("Unknown priority ignored.", 'i');
        }
    }

    // Due date.
    std::string curDue = t->dueDate.empty() ? "(none)" : t->dueDate;
    std::cout << "Current due date: " << curDue
              << "  (type 'clear' to remove)\n";
    std::string d;
    readLine("New due date (YYYY-MM-DD): ", d);
    d = trim(d);
    if (!d.empty()) {
        if (toLower(d) == "clear" || d == "-") {
            tm.setDueDate(id, "");
        } else if (isValidDate(d)) {
            tm.setDueDate(id, d);
        } else {
            displayMessage("Invalid date ignored.", 'i');
        }
    }

    displayMessage("Task " + std::to_string(id) + " updated.", '+');
}

void statisticsMenu(const TaskManager& tm, const std::string& today) {
    const int total   = tm.getTaskCount();
    const int done    = tm.getCompletedCount();
    const int pending = tm.getPendingCount();
    const int overdue = tm.getOverdueCount(today);

    std::cout << "\n" << col("--- Statistics ---", ansi::BOLD) << "\n";
    std::cout << "Total tasks : " << total << "\n";
    std::cout << "Completed   : " << done << "\n";
    std::cout << "Pending     : " << pending << "\n";
    std::cout << "Overdue     : " << col(std::to_string(overdue),
                                         overdue > 0 ? ansi::RED : ansi::GRAY)
              << "\n";

    std::cout << "By priority : "
              << col("high " + std::to_string(tm.getPriorityCount("high")),
                     ansi::RED)
              << "  "
              << col("medium " + std::to_string(tm.getPriorityCount("medium")),
                     ansi::YELLOW)
              << "  "
              << col("low " + std::to_string(tm.getPriorityCount("low")),
                     ansi::GREEN)
              << "\n";

    if (total > 0) {
        int pct = (done * 100) / total;
        std::cout << "Progress    : " << pct << "% [";
        const int bars = pct / 5; // 20-char bar
        std::string bar;
        for (int i = 0; i < 20; ++i) bar += (i < bars ? '#' : '.');
        std::cout << col(bar, ansi::GREEN) << "]\n";
    }
}

} // namespace

int main() {
#ifdef _WIN32
    // Render the check/cross glyphs and box characters as UTF-8...
    SetConsoleOutputCP(CP_UTF8);
    // ...and enable ANSI escape processing on the Windows console.
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD mode = 0;
        if (GetConsoleMode(hOut, &mode)) {
            SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
        }
    }
#endif

    // Enable colour only for an interactive terminal and when NO_COLOR is unset.
    gColor = ISATTY(FILENO(stdout)) && (std::getenv("NO_COLOR") == nullptr);

    TaskManager tm("tasks.txt");
    const std::string today = getToday();
    std::cout << "Welcome to the CLI Task Manager!\n";

    while (true) {
        displayMenu();
        int choice;
        if (!readInt("Enter choice (1-8): ", choice)) {
            if (std::cin.eof()) {
                std::cout << "\nGoodbye!\n";
                break;
            }
            displayMessage("Please enter a number between 1 and 8.", '-');
            continue;
        }

        switch (choice) {
            case 1: addTaskMenu(tm); break;
            case 2: viewTasksMenu(tm, today); break;
            case 3: markCompleteMenu(tm); break;
            case 4: deleteTaskMenu(tm); break;
            case 5: searchTasksMenu(tm, today); break;
            case 6: editTaskMenu(tm); break;
            case 7: statisticsMenu(tm, today); break;
            case 8:
                std::cout << "Goodbye!\n";
                return 0;
            default:
                displayMessage("Invalid choice. Pick 1-8.", '-');
                break;
        }
    }
    return 0;
}
