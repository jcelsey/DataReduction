#include <cstdlib>
#include <vector>
#include <iostream>
#include <string>
#include <fstream>
#include <algorithm>
#include <numeric>
#include <stdexcept>
#include <cstdlib>
#include <iterator>

using namespace std;

struct Param {
	int vt = 0;
	int width = 0;
	int pulse_delta = 0;
	double drop_ratio = 0;
	int below_drop_ratio = 0;
};


int main(int argc, string argv[]) {
	vector<int> data;
	Param p;

	ifstream in("gage2scope.ini");
	if (!in.is_open()) {
		cout << "Couldn't open file";
	}
	while (!in.eof()) {
		string chars;

		getline(in, chars);
		if (chars == "" || chars.at(0) == '#')
			continue;
		string var = chars.substr(0, chars.find('='));
		string val = chars.substr(chars.find('=') + 1);

		try {
			if (var == "vt")p.vt = stoi(val);
			else if (var == "width") p.width = stoi(val);
			else  if (var == "pulse_delta") p.pulse_delta = stoi(val);
			else if (var == "drop_ratio") p.drop_ratio = stod(val);
			else if (var == "below_drop_ratio") p.below_drop_ratio = stoi(val);
			else {
				throw std::runtime_error("This variable isn't recognized: " + var);
			}

			//if (p.vt = 0) throw std::runtime_error("Missing voltage threshold param in .ini file");
			//if (p.width = 0) throw std::runtime_error("Missing width param in .ini file");
			//if (p.pulse_delta = 0) throw std::runtime_error("Missing pulse_delta param in .ini file");
			//if (p.drop_ratio = 0) throw std::runtime_error("Missing drop_ratio param in .ini file");
			//if (p.below_drop_ratio = 0) throw std::runtime_error("Missing below_drop_ratio param in .ini file");
		}
		catch (std::runtime_error& e) {
			cout << e.what() << endl;
			exit(EXIT_FAILURE);
		}
	}
	in.close();


	in.open("2_Record2308.dat");
	if (!in.is_open()) {
		cout << "Couldn't open file";
	}
	
	transform(istream_iterator<int>(in), istream_iterator<int>(), back_inserter(data), [](const int& i) {return (i * -1); });
	in.close();

	vector<int> smoothData;

	smoothData.push_back(data.at(0));
	smoothData.push_back(data.at(1));
	smoothData.push_back(data.at(2));
	vector<int>::iterator it = data.begin() + 2;
	transform(it, data.end() - 4, back_inserter(smoothData), [&it](const int& i) { it++; return (*(it - 3) + 2 * *(it - 2) + 3 * *(it - 1) + 3 * *(it) + 3 * *(it + 1) + 2 * *(it + 2) + *(it + 3)) / 15; });
	smoothData.push_back(data.at(data.size() - 3));
	smoothData.push_back(data.at(data.size() - 2));
	smoothData.push_back(data.at(data.size() - 1));

	vector<int> pulses;
	vector<int> area;
	bool rising = false;

	for (int i = 0; i < smoothData.size() - 3; i++) {
		int threshold = smoothData.at(i + 2) - smoothData.at(i);
		if (threshold > p.vt && !rising) {
			pulses.push_back(i);
			rising = true;
			continue;
		}
		if (rising) {
			if (threshold < 0) rising = false;
		}
	}

	for (int i = 0; i < pulses.size() - 1; i++) {
		int next_pulse = pulses.at(i) + p.pulse_delta;
		if (pulses.at(i + 1) <= next_pulse) {
			int pulse_peak = pulses.at(i);
			for (int j = pulses.at(i) + 1; j < pulses.at(i + 1); j++) {
				if (smoothData.at(pulse_peak) <= smoothData.at(j)) pulse_peak = j; //max_element
			}
			int drop_count = 0;
			double drop = smoothData.at(pulse_peak) * p.drop_ratio;
			for (int j = pulse_peak + 1; j < pulses.at(i + 1); j++) {
				if (smoothData.at(j) < drop) drop_count++; //count_if
			}
			if (drop_count > p.below_drop_ratio) {
				pulses.erase(pulses.begin() + i);
				i--;
			}
		}
	}

	for (int i = 0; i < pulses.size(); i++) {
		vector<int>::iterator it = data.begin() + pulses.at(i);
		vector<int>::iterator end;
		int sum = 0;	
		if (i == pulses.size() - 1) {
			if(data.size() > data.at(i) + p.width) end = it + p.width;
			else { end = data.end() - 1; }
			sum = accumulate(it, end, 0);
		}
		else {
			if (data.at(i + 1) > data.at(i) + p.width) end = data.begin() + p.width;
			else{ end = data.begin() + pulses.at(i + 1); }			
			sum = accumulate(it, end, 0);
		}			
		area.push_back(sum);
	}

	return 0;
}
