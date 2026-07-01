#include "taskmanager.h"

#include <algorithm>
#include <cctype>
#include <ctime>
#include "storage.h"

namespace {

// Current local time as "YYYY-MM-DD HH:MM:SS".
std::string getCurrentTimestamp() {
    std::time_t now = std::time(nullptr);
    std::tm tmBuf{};
#if defined(_WIN32)
    localtime_s(&tmBuf, &now);
#else
    localtime_r(&now, &tmBuf);
#endif
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tmBuf);
    return std::string(buf);
}

std::string toLower(const std::string& s) {
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return out;
}

} // namespace

TaskManager::TaskManager(const std::string& filename)
    : filename(filename) {
    tasks = Storage::loadTasks(filename);

    // nextId is one past the highest existing id (ids never reused).
    nextId = 1;
    for (const Task& t : tasks) {
        if (t.id >= nextId) nextId = t.id + 1;
    }
}

int TaskManager::indexOf(int id) const {
    for (size_t i = 0; i < tasks.size(); ++i) {
        if (tasks[i].id == id) return static_cast<int>(i);
    }
    return -1;
}

bool TaskManager::save() const {
    return Storage::saveTasks(tasks, filename);
}

int TaskManager::addTask(const std::string& description,
                         const std::string& priority,
                         const std::string& dueDate) {
    Task t(nextId, description, /*completed=*/false, priority,
           getCurrentTimestamp(), dueDate);
    tasks.push_back(t);
    ++nextId;
    save();
    return t.id;
}

bool TaskManager::deleteTask(int id) {
    int idx = indexOf(id);
    if (idx < 0) return false;
    tasks.erase(tasks.begin() + idx);
    save();
    return true;
}

bool TaskManager::completeTask(int id) {
    int idx = indexOf(id);
    if (idx < 0) return false;
    tasks[idx].completed = true;
    save();
    return true;
}

bool TaskManager::editTask(int id, const std::string& newDescription) {
    int idx = indexOf(id);
    if (idx < 0) return false;
    tasks[idx].description = newDescription;
    save();
    return true;
}

bool TaskManager::setDueDate(int id, const std::string& dueDate) {
    int idx = indexOf(id);
    if (idx < 0) return false;
    tasks[idx].dueDate = dueDate;
    save();
    return true;
}

bool TaskManager::setPriority(int id, const std::string& priority) {
    int idx = indexOf(id);
    if (idx < 0) return false;
    tasks[idx].priority = priority;
    save();
    return true;
}

std::vector<Task> TaskManager::searchTask(const std::string& keyword) const {
    std::vector<Task> results;
    const std::string needle = toLower(keyword);
    for (const Task& t : tasks) {
        if (toLower(t.description).find(needle) != std::string::npos) {
            results.push_back(t);
        }
    }
    return results;
}

std::vector<Task> TaskManager::getAllTasks(SortOrder order) const {
    std::vector<Task> sorted = tasks;

    // Newest-first is the tie-breaker for every ordering.
    auto newerFirst = [](const Task& a, const Task& b) {
        if (a.dateCreated != b.dateCreated) return a.dateCreated > b.dateCreated;
        return a.id > b.id;
    };

    switch (order) {
        case SortOrder::PriorityHighFirst:
            std::sort(sorted.begin(), sorted.end(),
                      [&](const Task& a, const Task& b) {
                          if (a.priorityRank() != b.priorityRank())
                              return a.priorityRank() > b.priorityRank();
                          return newerFirst(a, b);
                      });
            break;
        case SortOrder::PendingFirst:
            std::sort(sorted.begin(), sorted.end(),
                      [&](const Task& a, const Task& b) {
                          if (a.completed != b.completed)
                              return !a.completed; // pending before done
                          return newerFirst(a, b);
                      });
            break;
        case SortOrder::DateNewest:
        default:
            std::sort(sorted.begin(), sorted.end(), newerFirst);
            break;
    }
    return sorted;
}

const Task* TaskManager::getTaskById(int id) const {
    int idx = indexOf(id);
    return idx < 0 ? nullptr : &tasks[idx];
}

int TaskManager::getTaskCount() const {
    return static_cast<int>(tasks.size());
}

int TaskManager::getCompletedCount() const {
    int count = 0;
    for (const Task& t : tasks) {
        if (t.completed) ++count;
    }
    return count;
}

int TaskManager::getPendingCount() const {
    return getTaskCount() - getCompletedCount();
}

int TaskManager::getPriorityCount(const std::string& priority) const {
    int count = 0;
    for (const Task& t : tasks) {
        if (t.priority == priority) ++count;
    }
    return count;
}

int TaskManager::getOverdueCount(const std::string& today) const {
    int count = 0;
    for (const Task& t : tasks) {
        if (t.isOverdue(today)) ++count;
    }
    return count;
}
