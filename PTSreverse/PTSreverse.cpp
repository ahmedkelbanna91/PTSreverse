#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <string>
#include <cctype>
#include <filesystem>
#include <sstream>
#include <iostream>
#include <unordered_set>
#include "include\rang.hpp"


auto ColorEnd = [](std::ostream& os) -> std::ostream& { return os << rang::fg::reset; };
auto Red = [](std::ostream& os) -> std::ostream& { return os << rang::fg::red; };
auto Green = [](std::ostream& os) -> std::ostream& { return os << rang::fg::green; };
auto Yellow = [](std::ostream& os) -> std::ostream& { return os << rang::fg::yellow; };
auto Blue = [](std::ostream& os) -> std::ostream& { return os << rang::fg::blue; };
auto Magenta = [](std::ostream& os) -> std::ostream& { return os << rang::fg::magenta; };
auto Cyan = [](std::ostream& os) -> std::ostream& { return os << rang::fg::cyan; };
auto Gray = [](std::ostream& os) -> std::ostream& { return os << rang::fg::gray; };

bool Duplicates = false, PrecisionAdjusted = false;
int precision = 4;

struct Point {
	double x, y, z, u, v, w;
};

bool isXYZLine(const std::string& line) {
	std::istringstream iss(line);
	double x, y, z, u, v, w;
	iss >> x >> y >> z;
	if (iss.fail()) {
		return false; // Failed to parse XYZ
	}

	iss >> u >> v >> w; // Attempt to parse UVW
	if (!iss.fail() && iss.eof()) {
		return true; // Successfully parsed XYZUVW
	}

	iss.clear(); // Clear the fail state and check if we reached the end of the line after XYZ
	std::string remaining;
	iss >> remaining;
	return remaining.empty(); // True if only XYZ were in the line and nothing else
}

std::vector<Point> ReadPointsFromFile(const std::string& filename) {
	std::vector<Point> points;
	std::ifstream inFile(filename);
	std::string line;
	Point p;

	while (std::getline(inFile, line)) {
		if (!isXYZLine(line)) {
			continue; // Skip lines until an XYZ line is found
		}

		std::istringstream iss(line);
		if (iss >> p.x >> p.y >> p.z) { // Assuming the format is "x y z"
			points.push_back(p);
		}
	}

	return points;
}

std::string GetRotationDirection(const std::vector<Point>& points) {
	double sum = 0;
	for (size_t i = 0; i < points.size(); ++i) {
		size_t nextIndex = (i + 1) % points.size();
		sum += (points[nextIndex].x - points[i].x) * (points[nextIndex].y + points[i].y);
	}

	if (sum > 0) {
		return "cw";
	}
	else if (sum < 0) {
		return "ccw";
	}
	else {
		return "unknown";
	}
}

bool needsPrecisionAdjustment(const std::string& coordinate) {
	size_t dotPosition = coordinate.find('.');
	if (dotPosition != std::string::npos && dotPosition + precision + 1 < coordinate.length()) {
		return true; // Check if there are more than x digits after the decimal point
	}
	return false;
}

std::string adjustPrecision(double value) {
	std::ostringstream oss;
	oss << std::fixed << std::setprecision(precision) << value;
	return oss.str();
}

std::string process_line(const std::string& line) {
	std::istringstream iss(line);
	double x, y, z, u = std::numeric_limits<double>::quiet_NaN(), v = std::numeric_limits<double>::quiet_NaN(), w = std::numeric_limits<double>::quiet_NaN();
	iss >> x >> y >> z;
	iss >> u >> v >> w; // Attempt to read UVW. Will fail for XYZ lines.

	std::string sx = std::to_string(x);
	std::string sy = std::to_string(y);
	std::string sz = std::to_string(z);
	std::string su, sv, sw;
	bool uvwRead = !iss.fail(); // Check if UVW read was successful

	if (uvwRead) {
		su = std::to_string(u);
		sv = std::to_string(v);
		sw = std::to_string(w);
	}

	if (needsPrecisionAdjustment(sx) || needsPrecisionAdjustment(sy) || needsPrecisionAdjustment(sz) ||
		(uvwRead && (needsPrecisionAdjustment(su) || needsPrecisionAdjustment(sv) || needsPrecisionAdjustment(sw)))) {
		PrecisionAdjusted = true;
		std::ostringstream oss;
		oss << adjustPrecision(x) << " " << adjustPrecision(y) << " " << adjustPrecision(z);
		if (uvwRead) {
			oss << " " << adjustPrecision(u) << " " << adjustPrecision(v) << " " << adjustPrecision(w);
		}
		return oss.str();
	}
	else {
		PrecisionAdjusted = false;
		return line;
	}
}

