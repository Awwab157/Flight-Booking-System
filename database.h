#ifndef DATABASE_H
#define DATABASE_H

#include "common.h"

void init_db();
int db_login(const char *username, const char *password, User *out_user);
int db_register(const char *username, const char *password, int role, User *out_user);
int db_add_flight(Flight *flight);
int db_mod_flight(Flight *flight);
int db_del_flight(int flight_id);
int db_get_flights(Flight *out_flights, int max_flights);
int db_search_flights(const char *origin, const char *destination, Flight *out_flights, int max_flights);
int db_book_ticket(int user_id, int flight_id, int seats, Booking *out_booking);
int db_cancel_ticket(int booking_id, int user_id);
int db_get_user_bookings(int user_id, Booking *out_bookings, int max_bookings);
int db_get_all_bookings(Booking *out_bookings, int max_bookings);

#endif // DATABASE_H
