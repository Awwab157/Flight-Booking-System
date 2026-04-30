#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>
#include <sys/types.h>

#define SOCKET_PATH "/tmp/flight_booking.sock"
#define MAX_PAYLOAD_SIZE 8192
#define MAX_FLIGHTS 100
#define MAX_BOOKINGS 100

#define ROLE_USER 0
#define ROLE_ADMIN 1

#define DB_USERS "users.dat"
#define DB_FLIGHTS "flights.dat"
#define DB_BOOKINGS "bookings.dat"

typedef struct {
    int id;
    char username[50];
    char password[50];
    int role; // 0 for User, 1 for Admin
} User;

typedef struct {
    int id;
    char flight_number[20];
    char origin[50];
    char destination[50];
    int total_seats;
    int available_seats;
    int is_active; // 1 if active, 0 if deleted
} Flight;

typedef struct {
    int id;
    int user_id;
    int flight_id;
    int seats_booked;
    int is_active; // 1 if active, 0 if cancelled
} Booking;

typedef enum {
    REQ_LOGIN = 1,
    REQ_REGISTER,
    REQ_ADD_FLIGHT,
    REQ_MOD_FLIGHT,
    REQ_DEL_FLIGHT,
    REQ_VIEW_FLIGHTS,
    REQ_SEARCH_FLIGHTS,
    REQ_BOOK_TICKET,
    REQ_CANCEL_TICKET,
    REQ_VIEW_USER_BOOKINGS,
    REQ_VIEW_ALL_BOOKINGS,
    REQ_LOGOUT
} OpCode;

typedef struct {
    char username[50];
    char password[50];
    int role; 
} PayloadAuth;

typedef struct {
    Flight flight;
} PayloadFlight;

typedef struct {
    int flight_id;
    int seats;
} PayloadBook;

typedef struct {
    int booking_id;
} PayloadCancel;

typedef struct {
    char origin[50];
    char destination[50];
} PayloadSearch;

typedef struct {
    OpCode op;
    int session_user_id; // 0 if not logged in
    int session_role;
    union {
        PayloadAuth auth;
        PayloadFlight flight;
        PayloadBook book;
        PayloadCancel cancel;
        PayloadSearch search;
    } data;
} ClientRequest;

typedef struct {
    int status; // 1 Success, 0 Fail
    int new_session_user_id; // Used for login response
    int new_session_role;
    char message[256];
    int item_count; // Number of items in payload (e.g., number of Flights)
    char payload[MAX_PAYLOAD_SIZE]; // Array of structs (Flight or Booking)
} ServerResponse;

#endif // COMMON_H
