#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

using namespace std;

namespace
{

vector<unsigned char> load(const string& filename)
{
	ifstream in{filename, std::ifstream::binary};
	return vector<unsigned char>(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

} // namespace

int main(int argc, char* argv[])
{
	if (argc != 5)
	{
		cout << "Usage: " << argv[0] << " shader.spv shader.txt.h shader.txt.c varName";
		return 1;
	}
	const string spv{argv[1]};
	const string txt_h{argv[2]};
	const string txt_c{argv[3]};
	const string varName{argv[4]};
	const auto data{load(spv)};

	// emit header
	{
		ofstream out{txt_h};
		out << "#ifndef _GENERATED_" << varName << "_H_\n";
		out << "#define _GENERATED_" << varName << "_H_\n";
		out << "extern unsigned char " << varName << "[" << data.size() << "];\n";
		out << "#endif\n";
	}

	// emit impl
	{
		ofstream out{txt_c};
		out << "const unsigned char " << varName << "[" << data.size() << "] = {";
		out << hex << setfill('0');
		for (const auto x : data)
			out << "0x" << setw(2) << static_cast<int>(x) << ','; // setw is not persistent
		out << "};\n";
	}
}
