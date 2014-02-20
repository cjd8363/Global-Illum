#include "reader.h"

std::vector<std::string> getInfo (char* aloc){
	std::vector<std::string> infoStruct;
	std::ifstream fileOpen;
	char buffer[40];
	fileOpen.open(aloc);
	
	// Exception check
	if (fileOpen.fail()){
		std::cout << "file does not exist"<<std::endl;
	}
	
	while(!fileOpen.eof()){
		fileOpen.getline(buffer,40);
		infoStruct.push_back(buffer);
	}
	return infoStruct;
}

float StringToNumber (const std::string &Text){
	std::stringstream ss(Text);
	float result;
	return ss >> result ? result : 0;
}

std::vector<float> dimensions (std::string &sample){
	std::string delim = ",";
	size_t pos = 0;
	std::vector<float> result;
	
	while ((pos = sample.find(delim)) != (std::string::npos)){
		std::string token = sample.substr(0,pos);
		result.push_back(StringToNumber(token));
		sample.erase(0, pos + delim.length());
	} 

	result.push_back(StringToNumber(sample));
	return result;
}