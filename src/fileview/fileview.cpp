#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <algorithm>
#include <cstring>
#include <cmath>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include <unistd.h>
#include <getopt.h>
#include <regex>
#include <map>
#include <csignal>
#include <cstdlib>

// ANSI color codes
#define COLOR_RESET   "\033[0m"
#define COLOR_BOLD    "\033[1m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_WHITE   "\033[37m"

std::map<std::string, std::string> extension_colors = {
    {".cpp", COLOR_CYAN},
    {".h", COLOR_CYAN},
    {".hpp", COLOR_CYAN},
    {".c", COLOR_CYAN},
    {".py", COLOR_GREEN},
    {".js", COLOR_YELLOW},
    {".html", COLOR_MAGENTA},
    {".css", COLOR_BLUE},
    {".md", COLOR_WHITE},
    {".txt", COLOR_WHITE},
    {".json", COLOR_YELLOW},
    {".xml", COLOR_MAGENTA},
    {".sh", COLOR_GREEN},
    {".jpg", COLOR_RED},
    {".png", COLOR_RED},
    {".gif", COLOR_RED},
    {".pdf", COLOR_RED},
    {".zip", COLOR_YELLOW},
    {".tar", COLOR_YELLOW},
    {".gz", COLOR_YELLOW}
};

bool show_sizes = false;
bool show_times = false;
bool show_permissions = false;
std::string type_filter;
long min_size = 0;

volatile sig_atomic_t interrupted = 0;

void signal_handler(int signal) {
    if (signal == SIGINT) {
        interrupted = 1;
        std::cout << "\nInterrupted. Exiting..." << std::endl;
    }
}

void print_usage();
void print_directory_tree(const std::string& path, int level = 0, const std::string& prefix = "");
std::string format_size(long size);
std::string format_permissions(mode_t mode);
std::string format_time(time_t time);
std::string get_file_extension(const std::string& filename);
bool matches_filter(const std::string& path, const struct stat& st);
std::string get_color_for_file(const std::string& filename, mode_t mode);
long parse_size(const std::string& size_str);

int main(int argc, char* argv[]) {
    std::signal(SIGINT, signal_handler);
    std::string directory = ".";
    static struct option long_options[] = {
        {"sizes", no_argument, 0, 's'},
        {"times", no_argument, 0, 't'},
        {"perms", no_argument, 0, 'p'},
        {"type", required_argument, 0, 'T'},
        {"minsize", required_argument, 0, 'm'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };
    int opt;
    int option_index = 0;
    while ((opt = getopt_long(argc, argv, "stpT:m:h", long_options, &option_index)) != -1) {
        switch (opt) {
            case 's':
                show_sizes = true;
                break;
            case 't':
                show_times = true;
                break;
            case 'p':
                show_permissions = true;
                break;
            case 'T':
                type_filter = optarg;
                break;
            case 'm':
                min_size = parse_size(optarg);
                break;
            case 'h':
                print_usage();
                return 0;
            default:
                print_usage();
                return 1;
        }
    }
    if (optind < argc) {
        directory = argv[optind];
    }
    struct stat st;
    if (stat(directory.c_str(), &st) != 0 || !S_ISDIR(st.st_mode)) {
        std::cerr << "Error: " << directory << " is not a valid directory." << std::endl;
        return 1;
    }
    std::cout << COLOR_BOLD << "Directory Tree: " << directory << COLOR_RESET << std::endl;
    print_directory_tree(directory);
    return 0;
}

void print_usage() {
    std::cout << "Usage: fileview [OPTIONS] [DIRECTORY]" << std::endl;
    std::cout << "Display directory structure with file sizes, types, and highlights." << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -s, --sizes           Show file sizes" << std::endl;
    std::cout << "  -t, --times           Show modification times" << std::endl;
    std::cout << "  -p, --perms           Show file permissions" << std::endl;
    std::cout << "  -T, --type=EXT        Filter by file extension (e.g., .cpp)" << std::endl;
    std::cout << "  -m, --minsize=SIZE    Filter by minimum size (e.g., 1MB, 500KB)" << std::endl;
    std::cout << "  -h, --help            Display this help and exit" << std::endl;
}

void print_directory_tree(const std::string& path, int level, const std::string& prefix) {
    if (interrupted) {
        return;
    }
    DIR* dir = opendir(path.c_str());
    if (!dir) {
        return;
    }
    std::vector<std::string> entries;
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        entries.push_back(entry->d_name);
    }
    closedir(dir);
    std::sort(entries.begin(), entries.end());
    for (size_t i = 0; i < entries.size(); ++i) {
        if (interrupted) {
            return;
        }
        bool is_last = (i == entries.size() - 1);
        std::string full_path = path + "/" + entries[i];
        struct stat st;
        if (stat(full_path.c_str(), &st) != 0) {
            continue;
        }
        if (!matches_filter(full_path, st)) {
            continue;
        }
        std::string new_prefix = prefix + (is_last ? "└── " : "├── ");
        std::string next_prefix = prefix + (is_last ? "    " : "│   ");
        std::string color = get_color_for_file(entries[i], st.st_mode);
        std::cout << prefix << (is_last ? "└── " : "├── ") << color << entries[i] << COLOR_RESET;
        if (show_sizes && !S_ISDIR(st.st_mode)) {
            std::cout << " [" << format_size(st.st_size) << "]";
        }
        if (show_times) {
            std::cout << " [" << format_time(st.st_mtime) << "]";
        }
        if (show_permissions) {
            std::cout << " [" << format_permissions(st.st_mode) << "]";
        }
        std::cout << std::endl;
        if (S_ISDIR(st.st_mode)) {
            print_directory_tree(full_path, level + 1, next_prefix);
        }
    }
}

