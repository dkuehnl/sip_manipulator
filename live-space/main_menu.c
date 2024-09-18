#include <stdlib.h>
#include <stdio.h>
#include <ncurses.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>

#include "log.h"

bool server_running = false;

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

int get_pid(const char  *name, FILE *logfile) {
    char path[265];
    int pid;
    char command[265];

    snprintf(command, sizeof(command), "pgrep %s", name);

    FILE *fp = popen(command, "r"); 
    if (fgets(path, sizeof(path)-1, fp)!=NULL) {
        pid = atoi(path); 
    } else {
        fprintf(logfile, "%s - No PID found for %s\n", get_timestamp(), name); 
        return 0;
    }
    fprintf(logfile, "%s - PID %d found for %s\n", get_timestamp(), pid, name);

    pclose(fp);

    return pid;
}

int terminate_process(const int pid, FILE *logfile) {

    if (kill(pid, SIGTERM) == -1) {
        fprintf(logfile, "%s - PID %d could not be terminated, trying to kill..\n", get_timestamp(), pid); 
        if (kill(pid, SIGKILL) == -1) {
            fprintf(logfile, "%s - PID %d could not be killed.\n", get_timestamp(), pid);
            return -1;
        } else {
            fprintf(logfile, "%s - PID %d stoppt by SIGKILL\n", get_timestamp(), pid);
        }
    } else {
        fprintf(logfile, "%s - PID %d stoppt by SIGTERM\n", get_timestamp(), pid);
    }
    return 0;
}

int start_sip_server(FILE *logfile) {
    if (!server_running) {
        server_running = true; 
        system("/usr/src/app/manipulator &");
        fprintf(logfile, "%s - >> Server is running..\n", get_timestamp());
    } else {
        mvprintw(LINES - 3, 0, "> Server already running\n");
    }

    return 0;
}

void stop_sip_server(FILE *logfile) {
    mvprintw(LINES-3, 0, "> Trying to stop Server\n");
    if (server_running) {
        server_running = false; 
        int pid = get_pid("manipulator", logfile);
        if (pid != 0) {
            if (terminate_process(pid, logfile) == 0) {
                fprintf(logfile, "%s - >> Server has stopped.\n", get_timestamp()); 
                mvprintw(LINES - 3, 0, "> Server stopped\n");
            } else {
                mvprintw(LINES -3, 0, "> Error while trying to stop the server - more information in the logs.\n");
                return;
            }
        } else {
            fprintf(logfile, "%s - No Process found to stop.\n", get_timestamp());
            mvprintw(LINES -3, 0, "> Error while trying to stop the server - more information in the logs.\n");
            return;
        }
    } else {
        mvprintw(LINES -3, 0, "> Server currently not running\n"); 
    }
}

void print_menu(WINDOW *menu_win, int highlight, char *choices[], int n_choices) {
    int x, y;
    x = 2;
    y = 2;

    box(menu_win, 0, 0);
    for (int i = 0; i < n_choices; ++i) {
        if (highlight == i +1) {
            wattron(menu_win, A_REVERSE); 
            mvwprintw(menu_win, y, x, "%s", choices[i]);
            wattroff(menu_win, A_REVERSE); 
        } else {
            mvwprintw(menu_win, y, x, "%s", choices[i]);
        }
        ++y;
    }
    wrefresh(menu_win);
}



int main() {
    WINDOW *menu_win;
    int highlight = 1; 
    int choice = 0; 
    int c; 

    char *choices[] = {
        "Start Manipulation-Server", 
        "Stop Server", 
        "Edit HMR", 
        "Change Settings", 
        "Exit",
    };

    int n_choiches = ARRAY_SIZE(choices);

    //Startup-Routine
    //Checking for logfiles
    char *logfiles[] = {
        "/usr/src/app/log_main.txt",
        "/usr/src/app/log_manipulator.txt",
    };
    check_logfiles(logfiles, ARRAY_SIZE(logfiles));
    //opening main-logfile
    FILE *log_main = fopen(logfiles[0], "a"); 
    if(access("/usr/src/app/hmr.txt", (F_OK | R_OK)) != 0) {
        fprintf(log_main, "%s - WARNING: hmr-file does not exist or got wrong permission\n", get_timestamp());
    }
    if (access("/usr/src/app/config.txt", (F_OK | R_OK | W_OK)) != 0) {
        fprintf(log_main, "%s - WARNING: config-file does not exist or got wrong permission\n", get_timestamp());
    }

    //Building Menu-Window
    initscr(); 
    clear(); 
    raw();
    noecho(); 
    cbreak(); 
    curs_set(0); 

    int height = 10, width = 30, starty = (LINES - height) / 2, startx = (COLS - width) / 2;
    menu_win = newwin(height, width, starty, startx); 
    keypad(menu_win, TRUE); 

    if (get_pid("manipulator", log_main) != 0) {
        server_running = true;
        mvprintw(LINES - 2, 0, "Server currently running.\n"); 
    }

    while(1) {
        print_menu(menu_win, highlight, choices, n_choiches);
        c = wgetch(menu_win); 
        switch (c) {
            case KEY_UP: 
                if (highlight == 1) {
                    highlight = n_choiches;
                } else {
                    --highlight;
                }
                break;
            
            case KEY_DOWN:
                if (highlight == n_choiches) {
                    highlight = 1; 
                } else {
                    ++highlight;
                }
                break;
            
            case 10:
                choice = highlight;
                break;
            
            default: 
                refresh(); 
                break;
        }

        if (choice != 0) {
            if (choice == 5) {
                mvprintw(LINES -2, 0, "Exiting..."); 
                refresh(); 
                break; 
            } else if (choice == 1){
                clear();
                mvprintw(LINES -2, 0, "Trying to start the server\n");
                start_sip_server(log_main);
                if (server_running) {
                    mvprintw(LINES -2, 0, "Server sucessfully started\n"); 
                }
                refresh(); 
                choice = 0; 
            } else if (choice == 2) {
                mvprintw(LINES -2, 0, "Trying to stop the server\n");
                refresh();
                stop_sip_server(log_main);
                if (!server_running) {
                    clear();
                    mvprintw(LINES -2, 0, "Server now has stopped\n"); 
                }
                refresh(); 
                choice = 0; 
            } else {
                clear();
                mvprintw(LINES -2, 0, "You chose: %s", choices[choice -1]);
                refresh(); 
                choice = 0; 
            }
        }
    }

    endwin();
    fclose(log_main);
    return 0; 
}