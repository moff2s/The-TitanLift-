// The Titan Lift 

// day 1 // work on the the const 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <windows.h> // for Sleep function



#define MAX_floors 10
#define MIN_floors 0
#define VIP_floor 9 
#define QUEUE_SIZE 50 
#define MAX_CARDs 15
#define true 1 
#define false 0
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

// structure to represent a passenger

// passanger structure
typedef struct {
    int passenger_id;
    int is_vip;
    int destination_floor;
} Passenger;

// floor structure 
typedef struct {
    int floorNumber;
    int floorType; // 0 for normal, 1 for VIP
    int islocked; // 0 for unlocked, 1 for locked
} Floor;    


// request structure
typedef struct {
    int floorNumber;
    int priority; // 0 for normal, 1 for VIP
    int alarm; // 0 for no alarm, 1 for alarm
} Request;

typedef struct {
    int current_floor;
    int direction; // UP, DOWN, IDLE
    int mode; // NORMAL, VIP, ALARM
    int door_status; // DOOR_OPEN, DOOR_CLOSED
    int door_lock_status; // LOCKED, UNLOCKED
    Passenger* passengers[MAX_CARDs];
    int passenger_count;// how many ppl count in cabin 
    int current_weight; //total weight in cabin  
    Request request_queue[QUEUE_SIZE];
    int queue_front;
    int queue_rear;
    int alarm_active; // 0 for no alarm, 1 for alarm
} Lift;

// dayyy 3 // normal movement logic 
void init_lift(Lift* lift) {
    lift->current_floor = 0;
    lift->direction = IDLE;
    lift->mode = NORMAL;
    lift->door_status = DOOR_CLOSED;
    lift->door_lock_status = UNLOCKED;
    lift->passenger_count = 0;
    lift->queue_front = 0;
    lift->queue_rear = 0;
    lift->alarm_active = 0;
    for (int i = 0; i < MAX_CARDs; i++) {
        lift->passengers[i] = NULL;
    }  
    printf("=== THE TITAN LIFT SYSTEM INITIALIZED ===\n"); 
}

// new floor request  ----------- queue management---------------------

int add_request(Lift* lift, int floorNNumber, int priority){
    if (floorNNumber < MIN_floors || floorNNumber > MAX_floors - 1) {
        printf("Invalid floor number: %d\n", floorNNumber);
        return 0 ;
    }
    if (floorNNumber == VIP_floor){
        printf ("Floor %d is VIP acccess only . Please scan your card.\n",VIP_floor);
        return 0 ;
    }
    if (lift ->queue_rear >= QUEUE_SIZE) {
        printf("Request queue is full. Cannot add new request.\n");
        return 0 ;
    
// passenger count check
    if (lift -> queue_rear >= MAX_PASS){
        printf ("Cabin is full! Max %d passengers allowed.Request rejectedd. \n",MAX_PASS);
        return 0 ; 
    }
// weight limit check 
    if (lift -> current_weight + AVG_WEIGH > MAX_WEIGH ){
        printf ("Weight limit exceeded ! Request rejected.\n");
    }
}
// get new request 
    Request new_request;
    new_request.floorNumber = floorNNumber;
    new_request.priority = priority;
    new_request.alarm = 0; // default no alarm

    lift->request_queue[lift->queue_rear] = new_request;
    lift->queue_rear++;
    printf("Added request for floor %d with priority %d\n", floorNNumber, priority);
    return 1 ;
}

// get the next request from the queue

