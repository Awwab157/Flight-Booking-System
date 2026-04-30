#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "common.h"

int sock;
int session_user_id = 0;
int session_role = -1;

void send_request(ClientRequest *req, ServerResponse *res) {
    write(sock, req, sizeof(ClientRequest));
    read(sock, res, sizeof(ServerResponse));
    if (res->new_session_role != -1 || res->new_session_user_id != 0 || req->op == REQ_LOGOUT) {
        session_user_id = res->new_session_user_id;
        session_role = res->new_session_role;
    }
}

void print_flights(Flight *flights, int count) {
    printf("\n--- Available Flights ---\n");
    printf("%-5s %-15s %-15s %-15s %-10s %-10s\n", "ID", "Flight No", "Origin", "Dest", "Total", "Avail");
    for (int i = 0; i < count; i++) {
        printf("%-5d %-15s %-15s %-15s %-10d %-10d\n", flights[i].id, flights[i].flight_number, flights[i].origin, flights[i].destination, flights[i].total_seats, flights[i].available_seats);
    }
}

void print_bookings(Booking *bookings, int count) {
    printf("\n--- Bookings ---\n");
    printf("%-5s %-10s %-10s %-10s %-10s\n", "ID", "User ID", "Flight ID", "Seats", "Status");
    for (int i = 0; i < count; i++) {
        printf("%-5d %-10d %-10d %-10d %-10s\n", bookings[i].id, bookings[i].user_id, bookings[i].flight_id, bookings[i].seats_booked, bookings[i].is_active ? "Active" : "Cancelled");
    }
}

void admin_menu() {
    int choice;
    while(1) {
        printf("\n=== ADMIN MENU ===\n");
        printf("1. Add Flight\n2. Modify Flight\n3. Delete Flight\n4. View All Flights\n5. View All Bookings\n6. Logout\nChoice: ");
        if (scanf("%d", &choice) != 1) { while(getchar() != '\n'); continue; }
        
        ClientRequest req = {0};
        ServerResponse res = {0};
        req.session_user_id = session_user_id;
        req.session_role = session_role;
        
        if (choice == 1) {
            req.op = REQ_ADD_FLIGHT;
            printf("Flight Number: "); scanf("%s", req.data.flight.flight.flight_number);
            printf("Origin: "); scanf("%s", req.data.flight.flight.origin);
            printf("Destination: "); scanf("%s", req.data.flight.flight.destination);
            printf("Total Seats: "); scanf("%d", &req.data.flight.flight.total_seats);
            send_request(&req, &res);
            printf("Server: %s\n", res.message);
        } else if (choice == 2) {
            req.op = REQ_MOD_FLIGHT;
            printf("Flight ID to modify: "); scanf("%d", &req.data.flight.flight.id);
            printf("New Total Seats: "); scanf("%d", &req.data.flight.flight.total_seats);
            send_request(&req, &res);
            printf("Server: %s\n", res.message);
        } else if (choice == 3) {
            req.op = REQ_DEL_FLIGHT;
            printf("Flight ID to delete: "); scanf("%d", &req.data.flight.flight.id);
            send_request(&req, &res);
            printf("Server: %s\n", res.message);
        } else if (choice == 4) {
            req.op = REQ_VIEW_FLIGHTS;
            send_request(&req, &res);
            if (res.status) print_flights((Flight *)res.payload, res.item_count);
        } else if (choice == 5) {
            req.op = REQ_VIEW_ALL_BOOKINGS;
            send_request(&req, &res);
            if (res.status) print_bookings((Booking *)res.payload, res.item_count);
        } else if (choice == 6) {
            req.op = REQ_LOGOUT;
            send_request(&req, &res);
            printf("Logged out.\n");
            break;
        }
    }
}

void user_menu() {
    int choice;
    while(1) {
        printf("\n=== USER MENU ===\n");
        printf("1. View Flights\n2. Search Flights\n3. Book Ticket\n4. Cancel Ticket\n5. View My Bookings\n6. Logout\nChoice: ");
        if (scanf("%d", &choice) != 1) { while(getchar() != '\n'); continue; }
        
        ClientRequest req = {0};
        ServerResponse res = {0};
        req.session_user_id = session_user_id;
        req.session_role = session_role;

        if (choice == 1) {
            req.op = REQ_VIEW_FLIGHTS;
            send_request(&req, &res);
            if (res.status) print_flights((Flight *)res.payload, res.item_count);
        } else if (choice == 2) {
            req.op = REQ_SEARCH_FLIGHTS;
            printf("Origin (or - for any): "); scanf("%s", req.data.search.origin);
            printf("Destination (or - for any): "); scanf("%s", req.data.search.destination);
            if (strcmp(req.data.search.origin, "-") == 0) strcpy(req.data.search.origin, "");
            if (strcmp(req.data.search.destination, "-") == 0) strcpy(req.data.search.destination, "");
            send_request(&req, &res);
            if (res.status) print_flights((Flight *)res.payload, res.item_count);
        } else if (choice == 3) {
            req.op = REQ_BOOK_TICKET;
            printf("Flight ID: "); scanf("%d", &req.data.book.flight_id);
            printf("Number of Seats: "); scanf("%d", &req.data.book.seats);
            send_request(&req, &res);
            printf("Server: %s\n", res.message);
        } else if (choice == 4) {
            req.op = REQ_CANCEL_TICKET;
            printf("Booking ID to cancel: "); scanf("%d", &req.data.cancel.booking_id);
            send_request(&req, &res);
            printf("Server: %s\n", res.message);
        } else if (choice == 5) {
            req.op = REQ_VIEW_USER_BOOKINGS;
            send_request(&req, &res);
            if (res.status) print_bookings((Booking *)res.payload, res.item_count);
        } else if (choice == 6) {
            req.op = REQ_LOGOUT;
            send_request(&req, &res);
            printf("Logged out.\n");
            break;
        }
    }
}

int main() {
    struct sockaddr_un server_addr;

    sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sun_family = AF_UNIX;
    strcpy(server_addr.sun_path, SOCKET_PATH);

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connect failed (is server running?)");
        exit(EXIT_FAILURE);
    }

    printf("Connected to server.\n");

    int choice;
    while(1) {
        printf("\n1. Login\n2. Register\n3. Exit\nChoice: ");
        if (scanf("%d", &choice) != 1) { while(getchar() != '\n'); continue; }

        if (choice == 1) {
            ClientRequest req = {0};
            ServerResponse res = {0};
            req.op = REQ_LOGIN;
            printf("Username: "); scanf("%s", req.data.auth.username);
            printf("Password: "); scanf("%s", req.data.auth.password);
            send_request(&req, &res);
            printf("Server: %s\n", res.message);
            
            if (res.status) {
                if (session_role == ROLE_ADMIN) admin_menu();
                else user_menu();
            }
        } else if (choice == 2) {
            ClientRequest req = {0};
            ServerResponse res = {0};
            req.op = REQ_REGISTER;
            printf("Username: "); scanf("%s", req.data.auth.username);
            printf("Password: "); scanf("%s", req.data.auth.password);
            req.data.auth.role = ROLE_USER; // Default to user registration
            send_request(&req, &res);
            printf("Server: %s\n", res.message);
        } else if (choice == 3) {
            break;
        }
    }

    close(sock);
    return 0;
}
