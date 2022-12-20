// FOVE Data Example
// This shows how to fetch and output data from the FOVE service in a console program

#include "FoveAPI.h"
#include "Util.h"
#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <thread>

// Use std namespace for convenience
using namespace std;

// Helper function to check error responses from the FOVE API
bool checkError(const Fove::ErrorCode errorCode)
{
	// Check for error
	switch (errorCode) {
	case Fove::ErrorCode::None:
		return true;

	case Fove::ErrorCode::Connect_NotConnected:
		// This happens when the service is off, or we haven't finished connecting to it
		cerr << "Not connected to service" << endl;
		break;

	case Fove::ErrorCode::Data_NoUpdate:
		// This happens after we connect to the service, but before the first frame of data comes in
		cerr << "No update" << endl;
		break;

	case Fove::ErrorCode::Data_Unreliable:
		// This happens when the user is not present, closes the eye or the headset is not properly positioned
		// => here we just ignore the data
		break;

	case Fove::ErrorCode::Data_LowAccuracy:
		// This happens when the user is looking at extreme positions or closing the eyes
		// Depending of the application you may want to ignore this data
		// => here we just print it the same way as reliable data
		return true;

	default:
		// Less common errors are simply logged with their numeric value
		cerr << "Error #" << EnumToUnderlyingValue(errorCode) << endl;
		break;
	}

	return false;
}

int main() try {
	// Create the Headset object, taking the capabilities we need in our program
	// Different capabilities may enable different hardware or software, so use only the capabilities that are needed
	Fove::Headset headset = Fove::Headset::create(Fove::ClientCapabilities::EyeTracking).getValue();

	// Loop indefinitely
	while (true) {
		// Wait for the next eye frame
		// The current thread will sleep until a new frame comes in
		// This allows us to capture data at the full frame rate of eye tracking and not use too much CPU
		const auto waitResult = headset.waitForProcessedEyeFrame();
		const auto fetchResult = headset.fetchEyeTrackingData();
		if (!checkError(waitResult.getError()) || !checkError(fetchResult.getError())) {
			// Sleep for a second in the event of failure
			// If the wait function fails, it might have returned immediately, and we may eat up 100% of a CPU core if we don't sleep manually
			this_thread::sleep_for(chrono::seconds { 1 }); // Can use 1s in C++14 and later

			continue; // Skip getting the gaze vectors
		}

		// Below we print data
		// Feel free to mess around and call other data query functions,
		// but remember to add the capabilities as needed

		// Get the gaze vector
		const auto gazeData = headset.getCombinedGazeRay();
		if (gazeData.isValid()) {
			cout << "Gaze vectors:   (" << fixed << setprecision(3)
			     << setw(5) << gazeData->direction.x << ", "
			     << setw(5) << gazeData->direction.y << ", "
			     << setw(5) << gazeData->direction.z << ')' << endl;
		} else {
			cout << "getCombinedGazeRay returned error #" << static_cast<int>(gazeData.getError()) << endl;
		}
	}
} catch (...) {

	// If an exception is thrown for any reason, log it and exit
	// The FOVE api is designed not to throw externally, but the standard library can
	cerr << "Error: " << currentExceptionMessage() << endl;
	return EXIT_FAILURE;
}