int get_next_request(Lift* lift, int* target_floor, int* target_priority){
    if (lift->queue_front >= lift->queue_rear) {
        printf("No pending requests.\n");
        return 0 ;
    }

    // find the highest priority request
    int highest_priority_index = lift->queue_front;
    for (int i = lift->queue_front + 1; i < lift->queue_rear; i++) {
        if (lift->request_queue[i].priority > lift->request_queue[highest_priority_index].priority) {
            highest_priority_index = i;
        }
    }

    *target_floor = lift->request_queue[highest_priority_index].floorNumber;
    *target_priority = lift->request_queue[highest_priority_index].priority;
    
    
    // remove the request from the queue
    for (int i = highest_priority_index; i < lift->queue_rear - 1; i++) {
        lift->request_queue[i] = lift->request_queue[i + 1];
    }
    lift->queue_rear--;

    printf("Processing request for floor %d with priority %d\n", *target_floor, *target_priority);
    return 1 ;
}

// remove the front request from the queue after processing 
 void remove_request(Lift* lift){
    if ( lift -> queue_front >= lift -> queue_rear) {
        printf("No pending requests to remove.\n");
        return ;
    }
    lift->queue_front++;
 }

 // resett the queue after processing all requests
    void reset_queue(Lift* lift){
        lift->queue_front = 0;
        lift->queue_rear = 0;
        printf("Request queue has been reset.\n");
    }

// ------------------door management ------------------
// open or close the door 

void lobby_door(Lift* lift, int action) {
    if (action == DOOR_OPEN) {
        if (lift->door_lock_status == LOCKED) {
            printf("Door is locked. Cannot open.\n");
            return;
        }
        lift->door_status = DOOR_OPEN;
        printf("Door opened.\n");
        int sleep_time = DOOR_TIMEOUT * 1000; // simulate door open time
        Sleep(sleep_time);
    } else if (action == DOOR_CLOSED) {
        lift->door_status = DOOR_CLOSED;
        printf("Door closed.\n");

// passenger exit 
    if (lift -> passengers > 0 ){
        lift -> passenger_count--;
        lift -> current_weight -= AVG_WEIGH;
    }

        int sleep_time = 1200; // simulate door close time
        Sleep(sleep_time);
    } else {
        printf("Invalid door action: %d\n", action);
    }
} 


// ------------Movement logic ----------------
// move the lift step by step to the target floor
void moveLift (Lift* lift, int target_floor) {
    if (target_floor < MIN_floors || target_floor > MAX_floors) {
        printf("Invalid target floor: %d\n", target_floor);
        return;
    }

    while (lift->current_floor != target_floor) {
        if (lift->current_floor < target_floor) {
            lift->direction = UP;
            lift->current_floor++;
        } else {
            lift->direction = DOWN;
            lift->current_floor--;
        }
        printf("Lift is at floor %d\n", lift->current_floor);
        Sleep(1000); // simulate time taken to move between floors
    }
    lift->direction = IDLE;
    printf("Lift has arrived at floor %d\n", lift->current_floor);
}


// -------------------- processing requests ------------------
void processNextRequest(Lift* lift) {
    int target_floor, target_priority;

    if (lift->alarm_active == 1) {
        printf("Lift is in EMERGENCY mode. Cannot process requests.\n");
        return;
    }

    if (lift -> queue_front >= lift -> queue_rear){
        printf ("NO PENDING REGUEST. Lift idle at floor %d.\n", lift -> current_floor);
        return; 
    }

    // check if any VIP request is waiting -> express mode is overrides normal queqe order

    for (int i = lift -> queue_front ; i < lift -> queue_rear ; i++){
        if (lift -> request_queue[i].priority == VIP){
            printf (" \n *** VIP REQUEST DETECTED -> EXPRESS MODE *** \n");
            lift -> mode = VIP ; 
            moveLift (lift, VIP_floor);
            lobby_door (lift, DOOR_OPEN);
            lobby_door (lift, DOOR_CLOSED);

            // remove the VIP request 
            get_next_request (lift, &target_floor, &target_priority);

            printf (" *** VIP MODE COMPLETE -> REVERTING TO NORMAL *** \n\n");
            lift -> mode = NORMAL ;
            return ;
        }
    }

    get_next_request (lift, &target_floor , &target_priority);

    printf (" ====================================== \n\n ");
    printf ( " NEXT STOP : %d (Priority %d)\n", target_floor, target_priority ) ;
    printf (" ====================================== \n\n");
    Sleep(500);



}

