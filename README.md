# **Flight Booking System (Client–Server Architecture)**

## **Overview**

This project implements a **Flight Booking System** using a **client–server architecture** in a **Linux environment**. The system enables **multiple clients** to interact with a **centralized server** for managing flight operations and bookings.

It demonstrates core **Operating Systems concepts** such as **process management**, **inter-process communication (IPC)**, and **concurrency handling using sockets**.

---

## **Features**

### **Admin (Airline Authority)**
- **Add new flights**
- **Modify flight details**
- **Delete flights**
- **View all flights**

### **Customer (User)**
- **View available flights**
- **Book tickets**
- **Cancel reservations**
- **View booking details**

---

## **System Architecture**

### **Server**
- **Handles all client requests**
- **Maintains flight database**
- **Ensures concurrency and data consistency**
- **Performs core business logic**

### **Client**
- **Sends requests to server**
- **Displays results to user**
- **Provides interactive interface**

### **Communication**
- **Implemented using TCP sockets**
- **Supports multiple clients**

---

## **Tech Stack**

- **Language:** C  
- **Platform:** Linux / WSL  

### **Concepts Used**
- **Socket Programming**
- **Fork / Process Management**
- **File Handling (for database)**
- **IPC mechanisms**
- **Synchronization (if implemented)**

---

## **Project Structure**

```
.
├── client.c        # Client-side implementation
├── server.c        # Server-side implementation
├── database.c      # Flight data handling
├── database.h
├── logger.c        # Logging system
├── logger.h
├── common.h        # Shared structures/constants
├── Makefile        # Build automation
├── server.log      # Server logs
```

---

## **Compilation**

```
make
```

**Or manually:**
```
gcc server.c database.c logger.c -o server -lpthread
gcc client.c -o client
```

---

## **Execution**

### **Start Server**
```
./server
```

### **Run Client**
```
./client
```

---

## **How It Works**

1. **Server starts and listens on a specified port**
2. **Multiple clients connect to the server**
3. **Client sends requests (admin/user operations)**
4. **Server processes request and updates database**
5. **Response is sent back to client**

---

## **Concurrency Model**

- Each client is handled using:
  - **fork()** *or* **threads (pthread)** *(depending on implementation)*  
- Ensures **multiple users can interact simultaneously**

---

## **Data Management**

- Data is stored using **files (persistent storage)**
- **CRUD operations** handled via **database.c**

---

## **Logging**

- Server logs important events:
  - **Client connections**
  - **Requests processed**
  - **Errors**

---

## **Key Learning Outcomes**

- **Understanding client–server design**
- **Practical use of sockets in C**
- **Handling multiple clients concurrently**
- **Managing shared data safely**
- **Building modular systems in C**

---

## **Future Improvements**

- **Authentication system**
- **GUI interface**
- **Database integration (MySQL)**
- **Better concurrency control (locks/semaphores)**
- **Network security enhancements**

---

## **Author**

**Awwab Ghole**
