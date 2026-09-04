#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#if defined(__has_include)
  #if __has_include(<gtk/gtk.h>)
    #include <gtk/gtk.h>
    #define HAVE_GTK 1
  #endif
#endif

#ifndef HAVE_GTK
  #define HAVE_GTK 0
#endif

#define MAX_floors 11
#define MIN_floors 0
#define ROOF_TOP 10
#define VIP_floor 9 
#define QUEUE_SIZE 50 
#define MAX_CARDs 15
#define CARD_ID_LENGTH 4

// passenger limit 

#define MAX_PASS 8 
#define AVG_WEIGH 70 
#define MAX_WEIGH 560 // 8 x 70

// direction of the lift 

#define UP 1
#define DOWN -1
#define IDLE 0

// mode of the lift 
#define NORMAL 0
#define VIP 1   
#define ALARM 2

// door mode 
#define DOOR_OPEN 1
#define DOOR_CLOSED 0
#define LOCKED 1
#define UNLOCKED 0
#define DOOR_TIMEOUT 5 // seconds


// request structure
typedef struct {
    int floorNumber;
    int destination;
    int priority; // 0 for normal, 1 for VIP
    int alarm; // 0 for no alarm, 1 for alarm
} Request;

typedef struct {
    int current_floor;
    int direction; // UP, DOWN, IDLE
    int mode; // NORMAL, VIP, ALARM
    int door_status; // DOOR_OPEN, DOOR_CLOSED
    int door_lock_status; // LOCKED, UNLOCKED
    int passenger_count;// how many ppl count in cabin 
    int current_weight; //total weight in cabin  
    Request request_queue[QUEUE_SIZE];
    int queue_front;
    int queue_rear;
    int alarm_active; // 0 for no alarm, 1 for alarm
    int on_board_destinations[MAX_PASS];
    int on_board_count ;
} Lift;
// ---- Movement state for timer ----
typedef struct {
    int target_floor;
    gboolean is_vip;
} MoveData;



void init_lift(Lift *l) {
    l->current_floor = 0;
    l->direction = IDLE;
    l->mode = NORMAL;
    l->door_status = DOOR_CLOSED;
    l->door_lock_status = UNLOCKED;
    l->passenger_count = 0;
    l->current_weight = 0;
    l->queue_front = 0;
    l->queue_rear = 0;
    l->alarm_active = 0;
    for (int i = 0; i < QUEUE_SIZE; i++) {
        l->request_queue[i].floorNumber = 0;
        l->request_queue[i].priority = 0;
        l->request_queue[i].alarm = 0;
    }
}

// -----------GLOBAL ----------------------
Lift lift;

#if HAVE_GTK
GtkWidget *floor_labels[MAX_floors];
GtkWidget *log_view;
GtkTextBuffer *log_buffer;
GtkWidget *status_label;
GtkWidget *card_entry;
#endif

int destination_floor = -1; // -1 means no active destination
gboolean lift_is_moving = FALSE;

int validate_card[MAX_CARDs]={
    1001, 1002, 1003, 1004, 1005, 
    1006, 1007, 1008, 1009, 1010,
    1011, 1012, 1013, 1014, 1015  
};

const char *floor_names[] = {
    "Ground" , "1st" , "2nd" , "3rd" , "5th" , "6th" , "7th" , "8th" , "VIP (9)" , "Rooftop"  
};


// ---- Forward declarations ----
void moveLift(Lift* l, int target_floor, gboolean is_vip);
void update_shaft_display(void);
void lobby_door(Lift* l, int action);
void processNextRequest(Lift* l);
gboolean auto_return_to_ground(gpointer data);


//---------LOGGING ------------------------

#if HAVE_GTK
void log_msg (const char *msg){
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(log_buffer, &end);
    gtk_text_buffer_insert(log_buffer, &end, msg, -1);
    gtk_text_buffer_get_end_iter(log_buffer, &end);
    gtk_text_buffer_insert(log_buffer, &end, "\n", -1);
}