std::vector<std::string> removeConsecutiveDuplicates(const std::vector<std::string>& lines) {
	std::vector<std::string> uniqueLines;
	std::unordered_set<std::string> seen;
	
	for (const auto& line : lines) {
		// Attempt to insert the line into the set to check if it's a duplicate
		if (seen.insert(line).second) { // insert returns a pair, where .second is true if the insertion took place
			uniqueLines.push_back(line); // If the line was inserted successfully, it's not a duplicate
		}
		else {
			Duplicates = true;
		}
	}

	return uniqueLines;
}

int main(int argc, char* argv[]) {
	std::filesystem::path exePath;
	std::filesystem::path outputDir;
	bool _ReadLine = true;
	int minimize = 0;

	if (argc != 4 && argc != 3 && argc != 2) {
		std::cerr << Yellow << "Usage: PTSreverse.exe <input file> <output Dir> <>" << ColorEnd << std::endl;
		std::cerr << Yellow << "   Or: PTSreverse.exe <input file> <>" << ColorEnd << std::endl;
		return 1;
	}

	if (argc == 2) {
		exePath = std::filesystem::current_path();
		outputDir = exePath / "reversed_pts";
	}
	else if (argc == 3) {
		outputDir = argv[2];
	}
	else if (argc == 4) {
		outputDir = argv[2];
		minimize = std::atoi(argv[3]);
	}

	std::string inputFile = argv[1];
	std::string fileName = std::filesystem::path(inputFile).filename().string();

	if (!std::filesystem::exists(outputDir)) {
		std::filesystem::create_directory(outputDir);
	}

	std::filesystem::path outputFile = outputDir / std::filesystem::path(inputFile).filename();
	outputFile.replace_extension(".pts");

	std::ifstream in(inputFile);
	if (!in) {
		std::cerr << Red << "      Error opening input file" << ColorEnd << std::endl;
		return 1;
	}

	std::vector<Point> points = ReadPointsFromFile(inputFile);
	std::string direction = GetRotationDirection(points);
	std::string line;
	std::vector<std::string> lines;
	std::vector<std::string> tempLines;

	while (std::getline(in, line)) {
		if (_ReadLine) {
			if (!isXYZLine(line)) {
				continue; // Skip lines until an XYZ line is found
			}
			line = process_line(line); // Process the line to fixed precision
			lines.push_back(line);
		}
		if (minimize == 1) {
			_ReadLine = !_ReadLine; // Toggle the flag to skip the next line
		}
		tempLines.push_back(line);
	}

	lines = removeConsecutiveDuplicates(lines); // Remove duplicates while preserving order

	std::ofstream out(outputFile);
	if (!out) {
		std::cerr << Red << "      Error opening output file" << ColorEnd << std::endl;
		return 1;
	}

	if (direction == "cw") {
		std::reverse(lines.begin(), lines.end()); // Reverse the vector
		out << "#CW. Done reversal (Created by Banna)" << "\n";
		for (const auto& l : lines) {
			out << l << "\n";
		}

		std::cout << " >>     " << Yellow << fileName << ColorEnd << " - " <<
			Red << "CW" << ColorEnd << " - " << lines.size() << "/" << tempLines.size()
			<< " Points - " << Yellow << (Duplicates ? (PrecisionAdjusted ? "Removing Duplicates, Adjusting Precision, " : "Removing Duplicates, ") : PrecisionAdjusted ? "Adjusting Precision, " : "") << "Reversing points" << ColorEnd << std::endl;
	}
	else if (direction == "ccw") {
		out << "#CCW. No reversal needed (Created by Banna)" << "\n";
		for (const auto& l : lines) {
			out << l << "\n";
		}

		std::cout << " >>     " << Yellow << fileName << ColorEnd << " - " <<
			Green << "CCW" << ColorEnd << " - " << lines.size() << "/" << tempLines.size()
			<< " Points - " << Yellow << (Duplicates ? (PrecisionAdjusted ? "Removing Duplicates, Adjusting Precision, " : "Removing Duplicates, ") : PrecisionAdjusted ? "Adjusting Precision, ":"") << "No reversal needed" << ColorEnd << std::endl;
	}
	else {
		std::cout << " >>     " << Yellow << fileName << ColorEnd << " - " << Red << "Direction is unknown. No Output file" << ColorEnd << std::endl;
	}

	return 0;
}