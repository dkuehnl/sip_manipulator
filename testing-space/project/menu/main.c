#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ncurses.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <sys/wait.h>

#include "../shared/include/log.h"
#include "./include/process_handling.h"

bool server_running = false;

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))
#define GLOBAL_CONFIG_PATH "../shared/config.txt"


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

void load_main_config(char *version, char *main_log, char *sip_server_log, char *sip_hmr_log, char *inc_hmr_path, char *out_hmr_path, char *mir_hmr_path,char *sip_bin_path) {
    FILE *config = fopen(GLOBAL_CONFIG_PATH, "r");
    if (config == NULL) {
        printf("CRITIC: Config-File could not be opened!\n");
        exit(EXIT_FAILURE); 
    }

    char line[128];
    while(fgets(line, sizeof(line), config)) {
        if (line[0] == '#' || line[0] == '\n') {
            continue;
        } else {
            if (strncmp(line, "MAIN_LOG", 8) == 0) {
                sscanf(line, "MAIN_LOG=%s", main_log); 
            } else if (strncmp(line, "MAIN_SIP_LOG", 12) == 0) {
                sscanf(line, "MAIN_SIP_LOG=%s", sip_server_log); 
            } else if (strncmp(line, "HMR_LOG", 7) == 0) {
                sscanf(line, "HMR_LOG=%s", sip_hmr_log);  
            }else if (strncmp(line, "VERSION", 7) == 0) {
                sscanf(line, "VERSION=%s", version); 
            } else if (strncmp(line, "INC_HMR_PATH", 12) == 0) {
                sscanf(line, "INC_HMR_PATH=%s", inc_hmr_path);
            } else if (strncmp(line, "OUT_HMR_PATH", 12) == 0) {
                sscanf(line, "OUT_HMR_PATH=%s", out_hmr_path);  
            } else if (strncmp(line, "MIR_HMR_PATH", 12) == 0) {
                sscanf(line, "MIR_HMR_PATH=%s", mir_hmr_path); 
            } else if (strncmp(line,"SIP_BIN", 7) == 0) {
                sscanf(line, "SIP_BIN=%s", sip_bin_path);
            }
        }
    }
    fclose(config);
}


int main() {
    WINDOW *menu_win;
    int highlight = 1; 
    int choice = 0; 
    int c; 

    char *choices[] = {
        "Start Manipulation-Server", 
        "Stop Server", 
        "Edit INC-HMR", 
        "Edit OUT-HMR", 
        "Edit MIR-HMW", 
        "Push HMR to Server",
        "Change Settings", 
        "Exit",
    };

    int n_choiches = ARRAY_SIZE(choices);

    //Startup-Routine
    if(access(GLOBAL_CONFIG_PATH, (F_OK | R_OK)) != 0) {
        printf("(MAIN) CRITIC: No Config-File found - please make sure it in place (%s)!\n", GLOBAL_CONFIG_PATH);
        exit(EXIT_FAILURE); 
    }

    char    main_log[64];
    char    sip_server_log[64];
    char    sip_hmr_log[64];
    char    version[8];
    char    inc_hmr_path[64];
    char    out_hmr_path[64];
    char    mir_hmr_path[64];
    char    sip_bin_path[64];

    load_main_config(version, main_log, sip_server_log, sip_hmr_log, inc_hmr_path, out_hmr_path, mir_hmr_path, sip_bin_path); 
    check_logfile(main_log);
    check_logfile(sip_server_log);
    check_logfile(sip_hmr_log);
    check_file(inc_hmr_path, main_log);
    check_file(out_hmr_path, main_log);
    check_file(mir_hmr_path, main_log);
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

    if (get_pid("manipulator", main_log) != 0) {
        server_running = true;
        mvprintw(LINES - 2, 0, "Server currently running.\n"); 
        refresh();
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
            if (choice == 8) {
                clear(); 
                mvprintw(LINES -2, 0, "Exiting..."); 
                refresh(); 
                sleep(2); 
                error_msg(main_log, "(MAIN) INFO: Program closed by user-action"); 
                break; 
            } else if (choice == 1){
                //Start SIP-Server
                clear();
                mvprintw(LINES -2, 0, "Trying to start the server\n");
                start_external_process("SIP", sip_bin_path, main_log);
                if (server_running) {
                    mvprintw(LINES -2, 0, "Server sucessfully started\n"); 
                }
                refresh(); 
                choice = 0; 
            } else if (choice == 2) {
                //Stop SIP-Server
                mvprintw(LINES -2, 0, "Trying to stop the server\n");
                refresh();
                if (server_running) {
                    stop_sip_server(main_log);
                    if (!server_running) {
                        clear();
                        mvprintw(LINES -2, 0, "Server now has stopped\n"); 
                        refresh(); 
                    }
                } else {
                    mvprintw(LINES -2, 0, "Server currently not running\n");
                    refresh(); 
                }
                choice = 0; 
            } else if (choice == 3){
                //Edit HMR
                clear();
                mvprintw(LINES -2, 0, "Starting Editor for INC-HMR\n"); 
                refresh(); 
                sleep(3); 
                start_external_process("HMR", inc_hmr_path, main_log); 
                clear();
                print_menu(menu_win, highlight, choices, n_choiches);
                mvprintw(LINES -2, 0, "Editor closed\n"); 
                refresh();
                choice = 0; 
            } else if (choice == 4) {
                clear(); 
                mvprintw(LINES -2, 0, "Starting Editor for OUT-HMR\n");
                refresh(); 
                sleep(3); 
                start_external_process("HMR", out_hmr_path, main_log); 
                clear();
                print_menu(menu_win, highlight, choices, n_choiches);
                mvprintw(LINES -2, 0, "Editor closed\n"); 
                refresh();
                choice = 0;
            } else if (choice == 5) {
                clear(); 
                mvprintw(LINES -2, 0, "Starting Editor for MIR-HMR\n");
                refresh(); 
                sleep(3); 
                start_external_process("HMR", mir_hmr_path, main_log); 
                clear();
                print_menu(menu_win, highlight, choices, n_choiches);
                mvprintw(LINES -2, 0, "Editor closed\n"); 
                refresh();
                choice = 0;
            } else if (choice == 6) {
                mvprintw(LINES -2, 0, "Pushing new HMR to Server\n"); 
                refresh(); 
                if (send_sighup(main_log) == 0) {
                    clear(); 
                    mvprintw(LINES -2, 0, "SIGHUP sent to Server\n"); 
                    refresh(); 
                } else {
                    mvprintw(LINES -2, 0, "Error while pushing to Server\n"); 
                    refresh(); 
                }
                choice = 0; 
            } else if (choice == 7){
                //Edit Config
                clear();
                mvprintw(LINES -2, 0, "Starting Editor for Config\n"); 
                refresh(); 
                sleep(3); 
                start_external_process("Config", GLOBAL_CONFIG_PATH, main_log); 
                clear();
                print_menu(menu_win, highlight, choices, n_choiches);
                mvprintw(LINES -2, 0, "Editor closed\n"); 
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