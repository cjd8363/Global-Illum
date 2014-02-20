#ifndef _READER_H
#define _READER_H
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>
#include <sstream>

/*
	Extract strings from text file specified by char* location
*/
std::vector<std::string> getInfo (char* aloc);

/*
	Function to convert string into float
*/
float stringToNumber (const std::string &Text);

/*
	dimensions: outputs vector of float, from sample input
	converts strings into float.
*/
std::vector<float> dimensions (std::string &sample);

#endif 