#include "ev.h"
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <iomanip>
#include <iostream> // For error logging

std::vector<EVRecord> readEVData(const std::string &filename)
{
    std::vector<EVRecord> records;
    std::ifstream file(filename);

    if (!file.is_open())
    {
        std::cerr << "Error opening file: " << filename << std::endl;
        return records; // Return an empty vector
    }

    std::string line;

    // Skip the header line
    std::getline(file, line);

    while (std::getline(file, line))
    {
        std::istringstream iss(line);
        EVRecord record;
        std::string token;

        try
        {
            // Parse the day
            std::getline(iss, token, ',');
            record.day = std::stoi(token);

            // Parse the weekday
            std::getline(iss, record.weekday, ',');

            // Parse the departure time
            std::getline(iss, record.departureTime, ',');

            if (record.departureTime == "No trips")
            {
                // Set default values for no-trip days
                record.socOnDeparture = 0.0;
                record.arrivalTime = "";
                record.socOnArrival = 0.0;
                record.distanceKm = 0.0;
                record.travelTimeMin = 0;
            }
            else
            {
                // Parse the SOC on departure
                std::getline(iss, token, ',');
                record.socOnDeparture = std::stod(token);

                // Parse the arrival time
                std::getline(iss, record.arrivalTime, ',');

                // Parse the SOC on arrival
                std::getline(iss, token, ',');
                record.socOnArrival = std::stod(token);

                // Parse the distance
                std::getline(iss, token, ',');
                record.distanceKm = std::stod(token);

                // Parse the travel time
                std::getline(iss, token, ',');
                record.travelTimeMin = std::stoi(token);
            }

            records.push_back(record);
        }
        catch (const std::invalid_argument &ia)
        {
            std::cerr << "Invalid argument: " << ia.what() << " in line: " << line << std::endl;
            continue; // Skip this record and continue with the next line
        }
        catch (const std::out_of_range &oor)
        {
            std::cerr << "Out of range error: " << oor.what() << " in line: " << line << std::endl;
            continue; // Skip this record and continue with the next line
        }
    }

    // Update the nextDepartureTime for each record
    for (size_t i = 0; i < records.size(); ++i)
    {
        if (i + 1 < records.size())
        {
            records[i].nextDepartureTime = records[i + 1].departureTime;
        }
        else
        {
            records[i].nextDepartureTime = "No trips";
        }
    }

    return records;
}

void updateEVStatus(EVStatus &status, const std::string &currentTime)
{
    // Update status based on currentTime and EV records
    // ...
}

void printEVRecords(const std::vector<EVRecord> &evRecords)
{
    for (const auto &record : evRecords)
    {
        std::cout << "Day: " << record.day
                  << ", Weekday: " << record.weekday
                  << ", Departure Time: " << record.departureTime
                  << ", SOC on Departure: " << record.socOnDeparture
                  << ", Arrival Time: " << record.arrivalTime
                  << ", SOC on Arrival: " << record.socOnArrival
                  << ", Distance: " << record.distanceKm
                  << ", Travel Time: " << record.travelTimeMin
                  << ", Next Departure Time: " << record.nextDepartureTime
                  << std::endl;
    }
}

int convertTimeToHour(const std::string &timeStr)
{
    if (timeStr == "No trips")
    {
        return -1; // Indicate no trip with a special value, e.g., -1
    }

    try
    {
        // Extract the hour part and convert to integer
        size_t colonPos = timeStr.find(':');
        if (colonPos != std::string::npos)
        {
            return std::stoi(timeStr.substr(0, colonPos));
        }
        else
        {
            throw std::invalid_argument("Invalid time format");
        }
    }
    catch (const std::exception &e)
    {
        //  std::cerr << "Error converting time to hour: " << e.what() << std::endl;
        return -1; // Return a default value or handle the error as appropriate
    }
}

