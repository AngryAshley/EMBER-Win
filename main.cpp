///////////////////////////////////////////////////////////////////////////////
/// Penrose Laboratories - EMBER-Win
/// Written by Ashley Verburg, (at)AngryAshley on GitHub
///
/// 22-9-2023
/////////

#include <iostream>
#include <ctime>
#include <windows.h>
#include <fstream>
#include <vector>
#include <unistd.h>
#include <cstdlib>
#include <string>

#include <Serial.h>
#include <Tools.h>

using namespace std;

/// Program specific variables ///
char incomingData[MAX_DATA_LENGTH];
string prog_ver="1.0.0 A";
string path_exe="";
bool actualModem=true;
bool modem_manualAnswer=true;

string port_name;
SerialPort serial;
Tools tools;

enum VideoTexProtocol {Prestel, Minitel, Viditel};

VideoTexProtocol currentProtocol;


void getPath() {
    char path_exe_temp[1024];
    GetModuleFileName(NULL, path_exe_temp, 1024);
    int pos=string(path_exe_temp).find_last_of("\\/");
    path_exe=string(path_exe_temp).substr( 0, pos+1);
    tools.pathexe=path_exe_temp;
}

void loadSettings(){
    printf("Loading settings\n");

    getPath();
    tools.pathexe = path_exe;
    string tempPort_name = tools.setting_read("ser_port", "\\Settings\\settings.txt");
    string settings_OS = tools.setting_read("sys_os", "\\Settings\\settings.txt");

    printf(" - OS: %s\n",settings_OS.c_str());
    if(settings_OS=="win"){
        port_name="COM"+tempPort_name;
        printf(" - Port: %s\n",port_name.c_str());
    }
    printf("[INFO] Done loading settings\n");
}

string serialGetLine(bool discardPureCRLF=true){ //a quick and dirty test'n'fix
    string line;
    char c;
    do{
        line="";
        do{
            c=serial.getKey()[0];
            switch(c){
                case '\x0A': break;
                case '\x0D': break;
                default: line+=c; break;
            }
            //printf(" c<%c>",c);
        } while (c!='\x0A');
    }while(discardPureCRLF?line=="":false);
    return line;
}

string modem_getPageNumber(){
    string line;
    char c;
        do{
            c=serial.getKey()[0];
            switch(c){
                case '*': break;
                case '#': break;
                case '_': break;
                default: line+=c; break;
            }
            serial.write(string(1,c));//echo the chars back
            printf("%02X",c);
            if(line=="\x13\x43")return line; // Herhaal   ////these special characters are only minitel compatible? CURIOUS!
            if(line=="\x13I")break; //verbinding einde
            if(line=="\x13\x42")break; //vorig
            if(line=="\x13\x48")break; //volgende
            if(line=="\x13\x46")break; //index
        } while (c!='#'&&c!='_');
    return line;
}


string modem_awaitMessage(char* reply, bool waitUntilFalse=false){
    string line;
    do{
        line=serialGetLine(false);
        printf("Waiting and got <%s>\n",line.c_str());
        if(line==reply&&!waitUntilFalse)break;
        if(line!=reply&&waitUntilFalse)break;
    }while(true);
    return line;
}

string modem_sendCommand(char* command, char* await = "OK"){ //await command
    string line;
    if(await == "*")await=command;

    serial.print(string(command));
    return modem_awaitMessage(await);
}

int waitForModemConnect(){
    printf("[....] Initialising modem...\n");

        ///implement ATZ later to prepare for connection recycling. Do this when you can uh... read actual data back.
        string line;
        modem_sendCommand("ATZ");
        modem_sendCommand("ATB3"); //normally atb3
        printf("       Waiting for modem connect...\n");

        if(modem_manualAnswer){ //consolidate this into modern commands once you use a manualAnswer modem
            bool NC=true;
            while(NC){
                //string ch = serial.getKey();
                string line = serialGetLine();
                //string line = serial.readLine(3);
                printf("<%s>\n",line.c_str());
                if(line=="RING")NC=false;
                if(line[0]=='\x0D')NC=false;
            }
            printf("       Modem Ringing! Going off hook...\n");
            Sleep(100); //usually 100 is fine
            serial.print("ATA");
        }

        modem_awaitMessage("CONNECT 1200");

        printf("\n[ OK ] Modem connected, handing off to Server...\n\n");
}

void modemHangUp(){ ///fix this one!!
    //modem_sendCommand("+++");
    serial.write("+");
    Sleep(100);
    serial.write("+");
    Sleep(100);
    serial.write("+");

    Sleep(1100);
    serial.write("ATH\r\n");
}

int commandLine(){


}
///stinky support functions
string getControlByte(char controlLetter){
    switch(controlLetter){
        case 'n': return "\n";
        case 'r': return "\r";
        case 'e': return "\e";
        case '\\': return "\\";
    }
}


/// end of stinky support functions