void update_shaft_display (void) {
    for (int i = 0; i < MAX_floors; i++) {
        gtk_label_set_text(GTK_LABEL(floor_labels[i]), floor_names[i]);

        GtkWidget *parent = gtk_widget_get_parent(floor_labels[i]);
        if (i == lift.current_floor) {
            gtk_widget_add_css_class(parent, "current-floor");
        } else {
            gtk_widget_remove_css_class(parent, "current-floor");
        }
    }
/* this loops through all 11 floors and sets their text
    if the floor matches the current floor then make the current floor glow
    when the cabin is not at that floor so it go back to normal, no glowing lights */

    char buf[128];
    snprintf(buf, sizeof(buf), "Floor : %s | Passengers: %d/%d | Weight: %dkg/%dkg",
             floor_names[lift.current_floor], lift.passenger_count, MAX_PASS, lift.current_weight, MAX_WEIGH);
    gtk_label_set_text(GTK_LABEL(status_label), buf);
}
#else
void log_msg (const char *msg){
    puts(msg);
}

void update_shaft_display(void) {
    for (int i = 0; i < MAX_floors; i++) {
        gtk_label_set_text(GTK_LABEL(floor_labels[i]), floor_names[i]);

        GtkWidget *parent = gtk_widget_get_parent(floor_labels[i]);

        if (i == lift.current_floor) {
            gtk_widget_add_css_class(parent, "current-floor");
        } else {
            gtk_widget_remove_css_class(parent, "current-floor");
        }

        if (i == destination_floor && i != lift.current_floor) {
            gtk_widget_add_css_class(parent, "destination-floor");
        } else {
            gtk_widget_remove_css_class(parent, "destination-floor");
        }
    }

    char buf[128];
    snprintf(buf, sizeof(buf),
             "Floor: %s | Passengers: %d/%d | Weight: %dkg/%dkg",
             floor_names[lift.current_floor],
             lift.passenger_count, MAX_PASS,
             lift.current_weight, MAX_WEIGH);
    gtk_label_set_text(GTK_LABEL(status_label), buf);
}
#endif

// this tells the weigh of the passenger in the cabin 

// ---------------ADD REQUEST --------------------

int add_request(Lift* l, int floorNumber, int destination, int priority) {
    char buf[128];

    if (floorNumber < MIN_floors || floorNumber > MAX_floors - 1) {
        snprintf(buf, sizeof(buf), "Invalid floor: %d", floorNumber);
        log_msg(buf);
        return 0;
    }
    if (destination < MIN_floors || destination > MAX_floors - 1) {
        snprintf(buf, sizeof(buf), "Invalid destination: %d", destination);
        log_msg(buf);
        return 0;
    }
    if (floorNumber == VIP_floor && priority != VIP) {
        log_msg("Floor 9 is VIP access only. Please scan your card.");
        return 0;
    }
    if (l->queue_rear >= QUEUE_SIZE) {
        log_msg("Request queue is full.");
        return 0;
    }
    if (l->passenger_count >= MAX_PASS) {
        snprintf(buf, sizeof(buf), "Cabin full! Max %d passengers. Request rejected.", MAX_PASS);
        log_msg(buf);
        return 0;
    }
    if (l->current_weight + AVG_WEIGH > MAX_WEIGH) {
        snprintf(buf, sizeof(buf), "Weight limit exceeded! %dkg/%dkg. Request rejected.",
                 l->current_weight, MAX_WEIGH);
        log_msg(buf);
        return 0;
    }

    Request new_request;
    new_request.floorNumber = floorNumber;
    new_request.destination = destination;
    new_request.priority = priority;
    new_request.alarm = 0;

    l->request_queue[l->queue_rear] = new_request;
    l->queue_rear++;

    snprintf(buf, sizeof(buf), "Request added: %s -> %s (priority %d)",
             floor_names[floorNumber], floor_names[destination], priority);
    log_msg(buf);
    return 1;
}

// --------- GET NEXT REQUEST ---------

int get_next_request(Lift* l, int* target_floor, int* target_priority, int* target_destination) {
    if (l->queue_front >= l->queue_rear) return 0;

    int best = l->queue_front;
    for (int i = l->queue_front + 1; i < l->queue_rear; i++) {
        if (l->request_queue[i].priority > l->request_queue[best].priority) {
            best = i;
        }
    }

    *target_floor = l->request_queue[best].floorNumber;
    *target_priority = l->request_queue[best].priority;
    *target_destination = l->request_queue[best].destination;

    for (int i = best; i < l->queue_rear - 1; i++) {
        l->request_queue[i] = l->request_queue[i + 1];
    }
    l->queue_rear--;
    return 1;
}

// -------------is valid card ---------------

int is_valid_card(int cardNumber) {
    for (int i = 0 ; i < MAX_CARDs ; i++) {
        if (validate_card[i] == cardNumber) return 1 ; 
    } return 0 ; 
}