// ================day 4 // VIP mode and alarm mode logic ==================

int validate_card[MAX_CARDs]={
    1001, 1002, 1003, 1004, 1005, 
    1006, 1007, 1008, 1009, 1010,
    1011, 1012, 1013, 1014, 1015  
}; // vip cards 1001-1015

int is_valid_card(int cardNumber) {
    for (int i = 0; i < MAX_CARDs; i++) {
        if (validate_card[i] == cardNumber) {
            return 1; // valid card
        }
    }
    return 0; // invalid card
}

// init the lift with card validation

void scanCard (Lift* lift , int cardNumber) {
    printf("Scanning card number: %d\n", cardNumber);

    if (lift -> alarm_active){
        printf ("Card scan denied. Lift is in emergency lockout. \n");
        return;
    }
   

    if (is_valid_card(cardNumber)) {
        printf("Valid VIP card. Access granted.\n");
        add_request (lift, VIP_floor, VIP);
    } else {
        printf("Invalid card. Access denied.\n");
    }
}


//==================day 5 // alarm mode logic =========================

void triggerAlarm(Lift* lift) {
    printf("ALARM TRIGGERED! Lift is in ALARM mode.\n");
    printf("Resason: Emergency detected (e.g., fire, security breach, etc.)\n");
    printf("Notifying security and emergency services...\n");
    printf("Current floor: %d\n", lift->current_floor);

    lift->mode = ALARM;
    lift->alarm_active = 1;

    printf ("CALLING EMERGENCY SERVICES...\n");
    Sleep(1500);
    printf("EMERGENCY SERVICES NOTIFIED. HELP IS ON THE WAY.\n");   
    // Additional alarm handling logic can be added here (e.g., notify security, stop the lift, etc.)

    if (lift-> current_floor != MIN_floors) {
        printf("Moving lift to ground floor for evacuation...\n");
        moveLift(lift, 0);
    }

    lobby_door(lift, DOOR_OPEN);
    printf("Lift is now in ALARM mode. Awaiting assistance...\n");  

}

void resetAlarm(Lift* lift) {
    if (lift->alarm_active) {
        printf("Resetting alarm...\n");
        lift->mode = NORMAL;
        lift->alarm_active = 0;
        printf("Alarm reset. Lift is back to NORMAL mode.\n");
    } else {
        printf("No active alarm to reset.\n");
    }
}




//==================main function ========================= 


int main() {
    Lift lift;
    init_lift(&lift);

    printf("\n--- Welcome to TitanLift ---\n");
    printf("You will be asked to enter requests as a passenger.\n\n");

    int choice;
    int floor, cardNumber;

    while (1) {
        printf("\n===== TitanLift Menu =====\n");
        printf("1. Request a floor\n");
        printf("2. Scan VIP card\n");
        printf("3. Trigger emergency alarm\n");
        printf("4. Reset alarm\n");
        printf("5. Process next request (move lift)\n");
        printf("6. Quit\n");
        printf("Enter choice: ");
        scanf("%d", &choice);

        if (choice == 1) {
            printf("Enter floor number (%d-%d): ", MIN_floors, MAX_floors);
            scanf("%d", &floor);
            add_request(&lift, floor, NORMAL);

        } else if (choice == 2) {
            printf("Enter your 4-digit card ID: ");
            scanf("%d", &cardNumber);
            scanCard(&lift, cardNumber);

        } else if (choice == 3) {
            triggerAlarm(&lift);

        } else if (choice == 4) {
            resetAlarm(&lift);

        } else if (choice == 5) {
            processNextRequest(&lift);

        } else if (choice == 6) {
            printf("Exiting TitanLift. Goodbye!\n");
            break;

        } else {
            printf("Invalid choice. Try again.\n");
        }
    }
return 0;
}   