int writeTeletextScreen(string file){
    bool writing=true;
    int bytesThisLine=0;
    string fullPath = path_exe+"\\pages\\"+file;
    if(tools.fileExists(fullPath)){
        printf("\n\e[1;37m       Requested %s\n\e[0m",fullPath.c_str());
        serial.write("\x0C\x1f\x40\x40");
        int bytesSent=0;
        vector<string> page = tools.file_getAll(fullPath);
        printf("File line 1 [%s]",page[0].c_str());
        for(int i=0; i<page.size(); i++){
            for(int j=0; j<page[i].length(); j++){
                if(bytesThisLine==0)serial.write("\x0F");
                switch(page[i][j]){

                    case '': if(bytesThisLine==0){
                                   serial.write(" ");
                                } else {
                                    serial.write("\x0E\eG ");
                                }

                                break;
                    case '': serial.write("\x0E\eE "); break;
                    case '': serial.write("\x0E\eB "); break;
                    case '': serial.write("\x0E\eF "); break;
                    case '': serial.write("\x0E\eD "); break;
                    case '': serial.write("\x0E\eC "); break;


                    default: if(writing){
                                serial.write(string(1,page[i][j]));
                                bytesSent++;
                            }
                             //printf("<%c>",page[i][j]);
                             break;
                }

                bytesThisLine++;
                if(bytesThisLine==40)bytesThisLine=0;
                if(bytesThisLine==page[i].size())serial.write("\n\r");
            }
        }
        printf("\n\e[1;37m       Printed page %s, %d bytes sent\n\e[0m",file.c_str(),bytesSent);
        return 0;
    } else {
    printf("\n\n\e[1;31mERROR - File %s not found\e[0m\n\n",fullPath.c_str());
        return 1;
    }
}


int writeScreen(string file){
    bool writing=true;
    string fullPath = path_exe+"\\pages\\"+file;
    if(tools.fileExists(fullPath)){
        printf("\n\e[1;37m       Requested %s\n\e[0m",fullPath.c_str());
        int bytesSent=0;
        vector<string> page = tools.file_getAll(fullPath);
        printf("File line 1 [%s]",page[0].c_str());
        for(int i=0; i<page.size(); i++){
            for(int j=0; j<page[i].length(); j++){
                switch(page[i][j]){
                    case '\\':  j++;
                                if(page[i][j]=='#'){//file comment mode!
                                    writing= !writing;
                                } else if(page[i][j]!='x'){
                                    if(writing){
                                        serial.write(getControlByte(page[i][j]));
                                        if(page[i][j]!='r')printf("<\\%c>",page[i][j]);
                                        bytesSent++;
                                    }
                                } else {

                                   string compose(1,page[i][++j]);
                                   compose +=page[i][++j];
                                   uint8_t byte = strtoul(compose.c_str(), NULL, 16);
                                   if(writing){
                                        serial.write(string(1,byte));
                                        printf("<\\x%s>",compose.c_str());
                                        bytesSent++;
                                   }
                                }
                                break;


                    default: if(writing){
                                serial.write(string(1,page[i][j]));
                                bytesSent++;
                            }
                             //printf("<%c>",page[i][j]);
                             break;
                }
            }
        }
        printf("\n\e[1;37m       Printed page %s, %d bytes sent\n\e[0m",file.c_str(),bytesSent);
        return 0;
    } else {
    printf("\n\n\e[1;31mERROR - File %s not found\e[0m\n\n",fullPath.c_str());
        return 1;
    }
}

int main()
{
    Sleep(1000);
    system("echo \" \"");
    printf("\e[2JParadigm VidiTel Server %s\n",prog_ver.c_str());
    //system("pause");
    loadSettings();
    printf("[....] Establishing connection with %s... ",port_name.c_str());
    int result = serial.connect(port_name);
    if(result!=0){
        printf("\e[80D\e[2A[\e[1;31mFAIL\e[0m\e[2B\n");
        printf("A fatal error occured while initialising the serial connection.");
        return 1;//probably recoverable in a way but I care far too little to fix that rn.

    } else {
        printf("\e[80D[ \e[1;32mOK\e[0m \n");
    }

    if(actualModem){
        waitForModemConnect();
    }
    printf("[INFO] User logged on\n");
    bool connected=true;
    string gateway;
    string indexPage;
    string currentPage;
    string lastPage; //full page
    string nextPage;
    string previousPage;

    string accessCode = "-TESTING";

    writeScreen("penroseSystem\\signon.txt");
    string currentAC = "";

    while(true){
        string c= serial.getKey();

        printf("<%c>",c[0]);
        if(c[0]==0x13){
            if(accessCode==currentAC){
                break;
            } else {
                char* str;
                int strLen =currentAC.length()+40;
                sprintf(str,"\x08\x12%i \x12%i\x08\x12%i",strLen,strLen,strLen);
                serial.write(string(str));
                currentAC="";
            }
        }
        currentAC+=c;
        serial.write("*");
    }

        while(connected){

            writeScreen("minitel\\_mainGateway.txt");
            serial.write("\x1F\x53\x41\e\x4D Please select *gateway#:");
            gateway="";
            gateway=modem_getPageNumber();
            printf("\nConnecting to %s\n",gateway.c_str());

            if(gateway=="000"){ //creatureTel - Ashley's Minitel Playground.
                gateway="CreatureTel";
                indexPage="000";
                currentPage="000";

                int reply;
                do{
                    reply = writeScreen("minitel\\"+gateway+"\\"+currentPage+".txt");
                    if(reply==1){
                        serial.write("\x1F\x58\x41\eT \x12\x67\x1f\x58\x41\eT \eCNo such page! \eG*PAGE#\eC:");
                        currentPage=lastPage;
                    }
                    lastPage=currentPage;
                    currentPage = modem_getPageNumber();

                    if(currentPage=="\x13\x43") currentPage=lastPage;
                    //if(currentPage=="")
                    if(currentPage=="\x13\x46"){
                       if(lastPage==indexPage){ //if we're already at the page's index...
                        serial.write("\r Return to the Gateway? [Y/N]");
                        if(serial.getKey()=="Y"){
                            break;
                        } else {
                            printf("We're staying here");
                            currentPage=indexPage;
                        }
                       }else{
                        currentPage=indexPage; //just return to the current index
                       }
                    }
                    if(currentPage=="\x13I"){
                        connected=false;
                        break;
                    }
                }while(true);


            } else if(gateway=="\x13I"){ //disconnect
                connected=false;
            }
        }

    modemHangUp();

    printf("[INFO] User disconnected\n");

    system("pause");
    return 0;
}