// --------- MOVE LIFT (timer-based) ---------


gboolean auto_return_to_ground(gpointer data) {
    if (lift.queue_front >= lift.queue_rear && !lift_is_moving && lift.current_floor != MIN_floors) {
        log_msg("Returning to ground floor...");
        moveLift(&lift, 0, FALSE);
    }
    return FALSE; // run once
}

gboolean move_one_floor(gpointer data) {
    MoveData *md = (MoveData *)data;

    if (lift.current_floor < md->target_floor) {
        lift.current_floor++;
        lift.direction = UP;
    } else if (lift.current_floor > md->target_floor) {
        lift.current_floor--;
        lift.direction = DOWN;
    }

    update_shaft_display();

    while (g_main_context_pending(NULL)) {
    g_main_context_iteration(NULL, FALSE);
    }

    char buf[64];
    snprintf(buf, sizeof(buf), "  [%s] Floor %s",
             (lift.direction == UP) ? "UP  " : "DOWN",
             floor_names[lift.current_floor]);
    log_msg(buf);


   // arrived at target
    if (lift.current_floor == md->target_floor) {
        lift.direction = IDLE;
        destination_floor = -1;
        snprintf(buf, sizeof(buf), "Arrived at %s.", floor_names[lift.current_floor]);
        log_msg(buf);

        lobby_door(&lift, DOOR_OPEN);
        lobby_door(&lift, DOOR_CLOSED);

        if (md->is_vip) {
            log_msg("*** VIP MODE COMPLETE -> Reverting to NORMAL ***");
            lift.mode = NORMAL;
        }

        g_free(md);
        lift_is_moving = FALSE;

        // check for more requests, or auto-return to ground
        if (lift.queue_front < lift.queue_rear) {
            processNextRequest(&lift);
        } else if (lift.current_floor != MIN_floors) {
            log_msg("No more requests. Returning to ground in 3 seconds...");
            g_timeout_add(3000, (GSourceFunc)auto_return_to_ground, NULL);
        }

        return FALSE; // stop timer
    }

    return TRUE; // keep moving
}

void moveLift(Lift* l, int target_floor, gboolean is_vip) {
    if (lift_is_moving) {
        log_msg("Lift is already moving. Request queued.");
        return;
    }

    destination_floor = target_floor;


    char buf[64];
    snprintf(buf, sizeof(buf), "Moving: %s -> %s",
             floor_names[l->current_floor], floor_names[target_floor]);
    log_msg(buf);

    MoveData *md = g_new(MoveData, 1);
    md->target_floor = target_floor;
    md->is_vip = is_vip;

    lift_is_moving = TRUE;
    g_timeout_add(600, move_one_floor, md); // move one floor every 600ms
}

// --------- LOBBY DOOR ---------
void lobby_door(Lift* l, int action) {
    char buf[128];
    if (action == DOOR_OPEN) {
        l->door_status = DOOR_OPEN;
        log_msg("  >> Doors OPEN");
    } else {
        l->door_status = DOOR_CLOSED;
        log_msg("  >> Doors CLOSED");

        // check who exits at this floor
        int new_count = 0;
        for (int i = 0; i < l->on_board_count; i++) {
            if (l->on_board_destinations[i] == l->current_floor) {
                l->passenger_count--;
                l->current_weight -= AVG_WEIGH;
            } else {
                l->on_board_destinations[new_count++] = l->on_board_destinations[i];
            }
        }
        l->on_board_count = new_count;

        snprintf(buf, sizeof(buf), "  Cabin: %d/%d | %dkg/%dkg",
                 l->passenger_count, MAX_PASS, l->current_weight, MAX_WEIGH);
        log_msg(buf);
        update_shaft_display();
    }
}

// --------- SCAN CARD ---------

void scanCard(Lift* l, int cardNumber) {
    char buf[64];
    snprintf(buf, sizeof(buf), "[CARD SCAN] Scanning card #%d...", cardNumber);
    log_msg(buf);

    if (l->alarm_active) {
        log_msg("Card scan denied. Lift is in emergency lockout.");
        return;
    }

    if (is_valid_card(cardNumber)) {
        log_msg("[ACCESS GRANTED] VIP card accepted! Heading to floor 9.");
        add_request(l, VIP_floor, VIP_floor, VIP); // pickup = destination = floor 9
    } else {
        log_msg("[ACCESS DENIED] Card not recognized.");
    }
}
// --------- PROCESS NEXT REQUEST ---------

