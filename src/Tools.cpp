#include "Tools.h"

Tools::Tools()
{
    //ctor
}

Tools::~Tools()
{
    //dtor
}

std::vector<std::string> Tools::file_getAll(std::string path){
    std::vector<std::string> contents;
    std::ifstream file(path.c_str());
    std::string temp;
    if(file.good()){
        while (getline(file, temp)){
            contents.push_back(temp);
        }
    file.close();
    }
    return contents;
}

bool Tools::fileExists(const std::string& fileName){
  DWORD ftyp = GetFileAttributesA(fileName.c_str());
  if (ftyp == INVALID_FILE_ATTRIBUTES)
    return false;  //something is wrong with your path!

  return true;    // this is not a directory!
}

std::string Tools::setting_read(std::string setting, std::string path){
    std::string line;
    std::ifstream file;
    //printf("Opening file %s from %s\n", path.c_str(), pathexe.c_str());
    std::string fullPath = pathexe + path;
    std::string fetched_setting, var;
    file.open(fullPath.c_str());
    if (file.is_open())
	{
        for(int i = 0; file.good(); i++)
        {
            getline(file, line);
            fetched_setting = line.substr(0, line.find("=", 0));
            var = line.substr(line.find("=", 0)+1,line.size());
            if(fetched_setting==setting){
                    //cout<<"Setting found! Setting: "<<setting<<" with value "<<var<<endl;
                return var;
                break;
            }
        }
        //cout<<"Setting "<<setting<<" Not Found"<<endl;
        return "NOT_FOUND";
	} else {
	    std::cout<<"ERROR: Couldn't open file "<<fullPath<<", "<<strerror(errno)<<std::endl;
	}
    file.close();
    return "FILE_NOT_FOUND";
}

void Tools::splitString(std::string temp, std::string output[], char* tokens){
    int s=0;
    char str[255];
    strcpy(str, temp.c_str());
    char * pch;
    pch = strtok (str,tokens);
    while (pch != NULL){
        output[s] = pch;
        pch = strtok (NULL, tokens);
        s++;
    }
}

std::string Tools::to_upper(std::string input){
    std::locale loc;
    std::string temp;
    for (std::string::size_type i=0; i<input.length(); ++i)
        temp += std::toupper(input[i],loc);
    return temp;
}

std::string Tools::date(){
    time_t now = time(0);
    char* dt = ctime(&now);
    return dt;
}

int Tools::dateToInt(int day, int month, int year, int hour, int minute){
    char* temp;
    std::sprintf(temp,"%i%i%i%i%i",year,month,day,hour,minute);
    printf("formatted date: %s", temp);
    return atoi(temp);
}

/*
bool Tools::compareArray(const std::array<int, 2>& u, const std::array<int, 2>& v ){
    return u[1] < v[1];
};
*/
int Tools::throwError(int type, std::string info, int severity){

}

