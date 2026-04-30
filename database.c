#include "database.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static void set_lock(int fd, int type, off_t offset, int whence, off_t len) {
    struct flock fl;
    fl.l_type = type;
    fl.l_whence = whence;
    fl.l_start = offset;
    fl.l_len = len;
    fcntl(fd, F_SETLKW, &fl);
}

static void unlock(int fd, off_t offset, int whence, off_t len) {
    struct flock fl;
    fl.l_type = F_UNLCK;
    fl.l_whence = whence;
    fl.l_start = offset;
    fl.l_len = len;
    fcntl(fd, F_SETLK, &fl);
}

static int get_new_id(int fd, size_t struct_size) {
    off_t file_size = lseek(fd, 0, SEEK_END);
    return (file_size / struct_size) + 1;
}

void init_db() {
    int fd_users = open(DB_USERS, O_CREAT | O_RDWR, 0666);
    int fd_flights = open(DB_FLIGHTS, O_CREAT | O_RDWR, 0666);
    int fd_bookings = open(DB_BOOKINGS, O_CREAT | O_RDWR, 0666);

    if (fd_users >= 0) {
        off_t size = lseek(fd_users, 0, SEEK_END);
        if (size == 0) {
            User admin = {1, "admin", "admin", ROLE_ADMIN};
            write(fd_users, &admin, sizeof(User));
        }
        close(fd_users);
    }
    if (fd_flights >= 0) close(fd_flights);
    if (fd_bookings >= 0) close(fd_bookings);
}

int db_login(const char *username, const char *password, User *out_user) {
    int fd = open(DB_USERS, O_RDONLY);
    if (fd < 0) return 0;

    User u;
    set_lock(fd, F_RDLCK, 0, SEEK_SET, 0);
    while (read(fd, &u, sizeof(User)) == sizeof(User)) {
        if (strcmp(u.username, username) == 0 && strcmp(u.password, password) == 0) {
            *out_user = u;
            unlock(fd, 0, SEEK_SET, 0);
            close(fd);
            return 1;
        }
    }
    unlock(fd, 0, SEEK_SET, 0);
    close(fd);
    return 0;
}

int db_register(const char *username, const char *password, int role, User *out_user) {
    int fd = open(DB_USERS, O_RDWR);
    if (fd < 0) return 0;

    User u;
    set_lock(fd, F_WRLCK, 0, SEEK_SET, 0);
    lseek(fd, 0, SEEK_SET);
    while (read(fd, &u, sizeof(User)) == sizeof(User)) {
        if (strcmp(u.username, username) == 0) {
            unlock(fd, 0, SEEK_SET, 0);
            close(fd);
            return 0; 
        }
    }

    u.id = get_new_id(fd, sizeof(User));
    strcpy(u.username, username);
    strcpy(u.password, password);
    u.role = role;
    
    lseek(fd, 0, SEEK_END);
    write(fd, &u, sizeof(User));
    *out_user = u;

    unlock(fd, 0, SEEK_SET, 0);
    close(fd);
    return 1;
}

int db_add_flight(Flight *flight) {
    int fd = open(DB_FLIGHTS, O_RDWR);
    if (fd < 0) return 0;

    set_lock(fd, F_WRLCK, 0, SEEK_SET, 0);
    flight->id = get_new_id(fd, sizeof(Flight));
    flight->available_seats = flight->total_seats;
    flight->is_active = 1;

    lseek(fd, 0, SEEK_END);
    write(fd, flight, sizeof(Flight));
    
    unlock(fd, 0, SEEK_SET, 0);
    close(fd);
    return 1;
}

int db_mod_flight(Flight *flight) {
    int fd = open(DB_FLIGHTS, O_RDWR);
    if (fd < 0) return 0;

    off_t offset = (flight->id - 1) * sizeof(Flight);
    set_lock(fd, F_WRLCK, offset, SEEK_SET, sizeof(Flight));
    
    lseek(fd, offset, SEEK_SET);
    Flight current;
    if (read(fd, &current, sizeof(Flight)) == sizeof(Flight)) {
        if (current.is_active) {
            int booked_seats = current.total_seats - current.available_seats;
            flight->available_seats = flight->total_seats - booked_seats;
            if (flight->available_seats < 0) flight->available_seats = 0; 
            flight->is_active = 1;

            lseek(fd, offset, SEEK_SET);
            write(fd, flight, sizeof(Flight));
            unlock(fd, offset, SEEK_SET, sizeof(Flight));
            close(fd);
            return 1;
        }
    }
    unlock(fd, offset, SEEK_SET, sizeof(Flight));
    close(fd);
    return 0;
}

int db_del_flight(int flight_id) {
    int fd = open(DB_FLIGHTS, O_RDWR);
    if (fd < 0) return 0;

    off_t offset = (flight_id - 1) * sizeof(Flight);
    set_lock(fd, F_WRLCK, offset, SEEK_SET, sizeof(Flight));
    
    lseek(fd, offset, SEEK_SET);
    Flight flight;
    if (read(fd, &flight, sizeof(Flight)) == sizeof(Flight)) {
        flight.is_active = 0;
        lseek(fd, offset, SEEK_SET);
        write(fd, &flight, sizeof(Flight));
        unlock(fd, offset, SEEK_SET, sizeof(Flight));
        close(fd);
        return 1;
    }
    unlock(fd, offset, SEEK_SET, sizeof(Flight));
    close(fd);
    return 0;
}

