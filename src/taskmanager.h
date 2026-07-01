#ifndef TASKMANAGER_H
#define TASKMANAGER_H

#include <string>
#include <vector>
#include "task.h"

// How getAllTasks() orders the returned list.
enum class SortOrder {
    DateNewest,        // newest creation date first (default)
    PriorityHighFirst, // high -> medium -> low
    PendingFirst       // incomplete tasks before completed ones
};

// Owns the in-memory list of tasks and keeps the storage file in sync.
// Every mutating operation auto-saves to disk.
class TaskManager {
public:
    // Loads existing tasks from `filename` (empty if the file does not exist).
    explicit TaskManager(const std::string& filename = "tasks.txt");

    // Create a task. `dueDate` may be empty. Returns the new task's id.
    int addTask(const std::string& description,
                const std::string& priority,
                const std::string& dueDate = "");

    // The following return true on success, false if no task has that id.
    bool deleteTask(int id);
    bool completeTask(int id);
    bool editTask(int id, const std::string& newDescription);
    bool setDueDate(int id, const std::string& dueDate);
    bool setPriority(int id, const std::string& priority);

    // Case-insensitive substring search over descriptions.
    std::vector<Task> searchTask(const std::string& keyword) const;

    // All tasks in the requested order.
    std::vector<Task> getAllTasks(SortOrder order = SortOrder::DateNewest) const;

    // Pointer to the task with `id`, or nullptr if none.
    const Task* getTaskById(int id) const;

    // Statistics.
    int getTaskCount() const;
    int getCompletedCount() const;
    int getPendingCount() const;
    int getPriorityCount(const std::string& priority) const;
    int getOverdueCount(const std::string& today) const;

private:
    std::vector<Task> tasks;
    int nextId;
    std::string filename;

    // Persist current state; called after every mutation.
    bool save() const;

    // Index of task with `id`, or -1.
    int indexOf(int id) const;
};

#endif // TASKMANAGER_H
