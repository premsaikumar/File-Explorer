#include <iostream>
#include <bits/stdc++.h>
#include <iomanip> //for cout formatting
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ctype.h>
#include <cstring>
#include <ctime>
#include <unistd.h>
#include <termios.h>   //for raw mode
#include <signal.h>    // event handling capability
#include <dirent.h>    //for opening and reading dir contents
#include <sys/stat.h>  //for getting file stats
#include <sys/types.h> //contains all additonal types
#include <sys/ioctl.h> //for reading terminal height
#include <pwd.h>       //for username
#include <grp.h>       //for groupname
using namespace std;

// Struct defining the files
struct files
{
    string name;
    string type;
    string dir_path;
    string permissions;
    string user_name;
    string group_name;
    string date;
    string size;
};

// Global variables used and manipulated throughout the code
vector<struct files> files_list;
string current_dir = get_current_dir_name();
string parent_dir, home_dir;
int terminal_height;
int top = 0, bottom;
deque<string> forward_stack;
deque<string> backward_stack;
struct termios orig_termios;
int cursor = 0;
bool in_command_mode = false;

// Auxillary Functions
void get_terminal_height()
{
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    terminal_height = w.ws_row;
}

void disableRawMode()
{
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void enableRawMode()
{
    tcgetattr(STDIN_FILENO, &orig_termios);
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) != 0)
    {
        cout << "Unable to Switch to Normal Mode";
        return;
    }
}

bool comparator(struct files a, struct files b)
{
    if (a.permissions[0] == 'd' && b.permissions[0] == 'd')
    {
        return a.name < b.name;
    }
    else if (a.permissions[0] == 'd' && b.permissions[0] == '-')
    {
        return true;
    }
    else if (a.permissions[0] == '-' && b.permissions[0] == 'd')
    {
        return false;
    }
    else
    {
        return a.name < b.name;
    }
}

void clrscr(void)
{
    cout << "\033[2J\033[1;1H";
}

string get_absolute_path(string path)
{
    string absolute_path;
    if (path[0] == '/')
    {
        // absolute
        absolute_path = path;
    }

    else if (path[0] == '.' && path[1] == '.')
    {
        // relative
        absolute_path = parent_dir + path.substr(2);
    }
    else if (path[0] == '.')
    {
        // relative
        absolute_path = current_dir + path.substr(1);
    }
    else if (path[0] == '~')
    {
        // relative from home
        absolute_path = home_dir + path.substr(1);
    }
    else
    {
        absolute_path = current_dir + "/" + path;
    }
    return absolute_path;
}

string print_permissions(struct stat file)
{
    string permissions = "";
    if (S_ISDIR(file.st_mode))
        permissions += "d";
    else
        permissions += "-";
    if (S_IRUSR & file.st_mode)
        permissions += "r";
    else
        permissions += "-";
    if (S_IWUSR & file.st_mode)
        permissions += "w";
    else
        permissions += "-";
    if (S_IXUSR & file.st_mode)
        permissions += "x";
    else
        permissions += "-";
    if (S_IRGRP & file.st_mode)
        permissions += "r";
    else
        permissions += "-";
    if (S_IWGRP & file.st_mode)
        permissions += "w";
    else
        permissions += "-";
    if (S_IXGRP & file.st_mode)
        permissions += "x";
    else
        permissions += "-";
    if (S_IROTH & file.st_mode)
        permissions += "r";
    else
        permissions += "-";
    if (S_IWOTH & file.st_mode)
        permissions += "w";
    else
        permissions += "-";
    if (S_IXOTH & file.st_mode)
        permissions += "x";
    else
        permissions += "-";

    return permissions;
}

bool one_level_search(string key, string dir_path)
{
    DIR *directory;
    struct dirent *marker;
    struct stat checker;

    directory = opendir(dir_path.c_str());
    if (directory == NULL)
    {
        return false;
    }
    while ((marker = readdir(directory)))
    {
        stat(marker->d_name, &checker);
        if (key == string(marker->d_name))
        {
            return true;
        }
    }
    closedir(directory);
    return false;
}

