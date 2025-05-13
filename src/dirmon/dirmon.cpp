#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <map>
#include <vector>
#include <chrono>
#include <ctime>
#include <ncurses.h>
#include <getopt.h>

#define BUF_LEN (10 * (sizeof(struct inotify_event) + NAME_MAX + 1))

bool use_curses = false;
std::ofstream log_file;
std::map<int, std::string> watch_descriptors;
size_t max_log_lines = 1000;
std::vector<std::string> log_history;

void add_watch_recursive(int fd, const std::string& path);
void log_message(const std::string& message);
void print_usage();
void setup_curses();
void cleanup_curses();
void update_curses_display();
std::string get_current_time();

int main(int argc, char* argv[]) {
    std::string directory;
    std::string log_file_path;

    static struct option long_options[] = {
        {"log-file", required_argument, 0, 'l'},
        {"curses", no_argument, 0, 'c'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    int opt;
    int option_index = 0;

    while ((opt = getopt_long(argc, argv, "l:ch", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'l':
                log_file_path = optarg;
                break;
            case 'c':
                use_curses = true;
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
    } else {
        std::cerr << "Error: No directory specified." << std::endl;
        print_usage();
        return 1;
    }

    struct stat st;
    if (stat(directory.c_str(), &st) != 0 || !S_ISDIR(st.st_mode)) {
        std::cerr << "Error: " << directory << " is not a valid directory." << std::endl;
        return 1;
    }

    if (!log_file_path.empty()) {
        log_file.open(log_file_path, std::ios::out | std::ios::app);
        if (!log_file.is_open()) {
            std::cerr << "Error: Could not open log file " << log_file_path << std::endl;
            return 1;
        }
    }

    if (use_curses) {
        setup_curses();
    }

    int fd = inotify_init();
    if (fd < 0) {
        log_message("Error: Could not initialize inotify");
        if (use_curses) cleanup_curses();
        return 1;
    }

    try {
        add_watch_recursive(fd, directory);
    } catch (const std::exception& e) {
        log_message(std::string("Error: ") + e.what());
        if (use_curses) cleanup_curses();
        close(fd);
        return 1;
    }

    log_message("Monitoring directory: " + directory);
    log_message("Press Ctrl+C to exit");

    char buffer[BUF_LEN];
    while (true) {
        int length = read(fd, buffer, BUF_LEN);
        if (length < 0) {
            log_message("Error: Could not read inotify events");
            break;
        }

        int i = 0;
        while (i < length) {
            struct inotify_event* event = (struct inotify_event*)&buffer[i];

            if (event->len) {
                std::string path = watch_descriptors[event->wd];
                std::string filename = event->name;
                std::string fullpath = path + "/" + filename;
                std::string event_type;

                if (event->mask & IN_CREATE) {
                    event_type = "CREATED";

                    if (event->mask & IN_ISDIR) {
                        try {
                            add_watch_recursive(fd, fullpath);
                        } catch (const std::exception& e) {
                            log_message(std::string("Error adding watch: ") + e.what());
                        }
                    }
                } else if (event->mask & IN_DELETE) {
                    event_type = "DELETED";
                } else if (event->mask & IN_MODIFY) {
                    event_type = "MODIFIED";
                } else if (event->mask & IN_MOVED_FROM) {
                    event_type = "MOVED_FROM";
                } else if (event->mask & IN_MOVED_TO) {
                    event_type = "MOVED_TO";
                } else if (event->mask & IN_ATTRIB) {
                    event_type = "ATTRIBUTES_CHANGED";
                } else {
                    event_type = "UNKNOWN";
                }

                std::string dir_or_file = (event->mask & IN_ISDIR) ? "directory" : "file";
                std::string message = get_current_time() + " " + event_type + " " + dir_or_file + ": " + fullpath;
                log_message(message);
            }

            i += sizeof(struct inotify_event) + event->len;
        }

        if (use_curses) {
            update_curses_display();
        }
    }

    close(fd);
    if (log_file.is_open()) {
        log_file.close();
    }
    if (use_curses) {
        cleanup_curses();
    }

    return 0;
}

void add_watch_recursive(int fd, const std::string& path) {
    DIR* dir = opendir(path.c_str());
    if (!dir) {
        throw std::runtime_error("Could not open directory: " + path);
    }

    uint32_t mask = IN_CREATE | IN_DELETE | IN_MODIFY | IN_MOVED_FROM | IN_MOVED_TO | IN_ATTRIB;
    int wd = inotify_add_watch(fd, path.c_str(), mask);
    if (wd < 0) {
        closedir(dir);
        throw std::runtime_error("Could not add watch for: " + path);
    }

    watch_descriptors[wd] = path;

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        std::string full_path = path + "/" + entry->d_name;
        struct stat st;
        if (stat(full_path.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
            try {
                add_watch_recursive(fd, full_path);
            } catch (const std::exception& e) {
                std::cerr << "Warning: " << e.what() << std::endl;
            }
        }
    }

    closedir(dir);
}

void log_message(const std::string& message) {
    log_history.push_back(message);
    if (log_history.size() > max_log_lines) {
        log_history.erase(log_history.begin());
    }

    if (log_file.is_open()) {
        log_file << message << std::endl;
        log_file.flush();
    }

    if (!use_curses) {
        std::cout << message << std::endl;
    }
}

std::string get_current_time() {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);

    char buffer[80];
    struct tm* timeinfo = localtime(&time_t_now);
    strftime(buffer, sizeof(buffer), "[%Y-%m-%d %H:%M:%S]", timeinfo);

    return std::string(buffer);
}

void print_usage() {
    std::cout << "Usage: dirmon [OPTIONS] DIRECTORY" << std::endl;
    std::cout << "Monitor a directory and log all changes (add, remove, edit) in real-time." << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -l, --log-file=FILE    Log events to FILE" << std::endl;
    std::cout << "  -c, --curses           Use curses UI with live file change feed" << std::endl;
    std::cout << "  -h, --help             Display this help and exit" << std::endl;
}

void setup_curses() {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    start_color();
    init_pair(1, COLOR_GREEN, COLOR_BLACK);
    init_pair(2, COLOR_RED, COLOR_BLACK);
    init_pair(3, COLOR_YELLOW, COLOR_BLACK);
    init_pair(4, COLOR_BLUE, COLOR_BLACK);
    init_pair(5, COLOR_CYAN, COLOR_BLACK);
    refresh();
}

void cleanup_curses() {
    endwin();
}

void update_curses_display() {
    clear();

    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);

    attron(A_BOLD);
    mvprintw(0, 0, "Directory Monitor - Press Ctrl+C to exit");
    attroff(A_BOLD);

    mvhline(1, 0, ACS_HLINE, max_x);

    int display_lines = max_y - 3;
    size_t start_idx = (log_history.size() > static_cast<size_t>(display_lines)) ? 
                      log_history.size() - static_cast<size_t>(display_lines) : 0;
    
    for (size_t i = start_idx, line = 2; i < log_history.size() && line < static_cast<size_t>(max_y - 1); ++i, ++line) {
        const std::string& entry = log_history[i];
        
        if (entry.find("CREATED") != std::string::npos) {
            attron(COLOR_PAIR(1));
        } else if (entry.find("DELETED") != std::string::npos) {
            attron(COLOR_PAIR(2));
        } else if (entry.find("MODIFIED") != std::string::npos) {
            attron(COLOR_PAIR(3));
        } else if (entry.find("MOVED") != std::string::npos) {
            attron(COLOR_PAIR(4));
        } else if (entry.find("ATTRIBUTES") != std::string::npos) {
            attron(COLOR_PAIR(5));
        }
        
        if (entry.length() > static_cast<size_t>(max_x)) {
            mvprintw(line, 0, "%s", entry.substr(0, max_x - 3).c_str());
            mvprintw(line, max_x - 3, "...");
        } else {
            mvprintw(line, 0, "%s", entry.c_str());
        }
        
        attroff(COLOR_PAIR(1) | COLOR_PAIR(2) | COLOR_PAIR(3) | COLOR_PAIR(4) | COLOR_PAIR(5));
    }
    
    refresh();
}