void processNextRequest(Lift* l) {
    int target_floor, target_priority, target_destination;

    if (l->alarm_active) {
        log_msg("Lift is in emergency lockout.");
        return;
    }

    if (lift_is_moving) {
        log_msg("Lift is currently moving.");
        return;
    }

    if (l->queue_front >= l->queue_rear) {
        log_msg("No pending requests. Lift idle.");
        return;
    }

    // check for VIP request first
    for (int i = l->queue_front; i < l->queue_rear; i++) {
        if (l->request_queue[i].priority == VIP) {
            log_msg("*** VIP REQUEST DETECTED -> EXPRESS MODE ***");
            l->mode = VIP;
            get_next_request(l, &target_floor, &target_priority, &target_destination);

            // board the VIP passenger's destination now (they're picked up at current pos)
            if (l->on_board_count < MAX_PASS) {
                l->on_board_destinations[l->on_board_count++] = target_destination;
            }

            moveLift(l, VIP_floor, TRUE);
            return;
        }
    }

    // normal request
    get_next_request(l, &target_floor, &target_priority, &target_destination);

    // board this passenger's destination
    if (l->on_board_count < MAX_PASS) {
        l->on_board_destinations[l->on_board_count++] = target_destination;
    }

    moveLift(l, target_floor, FALSE);
}

// --------- TRIGGER ALARM ---------

void triggerAlarm(Lift* l) {
    log_msg("🚨 ALARM TRIGGERED!");
    log_msg("Notifying security and emergency services...");
    l->mode = ALARM;
    l->alarm_active = 1;

    if (l->current_floor != MIN_floors) {
        moveLift(l, 0, FALSE);
    }
    log_msg("Lift is now LOCKED OUT. Awaiting assistance...");
    update_shaft_display();
}
// --------- RESET ALARM ---------

void resetAlarm(Lift* l) {
    if (l->alarm_active) {
        lobby_door(l, DOOR_CLOSED);
        l->mode = NORMAL;
        l->alarm_active = 0;
        log_msg("✅ Alarm reset. Lift back to NORMAL mode.");
        update_shaft_display();
    } else {
        log_msg("No active alarm to reset.");
    }
}



// =============================GTK callback ===========================


#if HAVE_GTK
void on_floor_button_clicked(GtkButton *button, gpointer user_data) {
    int floor = GPOINTER_TO_INT(user_data);
    if (add_request(&lift, floor, floor, NORMAL)) {
        processNextRequest(&lift);
    }
}

void on_scan_clicked(GtkButton *button, gpointer user_data) {
    const char *text = gtk_editable_get_text(GTK_EDITABLE(card_entry));
    int cardNumber = atoi(text);
    scanCard(&lift, cardNumber);
    if (lift.queue_front != lift.queue_rear) {
        processNextRequest(&lift);
    }
}

void on_alarm_clicked(GtkButton *button, gpointer user_data){
    triggerAlarm(&lift);
}

void on_reset_clicked(GtkButton *button, gpointer user_data){
    resetAlarm (&lift);
}

/*========================ACTIVATE (build the window ===========================
===================================================================================)*/