// Populates the files list global variable with the files in given directory
void populate_files_list(const char *path = current_dir.c_str())
{
    const char *cwd = path;
    current_dir = path;
    parent_dir = current_dir.substr(0, current_dir.find_last_of('/'));
    struct stat t;
    struct dirent *entry;
    DIR *dir = opendir(cwd);
    files_list.clear();
    if (dir != NULL)
    {
        while ((entry = readdir(dir)) != NULL)
        {
            struct files file;
            string file_name = entry->d_name;
            string path_string = string(cwd) + "/" + file_name;
            char *path = new char[path_string.length() + 1];
            strcpy(path, path_string.c_str());
            stat(path, &t);
            char *date = new char[20];
            strftime(date, 20, "%a %d/%m/%y %R", localtime(&(t.st_ctime)));
            string permissions = print_permissions(t);
            passwd *user_name = getpwuid(t.st_uid);
            group *group_name = getgrgid(t.st_gid);
            string type;
            string dir_path;
            if (S_ISDIR(t.st_mode))
            {
                type = "dir";
                dir_path = path_string;
            }
            else
            {
                type = "file";
                dir_path = "";
            }
            files_list.push_back(files());
            files_list[files_list.size() - 1].name = file_name;
            files_list[files_list.size() - 1].type = type;
            files_list[files_list.size() - 1].dir_path = dir_path;
            files_list[files_list.size() - 1].user_name = user_name->pw_name;
            files_list[files_list.size() - 1].group_name = group_name->gr_name;
            files_list[files_list.size() - 1].date = date;
            files_list[files_list.size() - 1].size = to_string(t.st_size) + " Bytes";
            files_list[files_list.size() - 1].permissions = permissions;
        }
        sort(files_list.begin(), files_list.end(), comparator);
        closedir(dir);
    }
}

// Takes the global files list and prints out to terminal
void print_files_list(string mode = "Normal Mode", string status = "")
{
    clrscr();
    get_terminal_height();
    int loop_limit;
    loop_limit = min((int)files_list.size(), terminal_height - 2);
    bottom = top + loop_limit;
    if (cursor == bottom)
    {
        top++;
        bottom++;
    }
    if (cursor >= 0 && cursor == top - 1)
    {
        top--;
        bottom--;
    }
    cout << endl;
    for (int i = top; i < bottom; i++)
    {
        if (i == cursor)
        {
            cout << ">>> " << left;
            cout << "\033[1;37m\033[1;44m";
        }
        else
            cout << "    " << left;
        if (files_list[i].name.length() > 30)
            cout << setw(30) << left << files_list[i].name.substr(0, 25) + "...";
        else
            cout << setw(30) << left << files_list[i].name;
        cout << setw(15) << files_list[i].permissions;
        cout << setw(20) << files_list[i].user_name;
        cout << setw(20) << files_list[i].group_name;
        cout << setw(15) << right << files_list[i].size;
        cout << setw(25) << right << files_list[i].date;
        if (i == cursor)
        {
            cout << "\033[0m";
        }
        cout << endl;
    }
    for (int j = loop_limit; j < terminal_height - 2; j++)
    {
        cout << endl;
    }
    cout << "\033[1;30m\033[1;44m" << mode << ":\033[0m ";
    if (status == "")
        cout << current_dir << endl;
    else
    {
        if (status.substr(0, 5) == "Error")
        {
            cout << "\033[1;30m\033[1;41m" << status << "\033[0m\n";
        }
        else
        {
            cout << "\033[1;30m\033[1;42m" << status << "\033[0m\n";
        }
    }
}

void move_up()
{
    cursor > 0 && cursor--;
    print_files_list("Normal Mode");
}

void move_down()
{
    cursor < files_list.size() - 1 && cursor++;
    print_files_list("Normal Mode");
}

void go_to_home()
{
    backward_stack.push_back(current_dir);
    forward_stack.clear();
    struct passwd *pw = getpwuid(getuid());
    char *home = pw->pw_dir;
    home_dir = home;
    cursor = 0;
    populate_files_list(home);
    print_files_list("Normal Mode");
}

