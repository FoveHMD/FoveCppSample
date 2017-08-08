// FOVE Data Example
// This shows how to fetch and output data from the FOVE service in a console program

#include "IFVRHeadset.h"
#include "Util.h"
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <thread>

// Use std namespace for convenience
using namespace std;

int main() try {
	// Create the IFVRHeadset object
	// This is managed by unique_ptr so it will be automatically deleted
	// This program never exits (except possibly by exception), but RAII should always be used for safety
	const unique_ptr<Fove::IFVRHeadset> headset{ Fove::GetFVRHeadset() };

	// Initialiase the headset
	// This allows us to declare what capabilities we would like enabled
	// Doing so may enable hardware or software, and thus consume resources, so it's important to only use capabilities that you know you need
	CheckError(headset->Initialise(Fove::EFVR_ClientCapabilities::Gaze), "Initialise");

	// Loop indefinitely
	while (true) {

		// Fetch the left gaze vector
		// You can swap this out with other IFVRHeadset functions to get other types of data
		Fove::SFVR_GazeVector leftGaze, rightGaze;
		const Fove::EFVR_ErrorCode error = headset->GetGazeVectors(&leftGaze, &rightGaze);

		// Check for error
		switch (error) {

		case Fove::EFVR_ErrorCode::None:
			// If there was no error, we are allowed to access the other members of the struct
			cout << "Left Gaze Vector: (" << fixed << setprecision(3)
			     << setw(6) << leftGaze.vector.x << ", "
			     << setw(6) << leftGaze.vector.y << ", "
			     << setw(6) << leftGaze.vector.z << ')' << endl;
			cout << "Right Gaze Vector: (" << fixed << setprecision(3)
			     << setw(6) << rightGaze.vector.x << ", "
			     << setw(6) << rightGaze.vector.y << ", "
			     << setw(6) << rightGaze.vector.z << ')' << endl;
			break;

		case Fove::EFVR_ErrorCode::Connect_NotConnected:
			cerr << "Not connected to service" << endl;
			break;

		case Fove::EFVR_ErrorCode::Data_NoUpdate:
			cerr << "No update" << endl;
			break;

		default:
			// Less common errors are simply logged with their numeric value
			cerr << "Error #" << EnumToUnderlyingValue(error) << endl;
			break;
		}

		// Sleep for a second so that we poll at an interval instead of as much as possible
		this_thread::sleep_for(1s);
	}
} catch (const exception& e) {

	// If an exception is thrown for any reason, log it and exit
	// The FOVE api is designed not to throw externally, but the standard library can
	cerr << "Error: " << e.what() << endl;
	return EXIT_FAILURE;
}
