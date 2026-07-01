#include "storage.h"

#include <fstream>
#include <string>
#include <vector>

namespace {

// Trim leading/trailing whitespace.
std::string trim(const std::string& s) {
    const std::string ws = " \t\r\n";
    const size_t start = s.find_first_not_of(ws);
    if (start == std::string::npos) return "";
    const size_t end = s.find_last_not_of(ws);
    return s.substr(start, end - start + 1);
}

// Escape a description so it is safe to store between '|' delimiters.
std::string escape(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        switch (c) {
            case '\\': out += "\\\\"; break;
            case '|':  out += "\\|";  break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            default:   out += c;      break;
        }
    }
    return out;
}

// Reverse of escape().
std::string unescape(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '\\' && i + 1 < s.size()) {
            char next = s[++i];
            switch (next) {
                case '\\': out += '\\'; break;
                case '|':  out += '|';  break;
                case 'n':  out += '\n'; break;
                case 'r':  out += '\r'; break;
                default:   out += next; break;
            }
        } else {
            out += s[i];
        }
    }
    return out;
}

// Split a line on '|' delimiters that are NOT escaped (\|), leaving escape
// sequences intact within each field. Fields are trimmed of surrounding space.
std::vector<std::string> splitFields(const std::string& line) {
    std::vector<std::string> fields;
    std::string cur;
    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (c == '\\' && i + 1 < line.size()) {
            cur += c;          // keep the escape sequence for later unescape()
            cur += line[++i];
        } else if (c == '|') {
            fields.push_back(trim(cur));
            cur.clear();
        } else {
            cur += c;
        }
    }
    fields.push_back(trim(cur));
    return fields;
}

} // namespace

namespace Storage {

bool saveTasks(const std::vector<Task>& tasks, const std::string& filename) {
    std::ofstream out(filename, std::ios::trunc);
    if (!out.is_open()) return false;

    for (const Task& t : tasks) {
        out << t.id << " | "
            << escape(t.description) << " | "
            << (t.completed ? "true" : "false") << " | "
            << t.priority << " | "
            << t.dateCreated << " | "
            << t.dueDate << "\n";
    }
    return static_cast<bool>(out);
}

std::vector<Task> loadTasks(const std::string& filename) {
    std::vector<Task> tasks;
    std::ifstream in(filename);
    if (!in.is_open()) {
        // First run: no file yet. Not an error.
        return tasks;
    }

    std::string line;
    while (std::getline(in, line)) {
        if (trim(line).empty()) continue;

        std::vector<std::string> f = splitFields(line);
        // Need at least: id, description, status, priority, date.
        if (f.size() < 5) continue;

        int id = 0;
        try {
            id = std::stoi(f[0]);
        } catch (...) {
            continue; // Bad id, skip line.
        }

        const std::string description = unescape(f[1]);
        const std::string status      = f[2];
        const std::string priority    = f[3];
        const std::string date        = f[4];
        const std::string dueDate     = (f.size() >= 6) ? f[5] : "";

        const bool completed = (status == "true" || status == "1");
        tasks.emplace_back(id, description, completed, priority, date, dueDate);
    }
    return tasks;
}

} // namespace Storage