void go_to_parent()
{
    if (current_dir == "/")
        return;
    if (current_dir == "/home")
        parent_dir += "/";
    forward_stack.clear();
    backward_stack.push_back(current_dir);
    char *path = new char[parent_dir.length() + 1];
    strcpy(path, parent_dir.c_str());
    populate_files_list(path);
    cursor = 0;
    print_files_list("Normal Mode");
}

void go_backward()
{
    if (backward_stack.empty())
        return;
    string back = backward_stack.back();
    backward_stack.pop_back();
    forward_stack.push_back(current_dir);
    char *path = new char[back.length() + 1];
    strcpy(path, back.c_str());
    populate_files_list(path);
    cursor = 0;
    print_files_list("Normal Mode");
}

void go_forward()
{
    if (forward_stack.empty())
        return;
    string forwar = forward_stack.back();
    forward_stack.pop_back();
    backward_stack.push_back(current_dir);
    char *path = new char[forwar.length() + 1];
    strcpy(path, forwar.c_str());
    populate_files_list(path);
    cursor = 0;
    print_files_list("Normal Mode");
}

void click()
{
    string click_path = files_list[cursor].dir_path;
    if (files_list[cursor].type == "dir" && files_list[cursor].name != "." && files_list[cursor].name != "..")
    {
        backward_stack.push_back(current_dir);
        forward_stack.clear();
        char *path = new char[click_path.length() + 1];
        strcpy(path, click_path.c_str());
        populate_files_list(path);
        top = 0;
        cursor = 0;
        print_files_list("Normal Mode");
    }
    else if (files_list[cursor].name != "." && files_list[cursor].name != "..")
    {
        pid_t pid = fork();
        if (pid == 0)
        {
            string path_string = string(current_dir) + "/" + files_list[cursor].name;
            char *path = new char[path_string.length() + 1];
            strcpy(path, path_string.c_str());
            execl("/usr/bin/xdg-open", "xdg-open", path, NULL);
            print_files_list("Normal Mode");
            exit(1);
        }
    }
}

void resize(int dummy)
{
    cursor = 0;
    if (in_command_mode)
    {
        print_files_list("Command Mode");
    }
    else
    {
        print_files_list("Normal Mode");
    }
}

// Command mode functions
bool command_search(string key, string dir_path)
{
    DIR *directory;
    struct dirent *marker;
    struct stat checker;

    directory = opendir(dir_path.c_str());
    if (directory == NULL)
    {
        return false;
    }
    while ((marker = readdir(directory)))
    {
        stat(marker->d_name, &checker);
        if (S_ISDIR(checker.st_mode))
        {

            if (key == string(marker->d_name))
            {
                return true;
            }
            if (string(marker->d_name) == "." || string(marker->d_name) == "..")
                continue;
            string next = dir_path + "/" + marker->d_name;
            bool res = command_search(key, next);
            if (res == true)
                return true;
        }
        else
        {
            if (key == string(marker->d_name))
            {
                return true;
            }
        }
    }
    closedir(directory);
    return false;
}

void command_delete_file(string parameters)
{
    // get absolute path
    string delete_file_path = get_absolute_path(parameters);
    // check whether it's a file or directory
    struct stat check1;
    stat(delete_file_path.c_str(), &check1);
    if (S_ISDIR(check1.st_mode))
    {
        return;
    }
    string destination = delete_file_path.substr(0, delete_file_path.find_last_of('/'));
    if (!command_search(delete_file_path.substr(delete_file_path.find_last_of('/') + 1), destination))
    {
        print_files_list("Command Mode", "Error, File to be deleted is not found!");
        return;
    }
    int check2 = remove(delete_file_path.c_str());
    if (check2 != 0)
    {
        print_files_list("Command Mode", "Error deleting file!");
    }
    populate_files_list(current_dir.c_str());
    print_files_list("Command Mode", "Successfully deleted the file.");
}

