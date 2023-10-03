#include <iostream>
#include <cstdlib>
#include <ctime>
#include <mpi.h>

const int NUM_PATIENTS = 50;
const int MIN_HEART_RATE = 60;
const int MAX_HEART_RATE = 100;

struct Patient {
    double height; // in meters
    double weight; // in kilograms
    int heartRate; // beats per minute
};

void generatePatientData(Patient* patients, int size) {
    for (int i = 0; i < size; ++i) {
        patients[i].height = static_cast<double>(rand() % 100 + 150) / 100.0 + 1.0; // Random height between 1.51m and 2.50m
        patients[i].weight = static_cast<double>(rand() % 100 + 500) / 10.0 + 1.0; // Random weight between 50.1kg and 100kg
        patients[i].heartRate = rand() % (MAX_HEART_RATE - MIN_HEART_RATE + 1) + MIN_HEART_RATE; // Random heart rate between 60 and 100 bpm
    }
}

double calculateAverageHeartRate(Patient* patients, int size) {
    int totalHeartRate = 0;
    for (int i = 0; i < size; ++i) {
        totalHeartRate += patients[i].heartRate;
    }
    return static_cast<double>(totalHeartRate) / size;
}

int main() {
    MPI_Init(NULL, NULL);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    std::srand(std::time(0)); // Seed for random number generation

    Patient* patientData = new Patient[NUM_PATIENTS];
    bool dataGenerated = false;

    int choice;
    do {
        MPI_Barrier(MPI_COMM_WORLD);
        if (rank == 0) {
            std::cout << "\nMenu:\n";
            std::cout << "1. Generate Patient Data\n";
            std::cout << "2. Calculate and Display BMI\n";
            std::cout << "3. Calculate and Display Average Heart Rate\n";
            std::cout << "4. Exit\n";
            std::cout << "Enter your choice: ";
            std::cin >> choice;
        }

        MPI_Bcast(&choice, 1, MPI_INT, 0, MPI_COMM_WORLD);

        switch (choice) {
            case 1:
                if (rank == 0) {
                    generatePatientData(patientData, NUM_PATIENTS);
                    std::cout << "Patient data generated." << std::endl;
                    dataGenerated = true;
                }
                break;
            case 2:
                if (dataGenerated) {
                    int localDataSize = NUM_PATIENTS / size;
                    Patient* localData = new Patient[localDataSize];
                    MPI_Scatter(patientData, localDataSize * sizeof(Patient), MPI_BYTE, localData, localDataSize * sizeof(Patient), MPI_BYTE, 0, MPI_COMM_WORLD);

                    // Calculate local BMI values with input validation
                    for (int i = 0; i < localDataSize; ++i) {
                        if (localData[i].height <= 0 || localData[i].weight <= 0) {
                            std::cerr << "Error: Invalid height or weight for patient " << i + 1 << std::endl;
                        } else {
                            double bmi = localData[i].weight / (localData[i].height * localData[i].height);
                            std::cout << "Process " << rank << ", Patient " << i + rank * localDataSize + 1 << ": BMI = " << bmi << std::endl;
                        }
                    }

                    delete[] localData;
                } else {
                    if (rank == 0) {
                        std::cout << "Patient data not generated yet. Please select option 1 first." << std::endl;
                    }
                }
                break;
            case 3:
                if (dataGenerated) {
                    double avgHeartRate = calculateAverageHeartRate(patientData, NUM_PATIENTS);
                    if (rank == 0) {
                        std::cout << "Average Heart Rate: " << avgHeartRate << " bpm" << std::endl;
                    }
                } else {
                    if (rank == 0) {
                        std::cout << "Patient data not generated yet. Please select option 1 first." << std::endl;
                    }
                }
                break;
            case 4:
                // Exit the loop
                break;
            default:
                if (rank == 0) {
                    std::cout << "Invalid choice. Please try again." << std::endl;
                }
                break;
        }
    } while (choice != 4);

    delete[] patientData;

    MPI_Finalize();
    return 0;
}