std::string format_size(long size) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit_index = 0;
    double size_d = static_cast<double>(size);
    while (size_d >= 1024.0 && unit_index < 4) {
        size_d /= 1024.0;
        unit_index++;
    }
    char buffer[32];
    if (unit_index == 0) {
        snprintf(buffer, sizeof(buffer), "%ld %s", size, units[unit_index]);
    } else {
        snprintf(buffer, sizeof(buffer), "%.1f %s", size_d, units[unit_index]);
    }
    return std::string(buffer);
}

std::string format_permissions(mode_t mode) {
    char perms[11];
    perms[0] = (S_ISDIR(mode)) ? 'd' : (S_ISLNK(mode)) ? 'l' : '-';
    perms[1] = (mode & S_IRUSR) ? 'r' : '-';
    perms[2] = (mode & S_IWUSR) ? 'w' : '-';
    perms[3] = (mode & S_IXUSR) ? 'x' : '-';
    perms[4] = (mode & S_IRGRP) ? 'r' : '-';
    perms[5] = (mode & S_IWGRP) ? 'w' : '-';
    perms[6] = (mode & S_IXGRP) ? 'x' : '-';
    perms[7] = (mode & S_IROTH) ? 'r' : '-';
    perms[8] = (mode & S_IWOTH) ? 'w' : '-';
    perms[9] = (mode & S_IXOTH) ? 'x' : '-';
    perms[10] = '\0';
    return std::string(perms);
}

std::string format_time(time_t time) {
    char buffer[20];
    struct tm* timeinfo = localtime(&time);
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M", timeinfo);
    return std::string(buffer);
}

std::string get_file_extension(const std::string& filename) {
    size_t pos = filename.find_last_of('.');
    if (pos != std::string::npos) {
        return filename.substr(pos);
    }
    return "";
}

bool matches_filter(const std::string& path, const struct stat& st) {
    if (!type_filter.empty()) {
        std::string ext = get_file_extension(path);
        if (ext != type_filter && !S_ISDIR(st.st_mode)) {
            return false;
        }
    }
    if (min_size > 0 && !S_ISDIR(st.st_mode) && st.st_size < min_size) {
        return false;
    }
    return true;
}

std::string get_color_for_file(const std::string& filename, mode_t mode) {
    if (S_ISDIR(mode)) {
        return std::string(COLOR_BOLD) + std::string(COLOR_BLUE);
    }
    if (mode & S_IXUSR) {
        return COLOR_GREEN;
    }
    std::string ext = get_file_extension(filename);
    if (extension_colors.find(ext) != extension_colors.end()) {
        return extension_colors[ext];
    }
    return COLOR_RESET;
}

long parse_size(const std::string& size_str) {
    std::regex size_regex("([0-9]+)([KMGTkmgt]?[Bb]?)");
    std::smatch match;
    if (!std::regex_match(size_str, match, size_regex)) {
        std::cerr << "Warning: Invalid size format: " << size_str << std::endl;
        return 0;
    }
    
    long size = std::stol(match[1]);
    std::string unit = match[2];
    
    if (unit.empty() || unit == "B" || unit == "b") {
        return size;
    } else if (unit == "K" || unit == "k" || unit == "KB" || unit == "kb") {
        return size * 1024;
    } else if (unit == "M" || unit == "m" || unit == "MB" || unit == "mb") {
        return size * 1024 * 1024;
    } else if (unit == "G" || unit == "g" || unit == "GB" || unit == "gb") {
        return size * 1024 * 1024 * 1024;
    } else if (unit == "T" || unit == "t" || unit == "TB" || unit == "tb") {
        return size * 1024 * 1024 * 1024 * 1024;
    }
    
    return size;
}
