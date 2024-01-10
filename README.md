# RT_EmbeddedSystem_Project
This Project is established by MohamedFATHALLAH and MaissaBOUZIRI
This project will provide you with a code that shows the value of duty cycle of BeagleBone Black as well as the pwm signal and blinks a LED using the GPIO. Most importantly, this project uses the sqlite3 to create a database to stock the values of the ADC in a table called ADC_Table
    Read ADC Values: The program reads analog values from an ADC pin connected to the energy meter.
    PWM Control: Utilizes Pulse Width Modulation to control the duty cycle based on the ADC values.
    Server Handling: Sets up a server to handle client connections for remote monitoring.

Prerequisites

To compile and run this application, ensure the following prerequisites are met:

    ARM GCC Toolchain installed on your development machine.
    BeagleBone board or a similar embedded system where this application will be deployed.
    SQLite library installed on the development machine and the target system.

Compilation and Execution

    Toolchain Setup: Set up the ARM GCC Toolchain. [Replace_toolchain_directory] with the actual path where the toolchain is installed.

bash

export PATH=$PATH:[Replace_toolchain_directory]/gcc-linaro-7.5.0-2019.12-x86_64_arm-linux-gnueabihf-20240110T180315Z-001/bin

Compile the Code:

Navigate to the code directory and compile using:

bash

arm-linux-gnueabihf-gcc -o output_file main.c -lpthread -lsqlite3

Replace output_file with the desired name for the compiled executable.

Running the Application:

Transfer the compiled executable to your BeagleBone and execute it:

bash

    ./output_file

Application Overview

    Threading: The application utilizes POSIX threads (pthread) to handle different tasks concurrently:
        thread_adc_func: Reads ADC values from a specified path at regular intervals.
        thread_pwm_func: Controls PWM based on ADC readings.
        server_func: Handles client connections and sets up a basic server for remote monitoring.

    Database Handling: Uses SQLite to manage energy meter data.
        db_open: Opens/creates a SQLite database for storing ADC values and timestamps.
        db_create_table: Creates a table within the database for storing ADC values.
        db_insert: Inserts ADC values into the database.
        db_read: Reads values from the database.
        db_countrow: Counts the number of rows in the database.

Contributing

Contributions to enhance and optimize the codebase are welcomed! Feel free to fork this repository, make improvements, and create pull requests.
