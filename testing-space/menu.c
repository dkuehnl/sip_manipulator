#include <stdlib.h>
#include <stdio.h>
#include <ncurses.h>
#include <pthread.h>
#include <stdbool.h>
#include <unistd.h>

bool server_running = false;
pthread_t worker_thread;

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

void* server_loop() {
    FILE *logfile = fopen("./log.txt", "a"); 
    int i = 0; 
    while (server_running) {
        fprintf(logfile, "%d: >> Server is running..\n", i++);
        sleep(10); 
    }
    fprintf(logfile, ">> Program has stopped.\n"); 
    fclose(logfile); 
    return NULL; 
}

void start_server() {
    if (!server_running) {
        server_running = true; 
        pthread_create(&worker_thread, NULL, server_loop, NULL); 
        mvprintw(LINES - 3, 0, "> Server started \n");
    } else {
        mvprintw(LINES - 3, 0, "> Server already running\n");
    }
}

void stop_server() {
    if (server_running) {
        server_running = false; 
        pthread_join(worker_thread, NULL); 
        mvprintw(LINES - 3, 0, "> Server stopped\n");
    } else {
        mvprintw(LINES -3, 0, "Server currently not running\n"); 
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
        "Start Programm", 
        "Stop Programm", 
        "Edit Files", 
        "Change Settings", 
        "Exit",
    };

    int n_choiches = ARRAY_SIZE(choices);

    initscr(); 
    clear(); 
    noecho(); 
    cbreak(); 
    curs_set(0); 

    int height = 10, width = 30, starty = (LINES - height) / 2, startx = (COLS - width) / 2;
    menu_win = newwin(height, width, starty, startx); 
    keypad(menu_win, TRUE); 

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
                start_server();
                if (server_running) {
                    mvprintw(LINES -2, 0, "Server sucessfully started\n"); 
                }
                refresh(); 
                choice = 0; 
            } else if (choice == 2) {
                clear();
                mvprintw(LINES -2, 0, "Trying to stop the server\n");
                stop_server();
                if (!server_running) {
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

    return 0; 
}