void command_delete_dir(string parameters)
{
    string delete_dir_path = get_absolute_path(parameters);
    DIR *directory;
    struct dirent *marker;
    directory = opendir(delete_dir_path.c_str());
    if (directory == NULL)
    {
        print_files_list("Command Mode", "Error, Folder to be deleted is not found!");
        return;
    }
    while ((marker = readdir(directory)))
    {
        string dot_check = marker->d_name;
        if (dot_check == "." || dot_check == "..")
        {
            continue;
        }
        string entry_name = delete_dir_path + "/" + dot_check;
        struct stat dir_check;
        stat(entry_name.c_str(), &dir_check);
        if (S_ISDIR(dir_check.st_mode))
        {
            command_delete_dir(entry_name);
        }
        else
        {
            command_delete_file(entry_name);
        }
    }
    closedir(directory);
    remove(delete_dir_path.c_str());
    populate_files_list(current_dir.c_str());
    print_files_list("Command Mode", "Successfully deleted the folder.");
}

bool copy_file(string entry_path, string destination_path, mode_t modes, uid_t user, gid_t group)
{
    FILE *src = fopen(entry_path.c_str(), "rb");
    FILE *dest = fopen(destination_path.c_str(), "wb");
    if (src == NULL || dest == NULL)
    {
        return false;
    }
    char buf[4096];
    size_t size;
    // copy contents
    while (size = fread(buf, 1, 4096, src))
        fwrite(buf, 1, size, dest);
    // copy permissions and owner
    chown(destination_path.c_str(), user, group);
    chmod(destination_path.c_str(), modes);
    fclose(src);
    fclose(dest);
    return true;
}

void copy_dir(string entry_path, string destination_path, mode_t modes, uid_t user, gid_t group)
{
    DIR *directory;
    struct dirent *marker;
    directory = opendir(entry_path.c_str());
    if (directory == NULL)
    {
        cout << "cannot open the directory" << endl;
        return;
    }
    while ((marker = readdir(directory)))
    {
        string dot_check = string(marker->d_name);
        if (dot_check == "." || dot_check == "..")
            continue;
        string entry_name = entry_path + "/" + marker->d_name;
        string destination_name = destination_path + "/" + marker->d_name;
        struct stat check;
        stat(entry_name.c_str(), &check);
        if (S_ISDIR(check.st_mode))
        {
            mkdir(destination_name.c_str(), 0777);
            copy_dir(entry_name, destination_name, check.st_mode, check.st_uid, check.st_gid);
        }
        else
        {
            copy_file(entry_name, destination_name, check.st_mode, check.st_uid, check.st_gid);
        }
    }
    closedir(directory);
}

void command_copy(vector<string> parameters)
{
    // remove command
    parameters.erase(parameters.begin());
    // error handling
    if (parameters.size() <= 1)
    {
        print_files_list("Command Mode", "Error, Must need atleast two arguemnts");
        return;
    }
    // get destination
    string destination_parameter = parameters.back();
    // remove destination, now vector contains files to be copied
    parameters.pop_back();
    // process destination
    string destination = get_absolute_path(destination_parameter);
    // identify the source whether it's a file or directory
    struct stat t;
    for (int i = 0; i < parameters.size(); i++)
    {
        string entry = parameters[i];
        string entry_path = get_absolute_path(entry);
        string entry_name = entry_path.substr(entry_path.find_last_of('/') + 1);
        stat(entry_path.c_str(), &t);
        string destination_path = destination + entry_path.substr(entry_path.find_last_of("/"));
        // if already present
        if (one_level_search(entry_name, destination))
        {
            string decision;
            print_files_list("Command Mode", "Error, The destination contains the file/folder already. Do you want to replace it?");
            getline(cin, decision);
            if (decision != "y")
            {
                print_files_list("Command Mode");
                return;
            }
        }
        if (S_ISDIR(t.st_mode))
        {
            mkdir(destination_path.c_str(), 0777);
            copy_dir(entry_path, destination_path, t.st_mode, t.st_uid, t.st_gid);
        }
        else
        {
            bool success;
            success = copy_file(entry_path, destination_path, t.st_mode, t.st_uid, t.st_gid);
            if (!success)
            {
                print_files_list("Command Mode", "Error, File not found!");
                return;
            }
        }
    }
    populate_files_list(current_dir.c_str());
    print_files_list("Command Mode", "Succesfully copied!");
}

