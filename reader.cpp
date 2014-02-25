#include "Reader.h"

std::vector<std::string> getInfo (char* aloc){
	std::vector<std::string> infoStruct;
	std::ifstream fileOpen;
	char buffer[100];
	fileOpen.open(aloc);
	printf("SOFAR");
	// Exception check
	if (fileOpen.fail()){
		std::cout << "file does not exist"<<std::endl;
	}
	
	while(fileOpen.good()){
		fileOpen.getline(buffer,100, '\n'); 
        printf("GET IT");
		infoStruct.push_back(buffer);
	}
	return infoStruct;
}

float stringToNumber (const std::string &Text){
	std::stringstream ss(Text);
	float result;
	return ss >> result ? result : 0;
}

std::vector<float> dimensions (std::string &sample){
	std::string delim = ",";
	size_t pos = 0;
	std::vector<float> result;
	char* temp = new char [sample.length() +1];
    std::strcpy (temp, sample.c_str());
    char * p = std::strtok(temp, ", ");
	while (p!=NULL)
    {
        std::string temp2 = std::string(p);
        result.push_back(stringToNumber(temp2));
        p = std::strtok(NULL, " ");
    }
    /*
    while ((pos = sample.find(delim)) != (std::string::npos)){
		std::string token = sample.substr(0,pos);
		result.push_back(stringToNumber(token));
		sample.erase(0, pos + delim.length());
	} */

	result.push_back(stringToNumber(sample));
	return result;
}