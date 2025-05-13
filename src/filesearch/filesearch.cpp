#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <fstream>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <getopt.h>
#include <ncurses.h>
#include <ctime>
#include <sstream>
#include <map>
#include <set>
#include <functional>
#include <cctype>
#include <pwd.h>

std::string get_cache_file_path() {
    const char* home_dir = getenv("HOME");
    if (!home_dir) {
        home_dir = getpwuid(getuid())->pw_dir;
    }
    return std::string(home_dir) + "/.filesearch_cache";
}

struct FileInfo {
    std::string path;
    std::string name;
    time_t modified_time;
    
    FileInfo(const std::string& p, const std::string& n, time_t mt)
        : path(p), name(n), modified_time(mt) {}
};

std::vector<FileInfo> file_cache;
bool rebuild_cache = false;
std::string search_path = ".";
std::string search_term;
int selected_index = 0;
int scroll_offset = 0;
int max_display_items = 0;

void print_usage();
void build_cache(const std::string& path);
void save_cache();
void load_cache();
void search_files();
void setup_ncurses();
void cleanup_ncurses();
void display_results(const std::vector<FileInfo>& results);
void open_file(const std::string& path);
int fuzzy_match_score(const std::string& str, const std::string& pattern);

int main(int argc, char* argv[]) {
    static struct option long_options[] = {
        {"path", required_argument, 0, 'p'},
        {"rebuild-cache", no_argument, 0, 'r'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    int opt;
    int option_index = 0;
    
    while ((opt = getopt_long(argc, argv, "p:rh", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'p':
                search_path = optarg;
                break;
            case 'r':
                rebuild_cache = true;
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
        search_term = argv[optind];
    }

    struct stat st;
    if (stat(search_path.c_str(), &st) != 0 || !S_ISDIR(st.st_mode)) {
        std::cerr << "Error: " << search_path << " is not a valid directory." << std::endl;
        return 1;
    }

    if (rebuild_cache) {
        build_cache(search_path);
        save_cache();
    } else {
        load_cache();
        
        if (file_cache.empty()) {
            build_cache(search_path);
            save_cache();
        }
    }

    if (!search_term.empty()) {
        std::vector<FileInfo> results;
        std::vector<std::pair<int, FileInfo>> scored_results;
        
        for (const auto& file : file_cache) {
            int score = fuzzy_match_score(file.name, search_term);
            if (score > 0) {
                scored_results.push_back(std::make_pair(score, file));
            }
        }
        
        std::sort(scored_results.begin(), scored_results.end(), 
                 [](const std::pair<int, FileInfo>& a, const std::pair<int, FileInfo>& b) {
                     return a.first > b.first;
                 });
        
        for (const auto& scored_file : scored_results) {
            results.push_back(scored_file.second);
        }
        
        if (results.empty()) {
            std::cout << "No files matching '" << search_term << "' found." << std::endl;
            return 0;
        }
        
        if (results.size() == 1) {
            open_file(results[0].path);
            return 0;
        }
        
        setup_ncurses();
        display_results(results);
        cleanup_ncurses();
    } else {
        print_usage();
    }

    return 0;
}

void print_usage() {
    std::cout << "Usage: filesearch [OPTIONS] [SEARCH_TERM]" << std::endl;
    std::cout << "Fuzzy search for files and open/edit them instantly." << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -p, --path=PATH        Path to search (default: current directory)" << std::endl;
    std::cout << "  -r, --rebuild-cache    Force rebuild of file cache" << std::endl;
    std::cout << "  -h, --help             Display this help and exit" << std::endl;
    std::cout << std::endl;
    std::cout << "Alias: ff [SEARCH_TERM]" << std::endl;
}

void build_cache(const std::string& path) {
    DIR* dir = opendir(path.c_str());
    if (!dir) {
        return;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        std::string name = entry->d_name;
        std::string full_path = path + "/" + name;
        
        struct stat st;
        if (stat(full_path.c_str(), &st) != 0) {
            continue;
        }
        
        if (S_ISDIR(st.st_mode)) {
            if (name[0] == '.') {
                continue;
            }
            
            build_cache(full_path);
        } else if (S_ISREG(st.st_mode)) {
            file_cache.emplace_back(full_path, name, st.st_mtime);
        }
    }
    
    closedir(dir);
}

void save_cache() {
    std::ofstream cache_file(get_cache_file_path());
    if (!cache_file.is_open()) {
        std::cerr << "Warning: Could not open cache file for writing." << std::endl;
        return;
    }
    
    time_t now = time(nullptr);
    cache_file << "FILESEARCH_CACHE_V1" << std::endl;
    cache_file << now << std::endl;
    cache_file << search_path << std::endl;
    
    for (const auto& file : file_cache) {
        cache_file << file.path << "\t" << file.name << "\t" << file.modified_time << std::endl;
    }
    
    cache_file.close();
}

void load_cache() {
    std::ifstream cache_file(get_cache_file_path());
    if (!cache_file.is_open()) {
        return;
    }
    
    std::string version;
    time_t timestamp;
    std::string cached_path;
    
    std::getline(cache_file, version);
    if (version != "FILESEARCH_CACHE_V1") {
        cache_file.close();
        return;
    }
    
    cache_file >> timestamp >> std::ws;
    std::getline(cache_file, cached_path);
    
    if (cached_path != search_path) {
        cache_file.close();
        rebuild_cache = true;
        return;
    }
    
    time_t now = time(nullptr);
    if (now - timestamp > 86400) {
        cache_file.close();
        rebuild_cache = true;
        return;
    }
    
    file_cache.clear();
    std::string line;
    while (std::getline(cache_file, line)) {
        std::istringstream iss(line);
        std::string path, name;
        time_t mtime;
        
        std::getline(iss, path, '\t');
        std::getline(iss, name, '\t');
        iss >> mtime;
        
        file_cache.emplace_back(path, name, mtime);
    }
    
    cache_file.close();
}

void setup_ncurses() {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    start_color();
    use_default_colors();
    init_pair(1, COLOR_GREEN, -1);  
    init_pair(2, COLOR_CYAN, -1);   
    curs_set(0);  
    
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    max_display_items = max_y - 4;  
}

void cleanup_ncurses() {
    endwin();
}

void display_results(const std::vector<FileInfo>& results) {
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    
    bool running = true;
    while (running) {
        clear();
        
        attron(A_BOLD);
        mvprintw(0, 0, "FileSearch: %s", search_term.c_str());
        attroff(A_BOLD);
        mvprintw(1, 0, "Found %zu files. Use arrow keys to navigate, Enter to open, q to quit.", results.size());
        
        mvhline(2, 0, ACS_HLINE, max_x);
        
        int display_count = std::min(max_display_items, static_cast<int>(results.size()));
        
        if (selected_index < scroll_offset) {
            scroll_offset = selected_index;
        } else if (selected_index >= scroll_offset + display_count) {
            scroll_offset = selected_index - display_count + 1;
        }
        
        for (int i = 0; i < display_count; ++i) {
            int index = i + scroll_offset;
            if (index >= static_cast<int>(results.size())) {
                break;
            }
            
            const FileInfo& file = results[index];
            
            if (index == selected_index) {
                attron(COLOR_PAIR(1) | A_BOLD);
                mvprintw(i + 3, 0, "> %s", file.name.c_str());
                attroff(COLOR_PAIR(1) | A_BOLD);
                
                attron(COLOR_PAIR(2));
                mvprintw(i + 3, max_x - 30, "%s", file.path.substr(0, 30).c_str());
                attroff(COLOR_PAIR(2));
            } else {
                mvprintw(i + 3, 2, "%s", file.name.c_str());
            }
        }
        
        mvhline(max_y - 1, 0, ACS_HLINE, max_x);
        mvprintw(max_y - 1, 0, "Press 'q' to quit");
        
        refresh();
        
        int ch = getch();
        switch (ch) {
            case KEY_UP:
                if (selected_index > 0) {
                    selected_index--;
                }
                break;
            case KEY_DOWN:
                if (selected_index < static_cast<int>(results.size()) - 1) {
                    selected_index++;
                }
                break;
            case '\n':  
                if (!results.empty()) {
                    cleanup_ncurses();
                    open_file(results[selected_index].path);
                    return;
                }
                break;
            case 'q':
            case 'Q':
                running = false;
                break;
        }
    }
}

void open_file(const std::string& path) {
    const char* editor = getenv("EDITOR");
    if (!editor || strlen(editor) == 0) {
        editor = "vi";  
    }
    
    std::string command = std::string(editor) + " \"" + path + "\"";
    system(command.c_str());
}

int fuzzy_match_score(const std::string& str, const std::string& pattern) {
    std::string str_lower = str;
    std::string pattern_lower = pattern;
    std::transform(str_lower.begin(), str_lower.end(), str_lower.begin(), ::tolower);
    std::transform(pattern_lower.begin(), pattern_lower.end(), pattern_lower.begin(), ::tolower);
    
    if (str_lower == pattern_lower) {
        return 1000;
    }
    
    size_t pos = str_lower.find(pattern_lower);
    if (pos != std::string::npos) {
        return 800 - pos;
    }
    
    int score = 0;
    size_t str_idx = 0;
    size_t consecutive = 0;
    
    for (char p : pattern_lower) {
        bool found = false;
        
        while (str_idx < str_lower.size()) {
            if (str_lower[str_idx] == p) {
                found = true;
                consecutive++;
                str_idx++;
                break;
            }
            
            consecutive = 0;
            str_idx++;
        }
        
        if (!found) {
            return 0;
        }
        score += 10 + (consecutive * 5);
    }
    return score;
}