void command_move(vector<string> parameters)
{
    // remove command
    parameters.erase(parameters.begin());
    // error handling
    if (parameters.size() <= 1)
    {
        print_files_list("Command Mode", "Error, Must need atleast two arguemnts");
        return;
    }
    // get destination
    string destination_parameter = parameters.back();
    // remove destination, now vector contains files to be copied
    parameters.pop_back();
    // process destination
    string destination = get_absolute_path(destination_parameter);
    struct stat t;
    for (int i = 0; i < parameters.size(); i++)
    {
        string entry = parameters[i];
        string entry_path = get_absolute_path(entry);
        string entry_name = entry_path.substr(entry_path.find_last_of('/') + 1);
        stat(entry_path.c_str(), &t);
        string destination_path = destination + entry_path.substr(entry_path.find_last_of("/"));
        // if file already present
        if (one_level_search(entry_name, destination))
        {
            string decision;
            print_files_list("Command Mode", "Error, The destination contains the file/folder already. Do you want to replace it?");
            getline(cin, decision);
            if (decision != "y")
                return;
        }
        if (S_ISDIR(t.st_mode))
        {
            mkdir(destination_path.c_str(), 0777);
            copy_dir(entry_path, destination_path, t.st_mode, t.st_uid, t.st_gid);
            command_delete_dir(entry_path);
        }
        else
        {
            copy_file(entry_path, destination_path, t.st_mode, t.st_uid, t.st_gid);
            command_delete_file(entry_path);
        }
    }

    populate_files_list(current_dir.c_str());
    print_files_list("Command Mode", "Succesfully moved!");
}

void command_rename(vector<string> parameters)
{
    // error handling
    if (parameters.size() < 3)
    {
        print_files_list("Command Mode", "Error, Must need atleast two arguemnts");
        return;
    }
    string src_name = parameters[1];
    string abs_src_name = get_absolute_path(parameters[1]);
    string src_path = abs_src_name.substr(0, abs_src_name.find_last_of("/"));
    if (!command_search(src_name, src_path))
    {
        print_files_list("Command Mode", "Error, File not present!");
        return;
    }
    string dest = src_path + "/" + parameters[2];
    rename(get_absolute_path(parameters[1]).c_str(), dest.c_str());
    populate_files_list(current_dir.c_str());
    print_files_list("Command Mode", "Renamed " + parameters[1] + " to " + parameters[2]);
}

void command_create_file(vector<string> parameters)
{
    // error handling
    if (parameters.size() != 3)
    {
        print_files_list("Command Mode", "Error, Must need atleast two arguemnts");
        return;
    }
    parameters.erase(parameters.begin());
    string new_file_name = parameters[0];
    string destination_dir_name;
    if (parameters.size() == 1)
        destination_dir_name = current_dir;
    else
    {
        destination_dir_name = get_absolute_path(parameters[1]);
    }
    // if file already present
    if (one_level_search(new_file_name, destination_dir_name))
    {
        string decision;
        print_files_list("Command Mode", "Error, The destination contains the file already. Do you want to replace it?");
        getline(cin, decision);
        if (decision != "y")
        {
            print_files_list("Command Mode");
            return;
        }
    }
    string destination_file_name = destination_dir_name + "/" + new_file_name;
    FILE *f = fopen(destination_file_name.c_str(), "w+");
    fclose(f);
    populate_files_list(current_dir.c_str());
    print_files_list("Command Mode", "Succesfully created new file!");
}

void command_create_dir(vector<string> parameters)
{
    // error handling
    if (parameters.size() != 3)
    {
        print_files_list("Command Mode", "Error, Must need atleast two arguemnts");
        return;
    }
    parameters.erase(parameters.begin());
    string new_dir_name = parameters[0];
    string destination_name;
    if (parameters.size() == 1)
        destination_name = current_dir;
    else
    {
        destination_name = get_absolute_path(parameters[1]);
    }
    // if file already present
    if (one_level_search(new_dir_name, destination_name))
    {
        string decision;
        print_files_list("Command Mode", "Error, The destination contains the folder already. Do you want to replace it?");
        getline(cin, decision);
        if (decision != "y")
        {
            print_files_list("Command Mode");
            return;
        }
    }
    string destination_dir_name = destination_name + "/" + new_dir_name;
    mkdir(destination_dir_name.c_str(), 0777);
    populate_files_list(current_dir.c_str());
    print_files_list("Command Mode", "Succesfully created new directory!");
}