static void activate(GtkApplication *app, gpointer user_data) {
    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "TitanLift Control Panel");
    gtk_window_set_default_size(GTK_WINDOW(window), 750, 600);

    GtkWidget *header = gtk_header_bar_new();
    gtk_header_bar_set_show_title_buttons(GTK_HEADER_BAR(header), TRUE);

    GtkWidget *title = gtk_label_new("TitanLift Control Panel");
    gtk_header_bar_set_title_widget(GTK_HEADER_BAR(header), title);
    gtk_window_set_titlebar(GTK_WINDOW(window), header);


    // dark theme CSS
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_string(provider,
        "window { background-color: #7c8593; }"
        "label { color: #e9cfad; font-family: monospace; }"
        "frame { border: 2px solid #415A77; border-radius: 6px; background-color: #16213E; }"
        "frame.current-floor { border: 2px solid #F2D88A; background-color: #B68D40; }"
        "frame.vip-floor { border: 2px solid #C9A227; background-color: #2A3655; }"
        "frame.rooftop-floor { border: 2px solid #D8C3A5; background-color: #24344D; }"
        "headerbar {background: #102542;color: #978368;}"
        "headerbar label {color: #A07856;font-weight: bold;font-size: 16px;}"
        "headerbar { background-color: #102542; color: #EADBC8; }"
        "headerbar:backdrop { background-color: #0B1929; }"
        "headerbar .title { color: #C9A227; font-weight: bold; }"
        "windowcontrols { color: #EADBC8; }"
        "button { background-color: #102542; color: #EADBC8; border: 1px solid #B89C74; border-radius: 20px; padding: 6px; }"
        "button:hover { background-color: #1a3a5c; border-color: #C9A227; color: #f0d080; }"
        "button > label { color: #EADBC8; }"
        "button, button:not(:hover) { background: #102542; }"
        "entry { background-color: #0F172A; color: #bb9b70; border: 1px solid #415A77; border-radius: 4px; }"
        "textview { background-color: #335d72; color: #a8875c; font-family: monospace; }"
        "textview text { background-color: #16213E; color:#EADBC8; }"
        ".shaft-frame { border: 3px solid #415A77; border-radius: 8px; }"
        ".controls-frame { border: 3px solid #415A77; border-radius: 8px; }"
        ".alarm-btn { background-color: #7A1E1E; color: #ffffff; border-color: #A83232; border-radius: 20px; }"
        ".alarm-btn:hover { background-color: #5a0000; }"
        ".reset-btn { background-color: #2D4A5A; color: #EADBC8; border-color: #415A77; border-radius: 20px; }"
        ".reset-btn:hover { background-color: #2a3a00; }"
        ".status-bar { color: #8B7355; font-weight: bold; font-size: 13px; background-color: #163559; padding: 4px; }"
        ".scan-btn { background-color: #D8C3A5; color: #102542; border-color: #A68A64; border-radius: 20px; }"
        ".scan-btn:hover { background-color: #3d2b1f; }"
        "frame.destination-floor { border: 2px solid #C9A227; background-color: #3a2a00; }"
    );
    gtk_style_context_add_provider_for_display(
        gdk_display_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
    );

    // main vertical layout
    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_margin_top(main_box, 10);
    gtk_widget_set_margin_bottom(main_box, 10);
    gtk_widget_set_margin_start(main_box, 10);
    gtk_widget_set_margin_end(main_box, 10);
    gtk_window_set_child(GTK_WINDOW(window), main_box);

    // status bar
    status_label = gtk_label_new("Floor: Ground | Passengers: 0/8 | Weight: 0kg/560kg");
    gtk_widget_add_css_class(status_label, "status-bar");
    gtk_widget_set_halign(status_label, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(main_box), status_label);

    // content row: shaft + controls
    GtkWidget *content_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_widget_set_vexpand(content_box, TRUE);
    gtk_box_append(GTK_BOX(main_box), content_box);

    // SHAFT (left)
    GtkWidget *shaft_outer = gtk_frame_new("  TITAN LIFT SHAFT  ");
    gtk_widget_add_css_class(shaft_outer, "shaft-frame");

    GtkWidget *shaft_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 3);
    gtk_widget_set_margin_top(shaft_box, 8);
    gtk_widget_set_margin_bottom(shaft_box, 8);
    gtk_widget_set_margin_start(shaft_box, 8);
    gtk_widget_set_margin_end(shaft_box, 8);

    for (int i = MAX_floors - 1; i >= 0; i--) {
        GtkWidget *frame = gtk_frame_new(NULL);
        GtkWidget *label = gtk_label_new(floor_names[i]);
        gtk_widget_set_size_request(frame, 100, 32);
        gtk_frame_set_child(GTK_FRAME(frame), label);
        floor_labels[i] = label;

        if (i == VIP_floor) {
            gtk_widget_add_css_class(frame, "vip-floor");
        } else if (i == ROOF_TOP) {
            gtk_widget_add_css_class(frame, "rooftop-floor");
        }
        gtk_box_append(GTK_BOX(shaft_box), frame);
    }

    gtk_frame_set_child(GTK_FRAME(shaft_outer), shaft_box);
    gtk_box_append(GTK_BOX(content_box), shaft_outer);

    // CONTROLS (right)
    GtkWidget *controls_outer = gtk_frame_new("  CONTROL PANEL  ");
    gtk_widget_add_css_class(controls_outer, "controls-frame");
    gtk_widget_set_hexpand(controls_outer, TRUE);

    GtkWidget *controls_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_top(controls_box, 10);
    gtk_widget_set_margin_bottom(controls_box, 10);
    gtk_widget_set_margin_start(controls_box, 10);
    gtk_widget_set_margin_end(controls_box, 10);
    gtk_frame_set_child(GTK_FRAME(controls_outer), controls_box);
    gtk_box_append(GTK_BOX(content_box), controls_outer);

    // activity log
    GtkWidget *log_label = gtk_label_new("Activity Log:");
    gtk_widget_set_halign(log_label, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(controls_box), log_label);

    GtkWidget *scroll = gtk_scrolled_window_new();
    gtk_widget_set_size_request(scroll, -1, 120);
    log_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(log_view), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(log_view), GTK_WRAP_WORD);
    log_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(log_view));
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), log_view);
    gtk_box_append(GTK_BOX(controls_box), scroll);

    // floor buttons
    GtkWidget *floor_label = gtk_label_new("Select Floor:");
    gtk_widget_set_halign(floor_label, GTK_ALIGN_START);
    gtk_widget_set_margin_top(floor_label, 6);
    gtk_box_append(GTK_BOX(controls_box), floor_label);

    GtkWidget *floor_grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(floor_grid), 6);
    gtk_grid_set_row_spacing(GTK_GRID(floor_grid), 6);
    gtk_grid_set_column_homogeneous(GTK_GRID(floor_grid), TRUE);

    int btn_floors[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 10};
    int btn_count = 10;
    for (int i = 0; i < btn_count; i++) {
        int f = btn_floors[i];
        GtkWidget *btn = gtk_button_new_with_label(floor_names[f]);
        g_signal_connect(btn, "clicked", G_CALLBACK(on_floor_button_clicked), GINT_TO_POINTER(f));
        gtk_widget_set_size_request(btn, 70, 40);
        gtk_grid_attach(GTK_GRID(floor_grid), btn, i % 3, i / 3, 1, 1);
    }
    gtk_box_append(GTK_BOX(controls_box), floor_grid);

    // VIP card scanner
    GtkWidget *card_label = gtk_label_new("VIP Card Scanner -> Floor 9:");
    gtk_widget_set_halign(card_label, GTK_ALIGN_START);
    gtk_widget_set_margin_top(card_label, 8);
    gtk_box_append(GTK_BOX(controls_box), card_label);

    GtkWidget *card_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    card_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(card_entry), "Enter card ID (e.g. 1007)");
    gtk_widget_set_hexpand(card_entry, TRUE);
    g_signal_connect(card_entry, "activate", G_CALLBACK(on_scan_clicked), NULL);
    GtkWidget *scan_btn = gtk_button_new_with_label("Scan");
    gtk_widget_add_css_class(scan_btn, "scan-btn");
    g_signal_connect(scan_btn, "clicked", G_CALLBACK(on_scan_clicked), NULL);
    gtk_box_append(GTK_BOX(card_box), card_entry);
    gtk_box_append(GTK_BOX(card_box), scan_btn);
    gtk_box_append(GTK_BOX(controls_box), card_box);

    // alarm buttons
    GtkWidget *emerg_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_widget_set_margin_top(emerg_box, 8);
    GtkWidget *alarm_btn = gtk_button_new_with_label("Trigger Alarm");
    GtkWidget *reset_btn = gtk_button_new_with_label("Reset Alarm");
    gtk_widget_add_css_class(alarm_btn, "alarm-btn");
    gtk_widget_add_css_class(reset_btn, "reset-btn");
    g_signal_connect(alarm_btn, "clicked", G_CALLBACK(on_alarm_clicked), NULL);
    g_signal_connect(reset_btn, "clicked", G_CALLBACK(on_reset_clicked), NULL);
    gtk_widget_set_hexpand(alarm_btn, TRUE);
    gtk_widget_set_hexpand(reset_btn, TRUE);
    gtk_box_append(GTK_BOX(emerg_box), alarm_btn);
    gtk_box_append(GTK_BOX(emerg_box), reset_btn);
    gtk_box_append(GTK_BOX(controls_box), emerg_box);

    init_lift(&lift);
    update_shaft_display();

    gtk_window_present(GTK_WINDOW(window));
}

// --------- MAIN ---------

int main(int argc, char **argv) {
    GtkApplication *app = gtk_application_new("com.titanlift.app", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}
#else
int main(int argc, char **argv) {
    init_lift(&lift);
    update_shaft_display();
    puts("GTK 4 not found. Install GTK 4 and configure include paths to build the GUI.");
    return 1;
}
#endif