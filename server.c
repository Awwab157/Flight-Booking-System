#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "common.h"
#include "database.h"
#include "logger.h"

void *handle_client(void *client_socket_ptr) {
    int client_sock = *(int *)client_socket_ptr;
    free(client_socket_ptr);

    ClientRequest req;
    ServerResponse res;
    
    int session_user_id = 0;
    int session_role = -1;

    while (read(client_sock, &req, sizeof(ClientRequest)) > 0) {
        memset(&res, 0, sizeof(ServerResponse));
        res.new_session_user_id = session_user_id;
        res.new_session_role = session_role;
        res.item_count = 0;

        switch (req.op) {
            case REQ_LOGIN: {
                User u;
                if (db_login(req.data.auth.username, req.data.auth.password, &u)) {
                    res.status = 1;
                    res.new_session_user_id = u.id;
                    res.new_session_role = u.role;
                    session_user_id = u.id;
                    session_role = u.role;
                    strcpy(res.message, "Login successful.");
                    log_message("User %s logged in.\n", u.username);
                } else {
                    res.status = 0;
                    strcpy(res.message, "Invalid credentials.");
                    log_message("Failed login attempt for user %s.\n", req.data.auth.username);
                }
                break;
            }
            case REQ_REGISTER: {
                User u;
                if (db_register(req.data.auth.username, req.data.auth.password, req.data.auth.role, &u)) {
                    res.status = 1;
                    strcpy(res.message, "Registration successful. Please login.");
                    log_message("New user %s registered.\n", u.username);
                } else {
                    res.status = 0;
                    strcpy(res.message, "Username already exists.");
                }
                break;
            }
            case REQ_ADD_FLIGHT: {
                if (session_role != ROLE_ADMIN) {
                    res.status = 0;
                    strcpy(res.message, "Unauthorized: Admin only.");
                    break;
                }
                Flight f = req.data.flight.flight;
                if (db_add_flight(&f)) {
                    res.status = 1;
                    strcpy(res.message, "Flight added successfully.");
                    log_message("Admin added flight %s.\n", f.flight_number);
                } else {
                    res.status = 0;
                    strcpy(res.message, "Failed to add flight.");
                }
                break;
            }
            case REQ_MOD_FLIGHT: {
                if (session_role != ROLE_ADMIN) {
                    res.status = 0;
                    strcpy(res.message, "Unauthorized: Admin only.");
                    break;
                }
                Flight f = req.data.flight.flight;
                if (db_mod_flight(&f)) {
                    res.status = 1;
                    strcpy(res.message, "Flight modified successfully.");
                    log_message("Admin modified flight ID %d.\n", f.id);
                } else {
                    res.status = 0;
                    strcpy(res.message, "Failed to modify flight.");
                }
                break;
            }
            case REQ_DEL_FLIGHT: {
                if (session_role != ROLE_ADMIN) {
                    res.status = 0;
                    strcpy(res.message, "Unauthorized: Admin only.");
                    break;
                }
                if (db_del_flight(req.data.flight.flight.id)) {
                    res.status = 1;
                    strcpy(res.message, "Flight deleted successfully.");
                    log_message("Admin deleted flight ID %d.\n", req.data.flight.flight.id);
                } else {
                    res.status = 0;
                    strcpy(res.message, "Failed to delete flight.");
                }
                break;
            }
            case REQ_VIEW_FLIGHTS: {
                Flight flights[MAX_FLIGHTS];
                int count = db_get_flights(flights, MAX_FLIGHTS);
                res.status = 1;
                res.item_count = count;
                memcpy(res.payload, flights, count * sizeof(Flight));
                strcpy(res.message, "Flights retrieved.");
                break;
            }
            case REQ_SEARCH_FLIGHTS: {
                Flight flights[MAX_FLIGHTS];
                int count = db_search_flights(req.data.search.origin, req.data.search.destination, flights, MAX_FLIGHTS);
                res.status = 1;
                res.item_count = count;
                memcpy(res.payload, flights, count * sizeof(Flight));
                strcpy(res.message, "Search completed.");
                break;
            }
            case REQ_BOOK_TICKET: {
                if (session_role != ROLE_USER) {
                    res.status = 0;
                    strcpy(res.message, "Unauthorized: User only.");
                    break;
                }
                Booking b;
                if (db_book_ticket(session_user_id, req.data.book.flight_id, req.data.book.seats, &b)) {
                    res.status = 1;
                    strcpy(res.message, "Ticket booked successfully.");
                    log_message("User %d booked %d seats on flight %d.\n", session_user_id, req.data.book.seats, req.data.book.flight_id);
                } else {
                    res.status = 0;
                    strcpy(res.message, "Booking failed (not enough seats or invalid flight).");
                }
                break;
            }
            case REQ_CANCEL_TICKET: {
                if (session_role != ROLE_USER) {
                    res.status = 0;
                    strcpy(res.message, "Unauthorized: User only.");
                    break;
                }
                if (db_cancel_ticket(req.data.cancel.booking_id, session_user_id)) {
                    res.status = 1;
                    strcpy(res.message, "Ticket cancelled successfully.");
                    log_message("User %d cancelled booking %d.\n", session_user_id, req.data.cancel.booking_id);
                } else {
                    res.status = 0;
                    strcpy(res.message, "Cancellation failed (invalid booking or ownership).");
                }
                break;
            }
            case REQ_VIEW_USER_BOOKINGS: {
                if (session_role != ROLE_USER) {
                    res.status = 0;
                    strcpy(res.message, "Unauthorized: User only.");
                    break;
                }
                Booking bookings[MAX_BOOKINGS];
                int count = db_get_user_bookings(session_user_id, bookings, MAX_BOOKINGS);
                res.status = 1;
                res.item_count = count;
                memcpy(res.payload, bookings, count * sizeof(Booking));
                strcpy(res.message, "User bookings retrieved.");
                break;
            }
            case REQ_VIEW_ALL_BOOKINGS: {
                if (session_role != ROLE_ADMIN) {
                    res.status = 0;
                    strcpy(res.message, "Unauthorized: Admin only.");
                    break;
                }
                Booking bookings[MAX_BOOKINGS];
                int count = db_get_all_bookings(bookings, MAX_BOOKINGS);
                res.status = 1;
                res.item_count = count;
                memcpy(res.payload, bookings, count * sizeof(Booking));
                strcpy(res.message, "All bookings retrieved.");
                break;
            }
            case REQ_LOGOUT: {
                log_message("User %d logged out.\n", session_user_id);
                session_user_id = 0;
                session_role = -1;
                res.status = 1;
                res.new_session_user_id = 0;
                res.new_session_role = -1;
                strcpy(res.message, "Logged out successfully.");
                break;
            }
            default:
                res.status = 0;
                strcpy(res.message, "Invalid operation.");
                break;
        }

        write(client_sock, &res, sizeof(ServerResponse));
    }

    log_message("Client disconnected. User ID: %d\n", session_user_id);
    close(client_sock);
    return NULL;
}

int main() {
    init_db();
    init_logger();
    log_message("Server started.\n");

    int server_sock;
    struct sockaddr_un server_addr;

    server_sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sun_family = AF_UNIX;
    strcpy(server_addr.sun_path, SOCKET_PATH);

    unlink(SOCKET_PATH);
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_sock, 5) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on %s\n", SOCKET_PATH);

    while (1) {
        struct sockaddr_un client_addr;
        socklen_t client_len = sizeof(client_addr);
        int *client_sock = malloc(sizeof(int));
        *client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_len);

        if (*client_sock < 0) {
            perror("Accept failed");
            free(client_sock);
            continue;
        }

        log_message("New client connected.\n");

        pthread_t tid;
        pthread_create(&tid, NULL, handle_client, client_sock);
        pthread_detach(tid);
    }

    close_logger();
    close(server_sock);
    return 0;
}