void command_goto(vector<string> parameters)
{
    string path = get_absolute_path(parameters[1]);
    cout << "goto - " << path << endl;
    if (path == current_dir)
    {
        print_files_list("Command Mode", "Navigating to " + path);
        return;
    }
    backward_stack.push_back(current_dir);
    char *path_char = new char[path.length() + 1];
    strcpy(path_char, path.c_str());
    populate_files_list(path_char);
    if (files_list.size() == 0)
    {
        string curr = backward_stack.back();
        backward_stack.pop_back();
        populate_files_list(curr.c_str());
        print_files_list("Command Mode", "Error, Folder not present!");
    }
    else
    {
        forward_stack.clear();
        print_files_list("Command Mode", "Navigating to " + path);
    }
}

bool command_mode()
{
    string statement;
    while (true)
    {
        getline(cin, statement);
        if (statement[0] == 27)
        {
            enableRawMode();
            clrscr();
            print_files_list();
            break;
        }
        else
        {
            vector<string> parameters;
            string word = "";
            for (auto x : statement)
            {
                if (x == ' ')
                {
                    parameters.push_back(word);
                    word = "";
                }
                else
                {
                    word = word + x;
                }
            }
            parameters.push_back(word);
            if (parameters[0] == "copy")
            {
                command_copy(parameters);
            }
            else if (parameters[0] == "move")
            {
                command_move(parameters);
            }
            else if (parameters[0] == "rename")
            {
                command_rename(parameters);
            }
            else if (parameters[0] == "create_file")
            {
                command_create_file(parameters);
            }
            else if (parameters[0] == "create_dir")
            {
                command_create_dir(parameters);
            }
            else if (parameters[0] == "goto")
            {
                // error handling
                if (parameters.size() != 2)
                {
                    print_files_list("Command Mode", "Error, Must have one arguemnt");
                    continue;
                }
                command_goto(parameters);
            }
            else if (parameters[0] == "search")
            {
                // error handling
                if (parameters.size() != 2)
                {
                    print_files_list("Command Mode", "Error, Must have one arguemnt");
                    continue;
                }
                if (command_search(parameters[1], current_dir))
                    print_files_list("Command Mode", "Found " + parameters[1]);
                else
                    print_files_list("Command Mode", "Error, Did not found " + parameters[1]);
            }
            else if (parameters[0] == "delete_file")
            {
                // error handling
                if (parameters.size() != 2)
                {
                    print_files_list("Command Mode", "Error, Must have one arguemnt");
                    continue;
                }
                parameters.erase(parameters.begin());
                command_delete_file(parameters[0]);
            }
            else if (parameters[0] == "delete_dir")
            {
                // error handling
                if (parameters.size() != 2)
                {
                    print_files_list("Command Mode", "Error, Must have one arguemnt");
                    continue;
                }
                parameters.erase(parameters.begin());
                command_delete_dir(parameters[0]);
            }
            else if (parameters[0] == "quit")
            {
                return true;
            }
            else
                print_files_list("Command Mode", "Error, the command you have entered is not available.");
        }
    }
    in_command_mode = false;
    return false;
}

int main(void)
{
    enableRawMode();
    clrscr();
    struct passwd *pw = getpwuid(getuid());
    char *home = pw->pw_dir;
    home_dir = home;
    populate_files_list();
    signal(SIGWINCH, resize);
    print_files_list();
    char key;
    bool quit = false;
    while (!quit && read(STDIN_FILENO, &key, 1) == 1 && key != 'q')
    {
        switch (key)
        {
        case 65:
            move_up();
            break;
        case 66:
            move_down();
            break;
        case 67:
            go_forward();
            break;
        case 68:
            go_backward();
            break;
        case 'h':
            go_to_home();
            break;
        case 127:
            go_to_parent();
            break;
        case 10:
            click();
            break;
        case ':':
            in_command_mode = true;
            print_files_list("Command Mode");
            disableRawMode();
            quit = command_mode();
            break;
        default:
            break;
        }
    }
    atexit(clrscr);
    atexit(disableRawMode);
    return 0;
}