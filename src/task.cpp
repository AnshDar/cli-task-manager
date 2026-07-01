#include "task.h"

Task::Task()
    : id(0), description(""), completed(false),
      priority("medium"), dateCreated(""), dueDate("") {}

Task::Task(int id,
           const std::string& description,
           bool completed,
           const std::string& priority,
           const std::string& dateCreated,
           const std::string& dueDate)
    : id(id), description(description), completed(completed),
      priority(priority), dateCreated(dateCreated), dueDate(dueDate) {}

std::string Task::statusIcon() const {
    // U+2713 check mark / U+2717 ballot cross.
    return completed ? "✓" : "✗";
}

int Task::priorityRank() const {
    if (priority == "high") return 3;
    if (priority == "medium") return 2;
    if (priority == "low") return 1;
    return 0;
}

bool Task::isOverdue(const std::string& today) const {
    if (completed || dueDate.empty()) return false;
    // Both are YYYY-MM-DD, so lexicographic compare == chronological compare.
    return dueDate < today;
}

std::string Task::toString() const {
    std::string s = "[" + std::to_string(id) + "] "
                  + statusIcon() + " "
                  + description
                  + " (" + priority + ", " + dateCreated;
    if (!dueDate.empty()) s += ", due " + dueDate;
    s += ")";
    return s;
}
