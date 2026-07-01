#ifndef STORAGE_H
#define STORAGE_H

#include <string>
#include <vector>
#include "task.h"

// File persistence for tasks.
//
// On-disk format is one task per line:
//   ID | Description | Status | Priority | DateCreated | DueDate
// e.g.
//   1 | Buy groceries | false | high | 2024-06-14 10:30:00 | 2024-06-20
//
// The description is escaped on write (\\, \| and \n) so that a literal '|'
// or newline inside a description never collides with the field delimiter.
// Lines with only 5 fields (no DueDate) from an older version are still read.
namespace Storage {

// Write all tasks to `filename`. Returns false on I/O failure.
bool saveTasks(const std::vector<Task>& tasks, const std::string& filename);

// Load all tasks from `filename`. A missing file is treated as "no tasks yet"
// (returns an empty vector, not an error). Malformed lines are skipped.
std::vector<Task> loadTasks(const std::string& filename);

} // namespace Storage

#endif // STORAGE_H
