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
	Fove::Headset headset = Fove::Headset::create(Fove::ClientCapabilities::Gaze).getValue();

	// Loop indefinitely
	while (true) {

		// Fetch the left gaze vector
		// You can swap this out with other IFVRHeadset functions to get other types of data
		const Fove::Result<Fove::Stereo<Fove::GazeVector>> gaze = headset.getGazeVectors();
		if (checkError(gaze.getError())) {
			// If there was no error, we are allowed to access the other members of the struct
			cout << "Gaze vectors:   L(" << fixed << setprecision(3)
			     << setw(5) << gaze.getValue().l.vector.x << ", "
			     << setw(5) << gaze.getValue().l.vector.y << ", "
			     << setw(5) << gaze.getValue().l.vector.z << ")   R("
			     << setw(5) << gaze.getValue().r.vector.x << ", "
			     << setw(5) << gaze.getValue().r.vector.y << ", "
			     << setw(5) << gaze.getValue().r.vector.z << ')' << endl;
		}

		// Sleep for a second so that we poll at an interval instead of as much as possible
		this_thread::sleep_for(chrono::seconds { 1 }); // Can use 1s in C++14 and later
	}
} catch (const exception& e) {

	// If an exception is thrown for any reason, log it and exit
	// The FOVE api is designed not to throw externally, but the standard library can
	cerr << "Error: " << e.what() << endl;
	return EXIT_FAILURE;
}