int db_get_flights(Flight *out_flights, int max_flights) {
    int fd = open(DB_FLIGHTS, O_RDONLY);
    if (fd < 0) return 0;

    set_lock(fd, F_RDLCK, 0, SEEK_SET, 0);
    Flight f;
    int count = 0;
    while (read(fd, &f, sizeof(Flight)) == sizeof(Flight) && count < max_flights) {
        if (f.is_active) {
            out_flights[count++] = f;
        }
    }
    unlock(fd, 0, SEEK_SET, 0);
    close(fd);
    return count;
}

int db_search_flights(const char *origin, const char *destination, Flight *out_flights, int max_flights) {
    int fd = open(DB_FLIGHTS, O_RDONLY);
    if (fd < 0) return 0;

    set_lock(fd, F_RDLCK, 0, SEEK_SET, 0);
    Flight f;
    int count = 0;
    while (read(fd, &f, sizeof(Flight)) == sizeof(Flight) && count < max_flights) {
        if (f.is_active && 
            (strlen(origin) == 0 || strcmp(f.origin, origin) == 0) &&
            (strlen(destination) == 0 || strcmp(f.destination, destination) == 0)) {
            out_flights[count++] = f;
        }
    }
    unlock(fd, 0, SEEK_SET, 0);
    close(fd);
    return count;
}

int db_book_ticket(int user_id, int flight_id, int seats, Booking *out_booking) {
    int fd_flight = open(DB_FLIGHTS, O_RDWR);
    if (fd_flight < 0) return 0;

    off_t f_offset = (flight_id - 1) * sizeof(Flight);
    set_lock(fd_flight, F_WRLCK, f_offset, SEEK_SET, sizeof(Flight));
    
    lseek(fd_flight, f_offset, SEEK_SET);
    Flight f;
    if (read(fd_flight, &f, sizeof(Flight)) != sizeof(Flight) || !f.is_active || f.available_seats < seats) {
        unlock(fd_flight, f_offset, SEEK_SET, sizeof(Flight));
        close(fd_flight);
        return 0; 
    }

    f.available_seats -= seats;
    lseek(fd_flight, f_offset, SEEK_SET);
    write(fd_flight, &f, sizeof(Flight));

    int fd_booking = open(DB_BOOKINGS, O_RDWR);
    set_lock(fd_booking, F_WRLCK, 0, SEEK_SET, 0); 
    
    out_booking->id = get_new_id(fd_booking, sizeof(Booking));
    out_booking->user_id = user_id;
    out_booking->flight_id = flight_id;
    out_booking->seats_booked = seats;
    out_booking->is_active = 1;

    lseek(fd_booking, 0, SEEK_END);
    write(fd_booking, out_booking, sizeof(Booking));
    
    unlock(fd_booking, 0, SEEK_SET, 0);
    close(fd_booking);

    unlock(fd_flight, f_offset, SEEK_SET, sizeof(Flight));
    close(fd_flight);

    return 1;
}

int db_cancel_ticket(int booking_id, int user_id) {
    int fd_booking = open(DB_BOOKINGS, O_RDWR);
    if (fd_booking < 0) return 0;

    off_t b_offset = (booking_id - 1) * sizeof(Booking);
    set_lock(fd_booking, F_WRLCK, b_offset, SEEK_SET, sizeof(Booking));
    
    lseek(fd_booking, b_offset, SEEK_SET);
    Booking b;
    if (read(fd_booking, &b, sizeof(Booking)) != sizeof(Booking) || !b.is_active || b.user_id != user_id) {
        unlock(fd_booking, b_offset, SEEK_SET, sizeof(Booking));
        close(fd_booking);
        return 0; 
    }

    b.is_active = 0;
    lseek(fd_booking, b_offset, SEEK_SET);
    write(fd_booking, &b, sizeof(Booking));

    int fd_flight = open(DB_FLIGHTS, O_RDWR);
    off_t f_offset = (b.flight_id - 1) * sizeof(Flight);
    set_lock(fd_flight, F_WRLCK, f_offset, SEEK_SET, sizeof(Flight));
    
    lseek(fd_flight, f_offset, SEEK_SET);
    Flight f;
    if (read(fd_flight, &f, sizeof(Flight)) == sizeof(Flight)) {
        f.available_seats += b.seats_booked;
        lseek(fd_flight, f_offset, SEEK_SET);
        write(fd_flight, &f, sizeof(Flight));
    }
    
    unlock(fd_flight, f_offset, SEEK_SET, sizeof(Flight));
    close(fd_flight);

    unlock(fd_booking, b_offset, SEEK_SET, sizeof(Booking));
    close(fd_booking);

    return 1;
}

int db_get_user_bookings(int user_id, Booking *out_bookings, int max_bookings) {
    int fd = open(DB_BOOKINGS, O_RDONLY);
    if (fd < 0) return 0;

    set_lock(fd, F_RDLCK, 0, SEEK_SET, 0);
    Booking b;
    int count = 0;
    while (read(fd, &b, sizeof(Booking)) == sizeof(Booking) && count < max_bookings) {
        if (b.user_id == user_id) {
            out_bookings[count++] = b;
        }
    }
    unlock(fd, 0, SEEK_SET, 0);
    close(fd);
    return count;
}

int db_get_all_bookings(Booking *out_bookings, int max_bookings) {
    int fd = open(DB_BOOKINGS, O_RDONLY);
    if (fd < 0) return 0;

    set_lock(fd, F_RDLCK, 0, SEEK_SET, 0);
    Booking b;
    int count = 0;
    while (read(fd, &b, sizeof(Booking)) == sizeof(Booking) && count < max_bookings) {
        out_bookings[count++] = b;
    }
    unlock(fd, 0, SEEK_SET, 0);
    close(fd);
    return count;
}