std::vector<EVStatus> generateDailyStatus(const std::vector<EVRecord> &dayRecords, double &previousSOC)
{
    std::vector<EVStatus> hourlyStatuses(24); // 24 hours in a day

    // Part 1: Logic to compute if the EV is home or away at a certain hour
    for (int hour = 0; hour < 24; ++hour)
    {
        EVStatus status;
        status.isAtHome = true; // Assume the EV is at home initially

        for (const auto &record : dayRecords)
        {
            if (record.departureTime != "No trips")
            {
                int departureHour = convertTimeToHour(record.departureTime);
                int arrivalHour = convertTimeToHour(record.arrivalTime);
                if (hour >= departureHour && hour < arrivalHour)
                {
                    status.isAtHome = false; // EV is away during this hours
                    status.powerUsed = record.socOnDeparture - record.socOnArrival;
                    break; // No need to check other records for this hour
                }
                else
                {
                    status.powerUsed = 0.0;
                }
            }
        }

        hourlyStatuses[hour] = status;
    }
    // Part 2: Logic to compute the nextDepartureTime field with days that have trips
    std::string nextDepartureTime = "No trips";
    size_t nextRecordIndex = 0;

    for (int hour = 0; hour < 24; ++hour)
    {
        // Handling "No trips" case
        if (!dayRecords.empty() && dayRecords[0].departureTime == "No trips")
        {
            nextDepartureTime = dayRecords[0].nextDepartureTime;
            if (nextDepartureTime != "No trips" && hour > convertTimeToHour(nextDepartureTime))
            {
                nextDepartureTime = dayRecords[0].nextDepartureTime;
            }
            else
            {
                nextDepartureTime = "No trips";
            }
        }
        else
        {
            // Update next departure time based on the records for days with trips
            if (nextRecordIndex < dayRecords.size() && hour < convertTimeToHour(dayRecords[nextRecordIndex].departureTime))
            {
                nextDepartureTime = dayRecords[nextRecordIndex].departureTime;
            }
            else
            {
                while (nextRecordIndex < dayRecords.size() &&
                       convertTimeToHour(dayRecords[nextRecordIndex].departureTime) <= hour)
                {
                    nextDepartureTime = dayRecords[nextRecordIndex].nextDepartureTime;
                    ++nextRecordIndex;
                }
            }
        }

        // Update the nextDepartureTime for the current hour
        hourlyStatuses[hour].nextDepartureTime = nextDepartureTime;
    }

    // Part 3: Logic to fill the currentSOC value of all EVStatus objects

    for (int hour = 0; hour < 24; ++hour)
    {
        // Set the day number, day name, and hour for each EVStatus object

        hourlyStatuses[hour].hour = hour;
        // Use the SOC value from the previous hour
        hourlyStatuses[hour].currentSOC = previousSOC;

        // Check if the current hour is a departure or arrival hour
        if (!dayRecords.empty())
        {
            for (const auto &record : dayRecords)
            {
                int departureHour = convertTimeToHour(record.departureTime);
                int arrivalHour = convertTimeToHour(record.arrivalTime);
                hourlyStatuses[hour].dayNumber = record.day;
                hourlyStatuses[hour].dayName = record.weekday;

                if (hour == departureHour)
                {
                    // Current hour is a departure hour
                    hourlyStatuses[hour].currentSOC = record.socOnDeparture;
                }
                else if (hour == arrivalHour)
                {
                    // Current hour is an arrival hour
                    hourlyStatuses[hour].currentSOC = record.socOnArrival;
                }
            }
        }

        // Update the previousSOC for the next iteration
        previousSOC = hourlyStatuses[hour].currentSOC;
    }

    return hourlyStatuses; // Return the vector of EVStatus objects
}

int findNumberOfDays(const std::vector<EVRecord> &evRecords)
{
    int maxDay = 0;
    for (const auto &record : evRecords)
    {
        if (record.day > maxDay)
        {
            maxDay = record.day;
        }
    }
    return maxDay;
}

std::vector<std::vector<EVStatus>> generateAllDailyStatuses(const std::vector<EVRecord> &allRecords)
{
    std::vector<std::vector<EVStatus>> allDailyStatuses;
    double lastDaySOC = 32.0; // Initial SOC value for the first day

    int currentDay = 1;
    std::vector<EVRecord> dayRecords;

    // Loop through all records and process each day
    for (const auto &record : allRecords)
    {
        if (record.day != currentDay)
        {
            // Generate daily status for the previous day and add to allDailyStatuses
            auto dailyStatuses = generateDailyStatus(dayRecords, lastDaySOC);
            allDailyStatuses.push_back(dailyStatuses);

            // Update lastDaySOC with the last SOC value of the previous day
            if (!dailyStatuses.empty())
            {
                lastDaySOC = dailyStatuses.back().currentSOC;
            }

            // Reset for the next day
            dayRecords.clear();
            currentDay = record.day;
        }
        dayRecords.push_back(record);
    }

    // Generate and add the last day's statuses
    auto lastDayStatuses = generateDailyStatus(dayRecords, lastDaySOC);
    allDailyStatuses.push_back(lastDayStatuses);
    return allDailyStatuses;
}

void printAllEVStatuses(const std::vector<std::vector<EVStatus>> &allDailyStatuses, const std::vector<EVRecord> &evRecords)
{
    for (const auto &dailyStatuses : allDailyStatuses)
    {
        if (!dailyStatuses.empty())
        {
            std::cout << "Day " << dailyStatuses[0].dayNumber
                      << " (" << dailyStatuses[0].dayName << "):" << std::endl;
        }

        for (const auto &status : dailyStatuses)
        {
            std::cout << "  Hour " << std::setw(2) << status.hour << ": ";
            std::cout << (status.isAtHome ? "At Home" : "Away");
            std::cout << ", Next Departure Time: " << status.nextDepartureTime;
            std::cout << ", Current SOC: " << std::fixed << std::setprecision(2) << status.currentSOC << std::endl;
        }
    }
}
void printAllEVStatusesToCSV(const std::vector<std::vector<EVStatus>> &allDailyStatuses, const std::vector<EVRecord> &evRecords, const std::string &filename)
{
    std::ofstream outFile(filename); // Open the file for writing

    // Check if file is successfully opened
    if (!outFile.is_open())
    {
        std::cerr << "Error opening file: " << filename << std::endl;
        return;
    }

    // Write CSV headers
    outFile << "Day,Day Name,Hour,Status,Next Departure Time,Power Used,Current SOC\n";

    for (const auto &dailyStatuses : allDailyStatuses)
    {
        if (!dailyStatuses.empty())
        {
            std::string dayNumber = std::to_string(dailyStatuses[0].dayNumber);
            std::string dayName = dailyStatuses[0].dayName;

            for (const auto &status : dailyStatuses)
            {
                outFile << dayNumber << "," << dayName << ",";
                outFile << std::setw(2) << status.hour << ",";
                outFile << (status.isAtHome ? "At Home" : "Away") << ",";
                outFile << status.nextDepartureTime << ",";
                outFile << status.powerUsed << ",";
                outFile << std::fixed << std::setprecision(2) << status.currentSOC << "\n";
            }
        }
    }

    outFile.close(); // Close the file
}
