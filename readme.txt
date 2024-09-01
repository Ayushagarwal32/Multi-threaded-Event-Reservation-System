Multi-threaded Event Reservation System


Overview
This project implements a concurrent event reservation system using POSIX threads (pthread). The system supports booking, canceling, and querying event tickets through multiple worker threads. It utilizes mutexes and condition variables to ensure thread safety and proper synchronization.




Features
Concurrent Reservation Handling: Manages event reservations through multiple worker threads.
Thread Synchronization: Uses mutexes and condition variables to maintain data consistency.

Dynamic Query Processing: Handles various types of queries including ticket booking, ticket cancellation, and seat availability checks.

Simulation of Operation Time: Introduces delays to simulate processing times and system load.
Project Structure

event_reservation.c: Contains the main logic of the reservation system, including thread creation, synchronization, and query processing.




