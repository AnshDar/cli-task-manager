#ifndef TASK_H
#define TASK_H

#include <string>

// A single to-do item.
struct Task {
    int id;                   // Unique identifier (auto-increment)
    std::string description;  // What needs to be done
    bool completed;           // true = done, false = pending
    std::string priority;     // "low", "medium" or "high"
    std::string dateCreated;  // Timestamp: YYYY-MM-DD HH:MM:SS
    std::string dueDate;      // Optional due date: YYYY-MM-DD ("" = none)

    // Default constructor (needed for containers / parsing).
    Task();

    // Full constructor.
    Task(int id,
         const std::string& description,
         bool completed,
         const std::string& priority,
         const std::string& dateCreated,
         const std::string& dueDate = "");

    // One-line human readable representation, e.g. for debugging.
    std::string toString() const;

    // Status glyph: check mark when completed, cross otherwise.
    std::string statusIcon() const;

    // Numeric rank for the priority string: high=3, medium=2, low=1, else 0.
    int priorityRank() const;

    // True when the task has a due date that is strictly before `today`
    // (format YYYY-MM-DD) and is not yet completed.
    bool isOverdue(const std::string& today) const;
};

#endif // TASK_H
